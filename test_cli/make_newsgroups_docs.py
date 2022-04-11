import subprocess
import os
from pathlib import Path
import argparse
import json
import pickle
import tempfile
import data_gen

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
    out_dir.joinpath(f"{fname}.txt").write_text("\n".join(txt_lines[a:b]))
    out_dir.joinpath(f"txt_data_{fname}.pkl").write_bytes(
        pickle.dumps(data["txt"][a:b])
    )


(out_dir / "labels.json").write_text(json.dumps(data["labels"]))
out_dir.joinpath("labels.txt").write_text(
    "\n".join(lab["text"] for lab in data["labels"])
)
(out_dir / "labels.jsonl").write_text(
    "\n".join([json.dumps(lab) for lab in data["labels"]])
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
