import subprocess
import os
from pathlib import Path
import argparse
import json
import pickle
import tempfile
import csv

from lxml import etree

import data_gen


def to_xml(xml_elems):
    xml = etree.Element("document_set")
    for xml_doc in xml_elems:
        xml.append(xml_doc)
    return etree.tostring(
        xml, encoding="utf-8", xml_declaration=True, pretty_print=True
    )


def write_csv(out_file_path, data):
    with open(out_file_path, "w", newline="", encoding="utf-8") as out_f:
        writer = csv.DictWriter(out_f, data[0].keys())
        writer.writeheader()
        for record in data:
            if "meta" in record:
                record = dict(record)
                record["meta"] = json.dumps(record["meta"])
            writer.writerow(record)


lb_command = os.environ["LABELBUDDY_COMMAND"]

parser = argparse.ArgumentParser()
parser.add_argument("-o", "--out_dir", type=str, default=None)
args = parser.parse_args()
if args.out_dir is None:
    out_dir = Path(__file__).parent.joinpath("data", "newsgroups")
else:
    out_dir = Path(args.out_dir)
out_dir.mkdir(parents=True, exist_ok=True)

data = data_gen.fetch_newsgroups()
xml_elems = []

for doc in data["xml"]:
    xml_doc = etree.Element("document")
    for key in ["text", "id", "short_title", "long_title"]:
        elem = etree.SubElement(xml_doc, key)
        elem.text = doc[key]

    xml_meta = etree.SubElement(xml_doc, "meta")
    for k, v in doc["meta"].items():
        xml_meta.set(k, v)
    xml_elems.append(xml_doc)

jsonl_lines = list(map(json.dumps, data["json"]))
txt_lines = [doc["text"] for doc in data["txt"]]

for (a, b) in [(None, None), (0, 11), (6, 15), (0, 15), (0, 300)]:
    fname = "docs" if a is None else f"docs_{a}-{b}"
    out_dir.joinpath(f"{fname}.json").write_text(json.dumps(data["json"][a:b]))
    out_dir.joinpath(f"json_data_{fname}.pkl").write_bytes(
        pickle.dumps(data["json"][a:b])
    )
    out_dir.joinpath(f"{fname}.jsonl").write_text("\n".join(jsonl_lines[a:b]))
    out_dir.joinpath(f"jsonl_data_{fname}.pkl").write_bytes(
        pickle.dumps(data["json"][a:b])
    )
    write_csv(out_dir.joinpath(f"{fname}.csv"), data["json"][a:b])
    out_dir.joinpath(f"csv_data_{fname}.pkl").write_bytes(
        pickle.dumps(data["json"][a:b])
    )
    out_dir.joinpath(f"{fname}.txt").write_text("\n".join(txt_lines[a:b]))
    out_dir.joinpath(f"txt_data_{fname}.pkl").write_bytes(
        pickle.dumps(data["txt"][a:b])
    )
    out_dir.joinpath(f"{fname}.xml").write_bytes(to_xml(xml_elems[a:b]))
    out_dir.joinpath(f"xml_data_{fname}.pkl").write_bytes(
        pickle.dumps(data["xml"][a:b])
    )


(out_dir / "labels.json").write_text(json.dumps(data["labels"]))
write_csv(out_dir / "labels.csv", data["labels"])
out_dir.joinpath("labels.txt").write_text(
    "\n".join(lab["text"] for lab in data["labels"])
)
(out_dir / "labels.jsonl").write_text(
    "\n".join([json.dumps(lab) for lab in data["labels"]])
)
xml_labels = etree.Element("label_set")
for label in data["labels"]:
    label_elem = etree.SubElement(xml_labels, "label")
    for key in ["text", "background_color", "shortcut_key"]:
        elem = etree.SubElement(label_elem, key)
        elem.text = label[key]
(out_dir / "labels.xml").write_bytes(
    etree.tostring(
        xml_labels, encoding="utf-8", xml_declaration=True, pretty_print=True
    )
)

with tempfile.TemporaryDirectory() as tmp_dir:
    tmp_dir = Path(tmp_dir)
    db = tmp_dir / "db"
    subprocess.run(
        [
            lb_command,
            db,
            "--import-labels",
            str(out_dir / "labels.json"),
            "--import-docs",
            str(out_dir / "docs_0-15.json"),
        ]
    )
    data_gen.add_random_annotations(
        db, annotation_generator=data_gen.random_non_overlapping_annotations
    )
    subprocess.run(
        [
            lb_command,
            db,
            "--export-labels",
            str(out_dir / "exported_labels_for_doccano.json"),
            "--export-docs",
            str(out_dir / "exported_docs_0-15_for_doccano.jsonl"),
        ]
    )
