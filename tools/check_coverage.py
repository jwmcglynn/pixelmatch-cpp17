#!/usr/bin/env python3
"""Fail unless every instrumented library line, region, function and branch is covered."""
import json
from pathlib import Path
import sys

report = json.loads(Path(sys.argv[1]).read_text())
source = Path(__file__).resolve().parents[1] / "src" / "pixelmatch"
expected = {path.name for path in source.glob("*.cc")} | {"pixelmatch.h"}
files = {}
for entry in report["data"]:
    for item in entry["files"]:
        path = Path(item["filename"]).resolve()
        if path.parent == source:
            if path.name in files:
                sys.exit(f"Duplicate coverage record: {path.name}")
            files[path.name] = item["summary"]
if set(files) != expected:
    sys.exit(f"Coverage file set mismatch: expected {sorted(expected)}, got {sorted(files)}")
failed = False
for name, summary in sorted(files.items()):
    for metric in ("lines", "regions", "functions", "branches"):
        count = summary[metric]["count"]
        covered = summary[metric]["covered"]
        print(f"{name}: {metric} {covered}/{count}")
        if count != covered or (metric != "branches" and count == 0):
            failed = True
sys.exit(1 if failed else 0)
