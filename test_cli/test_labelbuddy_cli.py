import math
from pathlib import Path
import hashlib
import os
import subprocess
import shutil
import sqlite3
import pickle
import json
import tempfile
import numpy as np
import pytest

from data_gen import add_random_annotations


@pytest.fixture
def cd_to_tempdir(tmp_path):
    old_dir = os.getcwd()
    new_dir = tmp_path / "working_directory"
    new_dir.mkdir()
    os.chdir(new_dir)
    yield
    os.chdir(old_dir)


@pytest.fixture(scope="session")
def lb_command():
    command = os.environ.get("LABELBUDDY_COMMAND", "")
    if command == "":
        raise RuntimeError("Please export $LABELBUDDY_COMMAND")
    command_path = Path(command)
    if command_path.is_file():
        command = str(command_path.absolute())
    return command


@pytest.fixture(autouse=True)
def env_config(tmp_path):
    os.environ["XDG_CONFIG_HOME"] = str(tmp_path / "config")


@pytest.fixture(scope="session")
def labelbuddy(lb_command):
    def fun(*args, capture_output=True, **kwargs):
        return subprocess.run(
            [lb_command] + [str(a) for a in args],
            capture_output=capture_output,
            **kwargs,
        )

    return fun


@pytest.fixture(scope="session")
def ng():
    ng_dir = Path(__file__).parent.joinpath("data", "newsgroups")
    res = {}
    for path in sorted(ng_dir.glob("*")):
        res[path.name] = str(path.absolute())
    return res


@pytest.fixture()
def preloaded_db(tmp_path, labelbuddy, ng):
    db = tmp_path / "preloaded_db.labelbuddy"
    labels = ng["labels.json"]
    docs = ng["docs_0-15.json"]
    labelbuddy(db, "--import-labels", labels, "--import-docs", docs)
    add_random_annotations(db)
    return db


def compare_dicts_excluding_keys(dict_a, dict_b, excluded):
    dict_a = dict(dict_a)
    dict_b = dict(dict_b)
    for k in excluded:
        dict_a.pop(k, None)
        dict_b.pop(k, None)
    return dict_a == dict_b


def compare_docs(db_doc, input_doc, use_meta_md5=True):
    assert "text" in input_doc
    for k in ["text", "display_title", "list_title"]:
        db_k = {"text": "content"}.get(k, k)
        if k in input_doc:
            assert input_doc[k] == db_doc[db_k]
        else:
            assert db_doc[db_k] is None
    if "metadata" in input_doc:
        db_doc_meta = json.loads(db_doc["metadata"])
        excluded = [] if use_meta_md5 else ["md5"]
        assert compare_dicts_excluding_keys(
            input_doc["metadata"], db_doc_meta, excluded
        )
        if use_meta_md5:
            assert db_doc["content_md5"].hex() == input_doc["metadata"]["md5"]
    assert (
        db_doc["content_md5"].hex()
        == hashlib.md5(input_doc["text"].encode("utf-8")).hexdigest()
    )


def check_imported_docs(
    db, all_input_docs, same_order=False, n_docs=None, use_meta_md5=True
):
    con = sqlite3.connect(db)
    if n_docs is not None:
        assert (
            con.execute("select count(*) from document;").fetchone()[0]
            == n_docs
        )
    con.row_factory = sqlite3.Row
    db_rows = con.execute("select * from document order by id;").fetchall()
    if same_order:
        for input_doc, db_doc in zip(all_input_docs, db_rows):
            compare_docs(db_doc, input_doc, use_meta_md5=use_meta_md5)
    else:
        all_db_docs = {row["content"]: row for row in db_rows}
        for input_doc in all_input_docs:
            db_doc = all_db_docs[input_doc["text"]]
            compare_docs(db_doc, input_doc, use_meta_md5=use_meta_md5)


def compare_databases(db_1, db_2):
    res = subprocess.run(
        ["sqldiff", str(db_1), str(db_2)], capture_output=True
    )
    if res.stdout == b"":
        return True
    return False


