#! /usr/bin/env python3

import csv
from pathlib import Path

data_dir = Path(__file__).parent
for file_path in data_dir.glob("*-in.csv"):
    name = file_path.name.split("-")[0]
    print(name)
    out_file = data_dir / f"{name}-out.csv"
    rows = []
    with open(file_path, "r", encoding="utf-8-sig", newline="") as fh:
        reader = csv.reader(fh, strict=True)
        try:
            for r in reader:
                rows.append(r)
        except csv.Error:
            pass
    rows[0].insert(0, "ignore_this_column")
    for r in rows[1:]:
        r.insert(0, "")
    with open(out_file, "w", encoding="utf-8-sig", newline="") as fh:
        writer = csv.writer(fh)
        writer.writerows(rows)
