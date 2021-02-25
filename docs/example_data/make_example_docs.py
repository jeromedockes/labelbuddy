#! /usr/bin/env python3

import hashlib
from pathlib import Path
import json
import re

example_dir = Path(__file__).parent
all_docs = []
for doc_name in [
    "hello_annotations.txt",
    "documentation.txt",
    "wikipedia_language_ar.txt",
    "wikipedia_language_el.txt",
    "wikipedia_language_zh.txt",
]:
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
    body = "\n".join(lines[1:])
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
        }
    )
example_dir.joinpath("example_documents.json").write_text(json.dumps(all_docs))
