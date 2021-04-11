import re
import hashlib
import itertools
import sqlite3
import string

import numpy as np
from sklearn.datasets import fetch_20newsgroups


def clean_xml(text):
    illegal_xml_re = re.compile(
        "[\x00-\x08\x0b-\x1f\x7f-\x84\x86-\x9f"
        "\ud800-\udfff\ufdd0-\ufddf\ufffe-\uffff]"
    )
    return illegal_xml_re.sub("", text)


def fetch_newsgroups(n_docs=None):
    newsgroups = fetch_20newsgroups(shuffle=False, subset="all")
    all_docs, all_targets = newsgroups["data"], newsgroups["target"]
    if n_docs is not None:
        all_docs, all_targets = all_docs[:n_docs], all_targets[:n_docs]

    json_ready = []
    xml_ready = []
    txt_ready = []
    for i, (doc, target) in enumerate(zip(all_docs, all_targets)):
        target_name = newsgroups["target_names"][target]
        header = doc.split("\n")[1].replace("Subject:", "").strip()
        long_title = f"{i} {target_name}: {header}"
        short_title = f"{i} {target_name}"
        json_ready.append(
            {
                "text": doc,
                "long_title": long_title,
                "id": str(i),
                "short_title": short_title,
                "meta": {
                    "target": int(target),
                    "target_name": target_name,
                    "md5": hashlib.md5(doc.encode("utf-8")).hexdigest(),
                    "pos_in_dataset": i,
                },
            }
        )
        xml_doc = clean_xml(doc)
        xml_ready.append(
            {
                "text": xml_doc,
                "long_title": clean_xml(long_title),
                "id": str(i),
                "short_title": clean_xml(short_title),
                "meta": {
                    "target": str(target),
                    "target_name": clean_xml(target_name),
                    "md5": hashlib.md5(xml_doc.encode("utf-8")).hexdigest(),
                    "pos_in_dataset": str(i),
                },
            }
        )
        txt_ready.append({"text": doc.replace("\n", r"\n")})

    return {
        "json": json_ready,
        "txt": txt_ready,
        "xml": xml_ready,
        "target_names": newsgroups["target_names"],
        "labels": get_labels(newsgroups["target_names"]),
    }


def get_labels(label_names):
    mpl_tab_colors = [
        "#aec7e8",
        "#ffbb78",
        "#98df8a",
        "#ff9896",
        "#c5b0d5",
        "#c49c94",
        "#f7b6d2",
        "#c7c7c7",
        "#dbdb8d",
        "#9edae5",
    ]
    letters = [chr(ord("a") + i) for i in range(26)]
    labels = [
        {
            "text": label,
            "background_color": mpl_tab_colors[i % len(mpl_tab_colors)],
            "shortcut_key": letters[i],
        }
        for (i, label) in enumerate(label_names)
    ]
    return labels


def random_annotations(doc_len, n_anno, labels, seed=0, chars=None):
    if chars is None:
        chars = list(string.printable)
    annotations = {}
    rng = np.random.default_rng(seed)
    for i in range(n_anno):
        start = int(rng.integers(0, doc_len - 1))
        stop = start + rng.poisson(30)
        stop = int(np.clip(stop, start + 1, doc_len))
        label = rng.choice(labels)
        extra = (
            None if i % 2 else "".join(rng.choice(chars, size=rng.poisson(30)))
        )
        annotations[(label, start, stop)] = (label, start, stop, extra)
    return list(annotations.values())


def random_non_overlapping_annotations(
    doc_len, n_anno, labels, seed=0, chars=None
):
    if chars is None:
        chars = list(string.printable)
    annotations = {}
    rng = np.random.default_rng(seed)
    start = 0
    for i in range(n_anno):
        start = start + rng.poisson(5)
        if start > doc_len - 1:
            break
        stop = start + rng.poisson(20)
        stop = int(np.clip(stop, start + 1, doc_len))
        label = int(rng.choice(labels))
        extra = (
            None if i % 2 else "".join(rng.choice(chars, size=rng.poisson(30)))
        )
        annotations[(label, start, stop)] = (label, start, stop, extra)
        start = stop
    return list(annotations.values())


def add_random_annotations(
    db,
    step=2,
    n_anno=20,
    seed=0,
    annotation_generator=random_annotations,
    max_docs=None,
    subsample_labels=False,
):
    con = sqlite3.connect(db)
    con.execute("pragma foreign_keys = on;")
    docs = con.execute(
        "select id, length(content) as len from document;"
    ).fetchall()
    label_ids = [int(r[0]) for r in con.execute("select id from label;")]
    chars = list(string.printable)
    rng = np.random.default_rng(seed)
    with con:
        for i, (doc_id, doc_len) in itertools.islice(
            enumerate(docs), 0, None, step
        ):
            if max_docs is not None and i >= max_docs:
                break
            print(
                f"random annotations: {i + 1}",
                end="\r",
                flush=True,
            )
            if subsample_labels:
                lab = rng.choice(
                    label_ids, size=len(label_ids) // 2, replace=False
                )
            else:
                lab = label_ids
            annotations = annotation_generator(
                doc_len=doc_len, n_anno=n_anno, labels=lab, seed=i, chars=chars
            )
            anno = [
                (int(doc_id), int(label), start, stop, extra)
                for (label, start, stop, extra) in annotations
            ]
            con.executemany(
                "insert into annotation "
                "(doc_id, label_id, start_char, end_char, extra_data) "
                "values (?, ?, ?, ?, ?)",
                anno,
            )
    print("\n")


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()
    parser.add_argument("db", type=str)
    parser.add_argument("-n", "--n_anno", type=int, default=150)
    parser.add_argument("-d", "--n_docs", type=int, default=None)
    parser.add_argument("-s", "--step", type=int, default=2)
    parser.add_argument("-l", "--subsample-labels", action="store_true")
    args = parser.parse_args()
    add_random_annotations(
        args.db,
        n_anno=args.n_anno,
        max_docs=args.n_docs,
        step=args.step,
        subsample_labels=args.subsample_labels,
    )
