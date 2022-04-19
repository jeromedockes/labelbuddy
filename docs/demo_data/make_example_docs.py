#! /usr/bin/env python3

import hashlib
from pathlib import Path
import json
import re


def get_annotations(doc, annotations):
    result = []
    for pattern, label, *extra_data in annotations:
        match = re.search(pattern, doc)
        if match is not None:
            start = match.start(1)
            end = match.end(1)
            if extra_data:
                result.append(
                    {
                        "start_char": start,
                        "end_char": end,
                        "label_name": label,
                        "extra_data": extra_data[0],
                    }
                )
            else:
                result.append(
                    {"start_char": start, "end_char": end, "label_name": label}
                )
    return result


example_dir = Path(__file__).parent
doc_file_names = [
    "hello_annotations.txt",
    "wikipedia_language_en.txt",
    "wikipedia_language_ar.txt",
    "wikipedia_language_el.txt",
    "wikipedia_language_zh.txt",
]

demo_docs = []
wiki_docs = []

for doc_name in doc_file_names:
    doc = example_dir.joinpath(doc_name).read_text(encoding="utf-8")
    lines = doc.split("\n")
    title = lines[0].strip()
    list_title = re.sub(r"<a[^>]*>([^<]*)</a>", r"\1", title)
    body = "\n".join(lines[1:])
    if doc_name == "hello_annotations.txt":
        annotations = get_annotations(
            body,
            [
                (r"(You) don't", "Word", "(optional) free-form annotation"),
                (r"(annotating) it instead", "Mot"),
                (
                    r"read this (text)",
                    "Palavra",
                    "https://en.wiktionary.org/wiki/text",
                ),
                (r"Gro(ups of overlap.*?with white te)xt", "In progress"),
                (
                    r"Groups of (overlapping) annotations",
                    "\u0643\u0644\u0645\u0629",
                ),
                (r"(Groups) of overlapping", "Word"),
                (r"gray with white (text)", "\u8a5e"),
            ],
        )
    else:
        annotations = []
    metadata = {
        "title": doc_name,
        "md5": hashlib.md5(body.encode("utf-8")).hexdigest(),
    }
    if doc_name != "wikipedia_language_en.txt":
        demo_docs.append(
            {
                "text": body,
                "metadata": metadata,
                "display_title": title,
                "list_title": list_title,
                "annotations": annotations,
            }
        )
    if doc_name != "hello_annotations.txt":
        wiki_docs.append(
            {
                "text": body,
                "metadata": {"source": "https://en.wikipedia.org"},
                "display_title": title,
                "list_title": list_title,
            }
        )

example_dir.joinpath("example_documents.json").write_text(
    json.dumps(demo_docs)
)
example_dir.joinpath("wiki_extracts_documents.json").write_text(
    json.dumps(wiki_docs)
)
