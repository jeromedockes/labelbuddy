import re
import hashlib
import itertools
import sqlite3

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
                "title": str(i),
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
                "title": str(i),
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
    }


def random_annotations(doc_len, n_anno, labels, seed=0):
    annotations = set()
    rng = np.random.default_rng(seed)
    for i in range(n_anno):
        start = int(rng.integers(0, doc_len - 1))
        stop = start + rng.poisson(30)
        stop = int(np.clip(stop, start + 1, doc_len))
        label = rng.choice(labels)
        annotations.add((label, start, stop))
    return list(annotations)


def random_non_overlapping_annotations(doc_len, n_anno, labels, seed=0):
    annotations = set()
    rng = np.random.default_rng(seed)
    start = 0
    for i in range(n_anno):
        start = start + rng.poisson(5)
        if start > doc_len - 1:
            break
        stop = start + rng.poisson(20)
        stop = int(np.clip(stop, start + 1, doc_len))
        label = int(rng.choice(labels))
        annotations.add((label, start, stop))
        start = stop
    return list(annotations)


def add_random_annotations(
    db, step=2, n_anno=20, seed=0, annotation_generator=random_annotations
):
    con = sqlite3.connect(db)
    con.execute("pragma foreign_keys = on;")
    docs = con.execute(
        "select id, length(content) as len from document;"
    ).fetchall()
    label_ids = [int(r[0]) for r in con.execute("select id from label;")]
    with con:
        for i, (doc_id, doc_len) in itertools.islice(
            enumerate(docs), 0, None, step
        ):
            annotations = annotation_generator(
                doc_len=doc_len, n_anno=n_anno, labels=label_ids, seed=seed
            )
            anno = [
                (int(doc_id), int(label), start, stop)
                for (label, start, stop) in annotations
            ]
            con.executemany(
                "insert into annotation "
                "(doc_id, label_id, start_char, end_char) "
                "values (?, ?, ?, ?)",
                anno,
            )
