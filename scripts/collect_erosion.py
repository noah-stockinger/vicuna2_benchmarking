#!/usr/bin/env python3
"""
Collect erosion_redmin results from a ctest log into CSV files.

Joins build_programs/build/Testing/Temporary/LastTest.log with
programs/erosion_redmin/test_data/INDEX.csv and writes to results/erosion_<core>_<date>/:

  results.csv   one row per test: shape, cycles, instructions, pass/fail, plus
                cycles per output pixel and per vredminu (one per clipped window row)
  meta.txt      commits and tool versions the numbers belong to
  LastTest.log  copy of the ctest log

Usage (inside the container, after `ctest -R erosion_redmin`):
  python3 scripts/collect_erosion.py [--log PATH] [--out DIR]
"""
import argparse
import csv
import datetime
import re
import shutil
from pathlib import Path

from collect_redminmax_micro import git, tool_version, ROOT, DEFAULT_LOG

INDEX = ROOT / "benchmark_sources" / "generic_cpp" / "programs" / "erosion_redmin" / "test_data" / "INDEX.csv"
NAME = re.compile(r"^\d+/\d+ Test: erosion_redmin_(\d+)_Verilator$")


def parse_log(path):
    results, cur = {}, None
    for line in Path(path).read_text(errors="replace").splitlines():
        if m := NAME.match(line):
            cur = int(m.group(1))
            results[cur] = {"cycles": "", "instructions": "", "status": "unknown"}
        elif cur is not None:
            if m := re.search(r"Total Cycles:\s+(\d+)", line):
                results[cur]["cycles"] = int(m.group(1))
            elif m := re.search(r"Total Instructions:\s+(\d+)", line):
                results[cur]["instructions"] = int(m.group(1))
            elif line.startswith("Test Passed"):
                results[cur]["status"] = "passed"
            elif line.startswith("Test Failed"):
                results[cur]["status"] = "failed"
    return results


def reductions(h, w, k):
    """Number of vredminu.vs executed: one per output pixel and clipped window row."""
    r = k // 2
    return w * sum(min(h, y + r + 1) - max(0, y - r) for y in range(h))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", default=DEFAULT_LOG)
    ap.add_argument("--out")
    args = ap.parse_args()

    core = git("rev-parse", "--short", "HEAD", cwd=ROOT / "rtl" / "vicuna2_core")
    out = Path(args.out) if args.out else ROOT / "results" / f"erosion_{core}_{datetime.date.today()}"
    out.mkdir(parents=True, exist_ok=True)

    log = parse_log(args.log)
    rows = []
    with open(INDEX) as f:
        for row in csv.DictReader(f):
            r = log.get(int(row["test"]), {"cycles": "", "instructions": "", "status": "not run"})
            h, w, k = int(row["height"]), int(row["width"]), int(row["k"])
            ok = r["status"] == "passed" and r["cycles"] != ""
            n_red = reductions(h, w, k)
            rows.append({**row, **r, "pixels": h * w, "reductions": n_red,
                         "cycles_per_pixel": round(r["cycles"] / (h * w), 2) if ok else "",
                         "cycles_per_reduction": round(r["cycles"] / n_red, 2) if ok else ""})
    with open(out / "results.csv", "w", newline="") as f:
        wr = csv.DictWriter(f, fieldnames=list(rows[0].keys()))
        wr.writeheader()
        wr.writerows(rows)

    shutil.copy(args.log, out / "LastTest.log")
    (out / "meta.txt").write_text(
        f"date: {datetime.datetime.now().isoformat(timespec='seconds')}\n"
        f"vicuna2_benchmarking: {git('rev-parse', '--short', 'HEAD')}"
        f"{' (dirty)' if git('status', '--porcelain', '--untracked-files=no') else ''}\n"
        f"vicuna2_core: {core}\n"
        f"verilator: {tool_version([str(ROOT / 'toolchain/verilator/bin/verilator'), '--version'])}\n"
        f"gcc: {tool_version([str(ROOT / 'toolchain/GCC/multilib/bin/riscv32-unknown-elf-gcc'), '--version'])}\n"
    )
    n_pass = sum(r["status"] == "passed" for r in rows)
    print(f"{n_pass}/{len(rows)} tests passed -> {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
