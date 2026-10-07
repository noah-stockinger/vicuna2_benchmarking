#!/usr/bin/env python3
"""
Collect maxpool_muriscv vs maxpool_redmax results from a ctest log into CSV files.

Reads build_programs/build/Testing/Temporary/LastTest.log. The layer shape of each test is
taken from the line benchmark.hpp prints ("Max pool s8 (...): N=.. H=.. W=.. C=.. ..."), so
both the CMSIS-NN sets and the shape sweep are covered. Writes to results/maxpool_<core>_<date>/:

  raw.csv       one row per test and variant (cycles, instructions, pass/fail, shape)
  compare.csv   one row per test: cycles of both variants, redmax/muriscv ratio, winner
  meta.txt      commits and tool versions the numbers belong to
  LastTest.log  copy of the ctest log

Usage (inside the container, after `ctest -R maxpool_`):
  python3 scripts/collect_maxpool.py [--log PATH] [--out DIR]
"""
import argparse
import csv
import datetime
import re
import shutil
from pathlib import Path

from collect_redminmax_micro import git, repo_state, tool_version, ROOT, DEFAULT_LOG

NAME = re.compile(r"^\d+/\d+ Test: maxpool_(muriscv|redmax)_(\d+)_Verilator$")
SHAPE = re.compile(r"N=(\d+) H=(\d+) W=(\d+) C=(\d+) filter (\d+)x(\d+) stride (\d+)x(\d+) pad (\d+)x(\d+) -> (\d+)x(\d+)")
SHAPE_KEYS = ["n", "h", "w", "c", "filter_h", "filter_w", "stride_h", "stride_w", "pad_h", "pad_w", "out_h", "out_w"]


def parse_log(path):
    results, cur = {}, None
    for line in Path(path).read_text(errors="replace").splitlines():
        if m := NAME.match(line):
            cur = (int(m.group(2)), m.group(1))
            results[cur] = {"cycles": "", "instructions": "", "status": "unknown"}
        elif cur is not None:
            r = results[cur]
            if m := re.search(r"Total Cycles:\s+(\d+)", line):
                r["cycles"] = int(m.group(1))
            elif m := re.search(r"Total Instructions:\s+(\d+)", line):
                r["instructions"] = int(m.group(1))
            elif m := SHAPE.search(line):
                r.update(zip(SHAPE_KEYS, (int(g) for g in m.groups())))
            elif line.startswith("Test Passed"):
                r["status"] = "passed"
            elif line.startswith("Test Failed"):
                r["status"] = "failed"
    return results


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--log", default=DEFAULT_LOG)
    ap.add_argument("--out")
    args = ap.parse_args()

    core = git("rev-parse", "--short", "HEAD", cwd=ROOT / "rtl" / "vicuna2_core")
    out = Path(args.out) if args.out else ROOT / "results" / f"maxpool_{core}_{datetime.date.today()}"
    out.mkdir(parents=True, exist_ok=True)

    log = parse_log(args.log)
    raw = [{"test": t, "variant": v, **r} for (t, v), r in sorted(log.items())]
    fields = ["test", "variant", "status", "cycles", "instructions"] + SHAPE_KEYS
    with open(out / "raw.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields, extrasaction="ignore", restval="")
        w.writeheader()
        w.writerows(raw)

    compare = []
    for t in sorted({t for t, _ in log}):
        m, r = log.get((t, "muriscv")), log.get((t, "redmax"))
        ok = m and r and m["status"] == r["status"] == "passed"
        shape = m or r
        compare.append({
            "test": t, "set": "cmsis-nn" if t < 100 else "sweep",
            **{k: shape.get(k, "") for k in ("c", "h", "w", "filter_h", "filter_w", "out_h", "out_w")},
            "cycles_muriscv": m["cycles"] if m else "", "cycles_redmax": r["cycles"] if r else "",
            "ratio_redmax_muriscv": round(r["cycles"] / m["cycles"], 3) if ok else "",
            "winner": ("redmax" if r["cycles"] < m["cycles"] else "muriscv") if ok else "",
            "valid": bool(ok),
        })
    with open(out / "compare.csv", "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(compare[0].keys()))
        w.writeheader()
        w.writerows(compare)

    shutil.copy(args.log, out / "LastTest.log")
    (out / "meta.txt").write_text(
        f"date: {datetime.datetime.now().isoformat(timespec='seconds')}\n"
        f"vicuna2_benchmarking: {repo_state()}\n"
        f"vicuna2_core: {core}\n"
        f"verilator: {tool_version([str(ROOT / 'toolchain/verilator/bin/verilator'), '--version'])}\n"
        f"gcc: {tool_version([str(ROOT / 'toolchain/GCC/multilib/bin/riscv32-unknown-elf-gcc'), '--version'])}\n"
    )
    n_pass = sum(r["status"] == "passed" for r in raw)
    print(f"{n_pass}/{len(raw)} tests passed, {sum(c['valid'] for c in compare)}/{len(compare)} "
          f"comparisons valid -> {out.relative_to(ROOT)}")


if __name__ == "__main__":
    main()
