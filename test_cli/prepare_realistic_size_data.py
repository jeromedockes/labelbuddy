import os
from pathlib import Path
import argparse
import json
import itertools
import uuid
import subprocess

from sklearn.datasets import fetch_20newsgroups

import data_gen


def gen_docs(
    all_docs,
    n_docs=100,
    approx_max_chars=10_000_000,
    single_line=False,
    prefix="",
):
    next_doc = [f"{prefix} {uuid.uuid4().hex}: "]
    n_chars = 0
    n_generated_docs = 0
    for i, doc in enumerate(itertools.cycle(all_docs)):
        print(
            f"docs: generated {n_generated_docs}; read {i}",
            end="\r",
            flush=True,
        )
        next_doc.append(doc)
        n_chars += len(doc)
        if n_chars >= approx_max_chars:
            if single_line:
                yield "".join(next_doc).replace("\n", " ")
            else:
                yield "\n\n".join(next_doc)
            n_generated_docs += 1
            next_doc = [f"{prefix} {uuid.uuid4().hex}: "]
            n_chars = 0
        if n_generated_docs == n_docs:
            print("\n")
            return


parser = argparse.ArgumentParser()
parser.add_argument(
    "-o", "--out", type=str, default="/tmp/labelbuddy_example_data"
)
parser.add_argument("-n", "--n_docs", type=int, default=50_000)
args = parser.parse_args()
out_dir = Path(args.out)
out_dir.mkdir(parents=True, exist_ok=True)

docs_file = out_dir / "docs.jsonl"

newsgroups = fetch_20newsgroups(shuffle=False, subset="all")
with open(docs_file, "w", encoding="utf-8") as f:
    for doc in itertools.chain(
        gen_docs(newsgroups["data"]),
        gen_docs(
            newsgroups["data"], approx_max_chars=100_000, single_line=True
        ),
        gen_docs(
            newsgroups["data"], n_docs=args.n_docs, approx_max_chars=3_000
        ),
    ):
        f.write(json.dumps({"text": doc}))
        f.write("\n")
out_dir.joinpath("labels.json").write_text(
    json.dumps(data_gen.get_labels(newsgroups["target_names"])),
    encoding="utf-8",
)
db = out_dir / "database.labelbuddy"
lb_command = os.environ.get("LABELBUDDY_COMMAND", "labelbuddy")
subprocess.run(
    [
        lb_command,
        str(db),
        "--import-labels",
        str(out_dir / "labels.json"),
        "--import-docs",
        str(docs_file),
    ]
)
data_gen.add_random_annotations(db, n_anno=100, max_docs=None)
