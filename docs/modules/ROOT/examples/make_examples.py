#! /usr/bin/env python3

from pathlib import Path
import json
import hashlib
import os
import subprocess

all_docs = [
    {
        "text": "😀 the text of document 1\nsome text\nthe end\n",
    },
    {
        "text": "the text of document 2\nmore text\nthe end\n",
        "display_title": "title 2",
        "list_title": "the title of document 2",
        "metadata": {"id": "doc-2", "source": "example.org"},
    },
]
all_annotations = []
all_annotations.append(
    {
        "utf8_text_md5_checksum": hashlib.md5(
            all_docs[0]["text"].encode("utf-8")
        ).hexdigest(),
        "annotations": [
            {"start_char": 6, "end_char": 10, "label_name": "Word"},
            {
                "start_char": 23,
                "end_char": 24,
                "label_name": "Number",
                "extra_data": "1",
            },
        ],
    }
)
all_annotations.append(
    {
        "utf8_text_md5_checksum": hashlib.md5(
            all_docs[1]["text"].encode("utf-8")
        ).hexdigest(),
        "annotations": [
            {"start_char": 12, "end_char": 20, "label_name": "Word"}
        ],
    }
)


data_dir = Path(__file__).resolve().parent

data_dir.joinpath("annotations.json").write_text(
    json.dumps(all_annotations, ensure_ascii=False), encoding="utf-8"
)
data_dir.joinpath("docs.json").write_text(
    json.dumps(all_docs, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
)
data_dir.joinpath("docs.jsonl").write_text(
    "\n".join(json.dumps(doc, ensure_ascii=False) for doc in all_docs) + "\n",
    encoding="utf-8",
)

data_dir.joinpath("docs.txt").write_text(
    "\n".join(doc["text"].rstrip("\n").replace("\n", r" ") for doc in all_docs)
    + "\n"
)

all_labels = [
    {"name": "Word"},
    {"name": "Number", "shortcut_key": "n", "color": "orange"},
]
data_dir.joinpath("labels.json").write_text(
    json.dumps(all_labels, ensure_ascii=False, indent=2) + "\n",
    encoding="utf-8",
)
data_dir.joinpath("labels.jsonl").write_text(
    "\n".join(json.dumps(label, ensure_ascii=False) for label in all_labels)
    + "\n",
    encoding="utf-8",
)
data_dir.joinpath("labels.txt").write_text(
    "\n".join(label["name"] for label in all_labels) + "\n"
)


lb_command = os.environ.get(
    "LABELBUDDY_COMMAND",
    str(data_dir.parents[4].joinpath("build", "cmake_release", "labelbuddy")),
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