def check_import_back(db, labelbuddy):
    with tempfile.TemporaryDirectory() as tmp_dir:
        tmp_dir = Path(tmp_dir)
        docs = tmp_dir / "docs.jsonl"
        labels = tmp_dir / "labels.json"
        db_1 = tmp_dir / "db"
        labelbuddy(db, "--export-docs", docs, "--export-labels", labels)
        labelbuddy(db_1, "--import-docs", docs, "--import-labels", labels)
        return compare_databases(db, db_1)


def check_exported_json(
    docs_f,
    n_docs=15,
    labelled_only=False,
    no_text=False,
    no_annotations=False,
):
    docs = json.loads(docs_f.read_text(encoding="utf-8"))
    return check_exported_as_dict(
        docs,
        n_docs=n_docs,
        labelled_only=labelled_only,
        no_text=no_text,
        no_annotations=no_annotations,
    )


def check_exported_jsonl(
    docs_f,
    n_docs=15,
    labelled_only=False,
    no_text=False,
    no_annotations=False,
):
    docs = [
        json.loads(doc)
        for doc in docs_f.read_text(encoding="utf-8").strip().split("\n")
    ]
    return check_exported_as_dict(
        docs,
        n_docs=n_docs,
        labelled_only=labelled_only,
        no_text=no_text,
        no_annotations=no_annotations,
    )


def check_exported_as_dict(
    docs,
    n_docs=15,
    labelled_only=False,
    no_text=False,
    no_annotations=False,
):
    if n_docs is not None:
        assert len(docs) == math.ceil(n_docs / 2) if labelled_only else n_docs
    for i, doc in enumerate(docs):
        assert "utf8_text_md5_checksum" in doc
        if no_annotations:
            assert "annotations" not in doc
            assert "start_char" not in doc
        else:
            assert "start_char" in doc or "annotations" in doc
            if "start_char" in doc:
                if labelled_only:
                    assert int(doc["start_char"]) < int(doc["end_char"])
                else:
                    assert (
                        doc["start_char"] == "" and doc["end_char"] == ""
                    ) or (int(doc["start_char"]) < int(doc["end_char"]))
            else:
                if labelled_only:
                    assert len(doc["annotations"]) > 0
                else:
                    assert (len(doc["annotations"]) > 0) == (not i % 2)
        if no_text:
            assert "text" not in doc
        else:
            assert len(doc["text"]) > 0
            assert (
                doc["utf8_text_md5_checksum"]
                == hashlib.md5(doc["text"].encode("utf-8")).hexdigest()
            )


def shuffle_docs(docs_f, seed=0):
    rng = np.random.default_rng(seed)
    if docs_f.suffix == "json":
        docs = json.loads(docs_f.read_text("utf-8"))
        rng.shuffle(docs)
        docs_f.write_text(json.dumps(docs), encoding="utf-8")
    if docs_f.suffix == "jsonl":
        docs = [
            json.loads(doc)
            for doc in docs_f.read_text("utf-8").strip().split("\n")
        ]
        rng.shuffle(docs)
        docs_f.write_text(
            "\n".join([json.dumps(doc) for doc in docs]), encoding="utf-8"
        )


def test_no_specified_db(labelbuddy, tmp_path):
    db = tmp_path / "db.labelbuddy"
    labels_f = tmp_path / "labels.json"
    res = labelbuddy(db, "--export-labels", labels_f)
    assert res.returncode == 0
    assert Path(db).exists()
    assert Path(labels_f).exists()
    labels_f = tmp_path / "labels_1.json"
    res = labelbuddy("--export-labels", labels_f)
    assert b"Specify database" in res.stderr
    assert res.returncode == 1
    assert not labels_f.exists()


def test_import_bad_extension_skipped(labelbuddy, tmp_path, ng):
    abc_docs = tmp_path / "docs.abc"
    abc_labels = tmp_path / "labels.abc"
    shutil.copy(ng["docs_0-11.json"], abc_docs)
    shutil.copy(ng["labels.json"], abc_labels)
    db = tmp_path / "db.labelbuddy"
    res = labelbuddy(
        db, "--import-docs", abc_docs, "--import-labels", abc_labels
    )
    assert res.returncode == 1
    con = sqlite3.connect(db)
    assert con.execute("select count(*) from label").fetchone()[0] == 0
    assert con.execute("select count(*) from document").fetchone()[0] == 0


