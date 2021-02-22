#! /usr/bin/env python3

import hashlib
from pathlib import Path
import json

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
    meta = {
        "title": doc_name,
        "md5": hashlib.md5(doc.encode("utf-8")).hexdigest(),
    }
    all_docs.append([doc, meta])
example_dir.joinpath("example_documents.json").write_text(json.dumps(all_docs))
