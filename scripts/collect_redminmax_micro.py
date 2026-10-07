#!/usr/bin/env python3
"""
Collect redminmax_micro results from a ctest log into CSV files.

Reads the per-test output ctest keeps in build_programs/build/Testing/Temporary/LastTest.log
("Total Cycles" / "Total Instructions" from framework/verilator/simulator.hpp), joins it with
programs/redminmax_micro/test_data/INDEX.csv and writes to results/redminmax_micro_<core>_<date>/:

  raw.csv          one row per test (cycles, instructions, pass/fail)
  per_setting.csv  one row per setting: cycles per reduction = (C(272) - C(16)) / 256
  meta.txt         commits and tool versions the numbers belong to
  LastTest.log     copy of the ctest log

Usage (inside the container, after `ctest -R redminmax_micro`):
  python3 scripts/collect_redminmax_micro.py [--log PATH] [--out DIR]
"""
import argparse
import csv
import datetime
import re
import shutil
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
INDEX = ROOT / "benchmark_sources" / "generic_cpp" / "programs" / "redminmax_micro" / "test_data" / "INDEX.csv"
DEFAULT_LOG = ROOT / "build_programs" / "build" / "Testing" / "Temporary" / "LastTest.log"
NAME = re.compile(r"^\d+/\d+ Test: redminmax_micro_(\d+)_Verilator$")


def git(*args, cwd=ROOT):
    try:
        return subprocess.run(["git", *args], cwd=cwd, capture_output=True, text=True, check=True).stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def tool_version(cmd):
    try:
        return subprocess.run(cmd, capture_output=True, text=True).stdout.splitlines()[0]
    except (OSError, IndexError):
        return "unknown"


def parse_log(path):
    results, current = {}, None
    for line in Path(path).read_text(errors="replace").splitlines():
        m = NAME.match(line)
        if m:
            current = int(m.group(1))
            results[current] = {"cycles": "", "instructions": "", "status": "unknown"}
        elif current is not None:
            if m := re.search(r"Total Cycles:\s+(\d+)", line):
                results[current]["cycles"] = int(m.group(1))
            elif m := re.search(r"Total Instructions:\s+(\d+)", line):
                results[current]["instructions"] = int(m.group(1))
            elif line.startswith("Test Passed"):
                results[current]["status"] = "passed"
            elif line.startswith("Test Failed"):
                results[current]["status"] = "failed"
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", default=DEFAULT_LOG)
    ap.add_argument("--out")
    args = ap.parse_args()

    core = git("rev-parse", "--short", "HEAD", cwd=ROOT / "rtl" / "vicuna2_core")
    out = Path(args.out) if args.out else ROOT / "results" / f"redminmax_micro_{core}_{datetime.date.today()}"
    out.mkdir(parents=True, exist_ok=True)

    with open(INDEX) as f:
        index = list(csv.DictReader(f))
    log = parse_log(args.log)

    raw, missing = [], []
    for row in index:
        r = log.get(int(row["test"]))
        if r is None:
            missing.append(row["test"])
            r = {"cycles": "", "instructions": "", "status": "not run"}
        raw.append({**row, **r})

    with open(out / "raw.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(raw[0].keys()))
        w.writeheader()
        w.writerows(raw)

    settings = {}
    for r in raw:
        settings.setdefault(r["setting"], {})[int(r["reps"])] = r
    per_setting = []
    for sid, by_reps in settings.items():
        lo, hi = sorted(by_reps)
        a, b = by_reps[lo], by_reps[hi]
        ok = a["status"] == b["status"] == "passed" and a["cycles"] != "" and b["cycles"] != ""
        per_setting.append({
            "setting": sid, "experiment": a["experiment"], "op": a["op"], "sew": a["sew"],
            "lmul": a["lmul"], "vl": a["vl"],
            "chunks": -(-int(a["vl"]) * int(a["sew"]) // 32),   # ceil(vl*SEW/32), OP_W = 32
            "cycles_lo": a["cycles"], "cycles_hi": b["cycles"],
            "cycles_per_red": (b["cycles"] - a["cycles"]) / (hi - lo) if ok else "",
            "instr_per_red": (b["instructions"] - a["instructions"]) / (hi - lo) if ok else "",
            "valid": ok,
        })
    with open(out / "per_setting.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(per_setting[0].keys()))
        w.writeheader()
        w.writerows(per_setting)

    shutil.copy(args.log, out / "LastTest.log")
    (out / "meta.txt").write_text(
        f"date: {datetime.datetime.now().isoformat(timespec='seconds')}\n"
        f"vicuna2_benchmarking: {git('rev-parse', '--short', 'HEAD')}"
        f"{' (dirty)' if git('status', '--porcelain', '--untracked-files=no') else ''}\n"
        f"vicuna2_core: {core}\n"
        f"verilator: {tool_version([str(ROOT / 'toolchain/verilator/bin/verilator'), '--version'])}\n"
        f"gcc: {tool_version([str(ROOT / 'toolchain/GCC/multilib/bin/riscv32-unknown-elf-gcc'), '--version'])}\n"
    )

    n_pass = sum(r["status"] == "passed" for r in raw)
    print(f"{n_pass}/{len(raw)} tests passed, {sum(p['valid'] for p in per_setting)}/{len(per_setting)} "
          f"settings valid -> {out.relative_to(ROOT)}")
    if missing:
        print(f"WARNING: {len(missing)} tests not in log, e.g. {missing[:5]}")


if __name__ == "__main__":
    main()