def test_import_missing_file_skipped(labelbuddy, tmp_path, ng):
    db = tmp_path / "db.labelbuddy"
    res = labelbuddy(
        db,
        "--import-docs",
        tmp_path / "missing_docs.json",
        "--import-docs",
        ng["docs_0-11.json"],
        "--import-labels",
        tmp_path / "missing_labels.json",
        "--import-labels",
        ng["labels.json"],
    )
    assert res.returncode == 1
    con = sqlite3.connect(db)
    assert con.execute("select count(*) from label").fetchone()[0] == 20
    assert con.execute("select count(*) from document").fetchone()[0] == 11


@pytest.mark.parametrize(
    "formats",
    [("json", "jsonl"), ("jsonl", "json"), ("txt", "txt")],
)
def test_import_docs(formats, labelbuddy, tmp_path, ng):
    db = tmp_path / "db.labelbuddy"
    res = labelbuddy(
        db,
        "--import-docs",
        ng[f"docs_0-11.{formats[0]}"],
        "--import-docs",
        ng[f"docs_6-15.{formats[1]}"],
    )
    assert res.returncode == 0
    with open(ng[f"{formats[0]}_data_docs_0-15.pkl"], "rb") as f:
        batch_0 = pickle.load(f)
    with open(ng[f"{formats[1]}_data_docs_0-15.pkl"], "rb") as f:
        batch_1 = pickle.load(f)
    input_docs = batch_0[0:11] + batch_1[11:15]
    check_imported_docs(db, input_docs, same_order=False, n_docs=15)


@pytest.mark.parametrize(
    "formats",
    [
        ("json", "json"),
        ("json", "jsonl"),
        ("jsonl", "json"),
        ("txt", "json"),
    ],
)
@pytest.mark.parametrize("subset", ["0-15", "0-300"])
def test_conversion_and_import_back(formats, subset, labelbuddy, tmp_path, ng):
    db = tmp_path / "db.labelbuddy"
    source_docs = ng[f"docs_{subset}.{formats[0]}"]
    target_docs = tmp_path / f"exported.{formats[1]}"
    labelbuddy(db, "--import-docs", source_docs, "--export-docs", target_docs)
    new_db = tmp_path / "new_db.labelbuddy"
    labelbuddy(new_db, "--import-docs", target_docs)
    ref_format = next(
        filter(formats.__contains__, ["txt", "jsonl", "json"])
    )
    with open(ng[f"{ref_format}_data_docs_{subset}.pkl"], "rb") as f:
        ref_docs = pickle.load(f)
    equiv = ["json", "jsonl"]
    use_meta_md5 = (formats[0] == formats[1]) or (
        formats[0] in equiv and formats[1] in equiv
    )
    check_imported_docs(
        new_db,
        ref_docs,
        n_docs=int(subset.split("-")[1]),
        use_meta_md5=use_meta_md5,
    )


@pytest.mark.parametrize(
    ["doc_format", "label_format"],
    [
        ("json", "txt"),
        ("jsonl", "json"),
    ],
)
def test_import_back(doc_format, label_format, tmp_path, ng, labelbuddy):
    db = tmp_path / "db"
    docs = ng[f"docs_0-300.{doc_format}"]
    labels = ng[f"labels.{label_format}"]
    labelbuddy(db, "--import-labels", labels, "--import-docs", docs)
    add_random_annotations(db)
    con = sqlite3.connect(db)
    n_anno = con.execute("select count(*) from annotation;").fetchone()[0]
    assert n_anno == 3000
    assert (
        con.execute(
            "select count(*) from annotation where extra_data is not null;"
        ).fetchone()[0]
        == 1500
    )
    assert check_import_back(db, labelbuddy)


