#! /usr/bin/env python3

from pathlib import Path
import json
import hashlib
import os
import subprocess

all_docs = [
    {
        "text": "the text of document 1\nsome text\nthe end\n",
    },
    {
        "text": "the text of document 2\nmore text\nthe end\n",
        "short_title": "title 2",
        "long_title": "the title of document 2",
        "meta": {"id": "doc-2", "source": "example.org"},
    },
]
all_annotations = []
all_annotations.append(
    {
        "utf8_text_md5_checksum": hashlib.md5(
            all_docs[0]["text"].encode("utf-8")
        ).hexdigest(),
        "labels": [[4, 8, "Word"], [21, 22, "Number", "1"]],
    }
)
all_annotations.append(
    {
        "utf8_text_md5_checksum": hashlib.md5(
            all_docs[1]["text"].encode("utf-8")
        ).hexdigest(),
        "labels": [[12, 20, "Word"]],
    }
)


data_dir = Path(__file__).resolve().parent

data_dir.joinpath("annotations.json").write_text(
    json.dumps(all_annotations), encoding="utf-8"
)
data_dir.joinpath("docs.json").write_text(
    json.dumps(all_docs, indent=2) + "\n", encoding="utf-8"
)
data_dir.joinpath("docs.jsonl").write_text(
    "\n".join(json.dumps(doc) for doc in all_docs) + "\n", encoding="utf-8"
)

data_dir.joinpath("docs.txt").write_text(
    "\n".join(doc["text"].replace("\n", r" ") for doc in all_docs) + "\n"
)

all_labels = [
    {"text": "Word"},
    {"text": "Number", "shortcut_key": "n", "color": "orange"},
]
data_dir.joinpath("labels.json").write_text(
    json.dumps(all_labels, indent=2) + "\n", encoding="utf-8"
)
data_dir.joinpath("labels.jsonl").write_text(
    "\n".join(json.dumps(label) for label in all_labels) + "\n",
    encoding="utf-8",
)
data_dir.joinpath("labels.txt").write_text(
    "\n".join(label["text"] for label in all_labels) + "\n"
)


lb_command = os.environ.get(
    "LABELBUDDY_COMMAND",
    str(data_dir.parents[3].joinpath("cmake_release_build", "labelbuddy")),
)
for doc_format in ["json", "jsonl"]:
    subprocess.run(
        [
            lb_command,
            ":memory:",
            "--import-docs",
            str(data_dir.joinpath(f"docs.{doc_format}")),
            "--import-docs",
            str(data_dir.joinpath("annotations.json")),
            "--export-docs",
            str(data_dir.joinpath(f"exported_docs.{doc_format}")),
        ]
    )
for label_format in ["json", "jsonl"]:
    subprocess.run(
        [
            lb_command,
            ":memory:",
            "--import-labels",
            str(data_dir.joinpath(f"labels.{label_format}")),
            "--export-labels",
            str(data_dir.joinpath(f"exported_labels.{label_format}")),
        ]
    )
