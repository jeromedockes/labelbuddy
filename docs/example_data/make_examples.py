#! /usr/bin/env python3

from pathlib import Path
import json
import csv
import hashlib
import os
import subprocess

from lxml import etree


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
xml = etree.Element("document_set")
for doc in all_docs:
    doc_elem = etree.SubElement(xml, "document")
    if "meta" in doc:
        meta = etree.SubElement(doc_elem, "meta")
        for k, v in doc["meta"].items():
            meta.set(k, v)
    for k, v in doc.items():
        if k != "meta":
            elem = etree.SubElement(doc_elem, k)
            elem.text = v
xml_s = etree.tostring(
    xml, encoding="utf-8", xml_declaration=True, pretty_print=True
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
data_dir.joinpath("docs.xml").write_bytes(xml_s)
all_csv_docs = []
for doc in all_docs:
    csv_doc = dict(doc)
    if "meta" in csv_doc:
        csv_doc["id"] = csv_doc["meta"]["id"]
        del csv_doc["meta"]
    all_csv_docs.append(csv_doc)
with open(
    data_dir.joinpath("docs.csv"), "w", encoding="utf-8", newline=""
) as f:
    writer = csv.DictWriter(f, all_csv_docs[1].keys())
    writer.writeheader()
    writer.writerows(all_csv_docs)
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
xml_labels = etree.Element("label_set")
for label in all_labels:
    label_elem = etree.SubElement(xml_labels, "label")
    for key in ("text", "shortcut_key", "color"):
        if key in label:
            elem = etree.SubElement(label_elem, key)
            elem.text = label[key]
data_dir.joinpath("labels.xml").write_bytes(
    etree.tostring(
        xml_labels, encoding="utf-8", xml_declaration=True, pretty_print=True
    )
)
with open(
    data_dir.joinpath("labels.csv"), "w", encoding="utf-8", newline="\n"
) as f:
    writer = csv.DictWriter(f, all_labels[1].keys())
    writer.writeheader()
    writer.writerows(all_labels)
data_dir.joinpath("labels.txt").write_text(
    "\n".join(label["text"] for label in all_labels) + "\n"
)

lb_command = os.environ.get(
    "LABELBUDDY_COMMAND",
    str(data_dir.parents[1].joinpath("cmake_build", "labelbuddy")),
)
for doc_format in ["json", "jsonl", "xml", "csv"]:
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
for label_format in ["json", "jsonl", "xml", "csv"]:
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

schema_dir = Path(__file__).resolve().parents[1].joinpath("schema")
labels_schema = etree.RelaxNG(etree.parse(str(schema_dir / "labels.rng")))
for lfile in ["labels.xml", "exported_labels.xml"]:
    labels_schema.assertValid(etree.parse(str(data_dir.joinpath(lfile))))
docs_schema = etree.RelaxNG(etree.parse(str(schema_dir / "documents.rng")))
for dfile in ["docs.xml", "exported_docs.xml"]:
    docs_schema.assertValid(etree.parse(str(data_dir.joinpath(dfile))))