@pytest.mark.parametrize("doc_format", ["json", "jsonl"])
@pytest.mark.parametrize("labelled_only", [True, False])
@pytest.mark.parametrize("no_text", [True, False])
def test_import_annotations(
    doc_format, labelled_only, no_text, preloaded_db, tmp_path, labelbuddy
):
    new_db = tmp_path / "db"
    shutil.copy(preloaded_db, new_db)
    con = sqlite3.connect(new_db)
    con.execute("pragma foreign_keys = on;")
    with con:
        con.execute("delete from annotation;")
        assert (
            con.execute("select count(*) from annotation;").fetchone()[0] == 0
        )
    docs = tmp_path / f"docs.{doc_format}"
    labels = tmp_path / "labels.json"

    args = []
    if labelled_only:
        args.append("--labelled-only")
    if no_text:
        args.append("--no-text")
    labelbuddy(
        preloaded_db, "--export-docs", docs, "--export-labels", labels, *args
    )
    shuffle_docs(docs)
    labelbuddy(new_db, "--import-docs", docs, "--import-labels", labels)
    compare_databases(preloaded_db, new_db)
    labelbuddy(new_db, "--import-docs", docs, "--import-labels", labels)
    compare_databases(preloaded_db, new_db)


def test_vacuum(preloaded_db, labelbuddy):
    original_size = preloaded_db.stat().st_size
    con = sqlite3.connect(preloaded_db)
    with con:
        con.execute("delete from document;")
    labelbuddy(preloaded_db, "--vacuum")
    assert preloaded_db.stat().st_size < original_size


@pytest.mark.parametrize("doc_format", ["json", "jsonl"])
@pytest.mark.parametrize("labelled_only", [True, False])
@pytest.mark.parametrize("no_text", [True, False])
@pytest.mark.parametrize("no_annotations", [True, False])
def test_export_options(
    doc_format,
    labelled_only,
    no_text,
    no_annotations,
    preloaded_db,
    labelbuddy,
    tmp_path,
):
    docs = tmp_path / f"docs.{doc_format}"
    args = []
    if no_annotations:
        args.append("--no-annotations")
    if no_text:
        args.append("--no-text")
    if labelled_only:
        args.append("--labelled-only")

    labelbuddy(preloaded_db, "--export-docs", docs, *args)
    check = {
        "json": check_exported_json,
        "jsonl": check_exported_jsonl,
    }[doc_format]
    check(
        docs,
        labelled_only=labelled_only,
        no_text=no_text,
        no_annotations=no_annotations,
    )


def test_control_characters(tmp_path, labelbuddy):
    text = "\u000c,\u0000,<,&"
    docs = [{"text": text}]
    json_docs = tmp_path / "docs.json"
    exported_json_docs = tmp_path / "exported_docs.json"
    json_docs.write_text(json.dumps(docs), encoding="utf-8")
    db = tmp_path / "db"
    labelbuddy(db, "--import-docs", json_docs)
    labelbuddy(db, "--export-docs", exported_json_docs)
    assert (
        json.loads(exported_json_docs.read_text(encoding="utf-8"))[0]["text"]
        == text
    )


def test_using_memory_db(labelbuddy, ng, tmp_path, cd_to_tempdir):
    docs = tmp_path / "docs.json"
    labelbuddy(
        ":memory:",
        "--import-docs",
        ng["docs_0-15.json"],
        "--export-docs",
        docs,
    )
    loaded = json.loads(docs.read_text("utf-8"))
    assert len(loaded) == 15
    assert not Path(":memory:").is_file()


def test_using_relative_path(labelbuddy, ng, tmp_path, cd_to_tempdir):
    shutil.copy(ng["docs_0-15.json"], "./docs.json")
    labelbuddy(
        "./db",
        "--import-docs",
        "./docs.json",
    )
    con = sqlite3.connect("./db")
    assert con.execute("select count(*) from document").fetchone()[0] == 15


def test_import_duplicate_shortcut_key(labelbuddy, tmp_path):
    in_f = tmp_path.joinpath("labels.json")
    in_f.write_text(
        json.dumps(
            [
                {"name": "lab1", "shortcut_key": "a"},
                {"name": "lab2", "shortcut_key": "a"},
            ]
        )
    )
    out_f = tmp_path.joinpath("labels_out.json")
    labelbuddy(":memory:", "--import-labels", in_f, "--export-labels", out_f)
    exported = json.loads(out_f.read_text())
    assert len(exported) == 2
    assert exported[0]["name"] == "lab1"
    assert exported[0]["shortcut_key"] == "a"
    assert exported[1]["name"] == "lab2"
    assert "shortcut_key" not in exported[1]
