#! /usr/bin/env python3

import hashlib
from pathlib import Path
import json
import re


def get_annotations(doc, annotations):
    result = []
    for pattern, label in annotations:
        match = re.search(pattern, doc)
        if match is not None:
            start = match.start(1)
            end = match.end(1)
            result.append([start, end, label])
    return result


example_dir = Path(__file__).parent
doc_file_names = [
    "hello_annotations.txt",
    "documentation.txt",
    "wikipedia_language_ar.txt",
    "wikipedia_language_el.txt",
    "wikipedia_language_zh.txt",
]

all_docs = []

for doc_name in doc_file_names:
    doc = example_dir.joinpath(doc_name).read_text(encoding="utf-8")
    lines = doc.split("\n")
    title = lines[0].strip()
    long_title = re.sub(r"<a[^>]*>([^<]*)</a>", r"\1", title)
    if doc_name == "documentation.txt":
        title = (
            "labelbuddy documentation — "
            "<a href='https://jeromedockes.github.io/labelbuddy/"
            "documentation.html'>online version</a>"
        )
        long_title = "labelbuddy documentation"
        body = doc
    else:
        body = "\n".join(lines[1:])
    if doc_name == "hello_annotations.txt":
        annotations = get_annotations(
            body,
            [
                (r"(You) don't", "pronoun"),
                (r"(annotating) it instead", "verb"),
                (r"read this (text)", "noun"),
                (r"Gro(ups of overlap.*?with white te)xt", "something else"),
                (r"Groups of (overlapping) annotations", "verb"),
                (r"(Groups) of overlapping", "noun"),
                (r"gray with white (text)", "noun"),
            ],
        )
    else:
        annotations = []
    meta = {
        "title": doc_name,
        "md5": hashlib.md5(body.encode("utf-8")).hexdigest(),
    }
    all_docs.append(
        {
            "text": body,
            "meta": meta,
            "short_title": title,
            "long_title": long_title,
            "labels": annotations,
        }
    )
example_dir.joinpath("example_documents.json").write_text(json.dumps(all_docs))
