#!/usr/bin/env python3
"""Measure driver calls and record allocation in disposable files (Linux/GNU ld)."""
import argparse
from datetime import datetime, timezone
import json
import os
from pathlib import Path
import platform
import re
import shlex
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
PAGE, FLASH_BLOCK, RECORD = 528, 4224, 85


def run(args, **kwargs):
    result = subprocess.run([str(arg) for arg in args], capture_output=True,
                            text=True, timeout=30, **kwargs)
    if result.returncode:
        raise RuntimeError(f"{args}: exit {result.returncode}\n{result.stderr}")
    return result


def header(path):
    return struct.unpack("=ii", path.read_bytes()[:8])


def slots(path):
    data = path.read_bytes()[8:]
    assert len(data) % RECORD == 0
    return [data[i:i + RECORD] for i in range(0, len(data), RECORD)]


def measure_flash(binary, directory):
    results = []
    for occupied in (1, 4, 8):
        image = directory / f"flash-{occupied}.bin"
        run([binary, "c", image, 2])
        assert image.read_bytes() == b"\xff" * (2 * FLASH_BLOCK)
        for ppn in range(occupied):
            run([binary, "w", image, ppn, f"page-{ppn}", ppn + 100])
        before = image.read_bytes()
        update = run([binary, "u", image, 0, "updated", 999])
        printed = re.search(r"#reads=(\d+) #writes=(\d+) #erases=(\d+)", update.stdout)
        counted = re.search(r"DRIVER reads=(\d+) writes=(\d+) erases=(\d+)", update.stderr)
        assert printed and counted
        after = image.read_bytes()
        assert after[:7] == b"updated"
        assert struct.unpack("=i", after[512:516])[0] == 999
        assert after[PAGE:FLASH_BLOCK] == before[PAGE:FLASH_BLOCK]
        assert after[FLASH_BLOCK:] == b"\xff" * FLASH_BLOCK
        assert image.stat().st_size == 2 * FLASH_BLOCK
        assert run([binary, "r", image, 0]).stdout.split(" ")[0].startswith("updated")
        for bad in ([binary, "w", image, 0, "bad", 1],
                    [binary, "r", image, 16], [binary, "e", image, 2]):
            rejected = subprocess.run([str(x) for x in bad], capture_output=True, timeout=30)
            assert rejected.returncode != 0
        run([binary, "e", image, 0])
        assert image.read_bytes() == b"\xff" * (2 * FLASH_BLOCK)
        results.append({
            "occupied_pages": occupied,
            "reported": dict(zip(("reads", "writes", "erases"), map(int, printed.groups()))),
            "driver_calls": dict(zip(("reads", "writes", "erases"), map(int, counted.groups()))),
            "verified_preserved_pages": occupied - 1,
            "image_bytes": len(after),
        })
    return results


def measure_records(binary, directory):
    image = directory / "records.dat"
    for i in range(1000):
        run([binary, "-a", image, f"{i:08d}", f"S{i}",
             "DROP" if i % 4 == 0 else "KEEP", "Seoul", "s@example.com"])
    assert header(image) == (1000, -1)
    assert image.stat().st_size == 85008
    original = slots(image)
    found = run([binary, "-s", image, "DEPT=DROP"]).stdout
    assert found.splitlines()[0] == "#records = 250"
    assert len(found.splitlines()) == 251
    run([binary, "-d", image, "DEPT=DROP"])
    assert header(image) == (750, 996)
    deleted = slots(image)
    assert sum(row.startswith(b"*") for row in deleted) == 250
    size_deleted = image.stat().st_size
    for i in range(250):
        run([binary, "-i", image, f"{1000 + i:08d}", f"R{i}",
             "NEW", "Seoul", "r@example.com"])
    assert header(image) == (1000, -1)
    final = slots(image)
    for i in range(1000):
        if i % 4:
            assert final[i] == original[i]
        else:
            expected_id = f"{1000 + (996 - i) // 4:08d}".encode()
            assert final[i].startswith(expected_id + b"#")
    assert image.stat().st_size == 85008
    assert run([binary, "-s", image, "DEPT=NEW"]).stdout.splitlines()[0] == "#records = 250"

    # Reproduce the known append-after-delete defect without changing the source.
    defect = directory / "append-after-delete.dat"
    for i in range(3):
        run([binary, "-a", defect, f"{i:08d}", f"S{i}", "CS", "Seoul", "s@example.com"])
    run([binary, "-d", defect, "SID=00000000"])
    run([binary, "-a", defect, "00000003", "S3", "CS", "Seoul", "s@example.com"])
    defect_rows = slots(defect)
    assert defect_rows[0].startswith(b"*")
    assert defect_rows[1].startswith(b"00000001#")
    assert defect_rows[2].startswith(b"00000003#")
    assert b"00000002#" not in defect.read_bytes()
    return {"initial_records": 1000, "deleted_records": 250, "inserted_records": 250,
            "bytes_before": 85008, "bytes_after_delete": size_deleted,
            "bytes_after_insert": image.stat().st_size,
            "reused_slots": 250, "unchanged_survivors": 750,
            "append_after_delete_overwrites_live_slot": True}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=ROOT / "experiments/results/latest.json")
    args = parser.parse_args()
    compiler = shlex.split(os.environ.get("CC", "gcc"))
    run(["make", "-B", "-C", ROOT, "CC=" + os.environ.get("CC", "gcc"),
         "CFLAGS=-Wall -Wextra -O0 -g"])
    result = {
        "measured_at_utc": datetime.now(timezone.utc).isoformat(timespec="seconds"),
        "environment": {"kernel": platform.release(), "architecture": platform.machine(),
                        "compiler": run(compiler + ["--version"]).stdout.splitlines()[0],
                        "python": platform.python_version(), "int_bytes": struct.calcsize("@i"),
                        "byteorder": __import__("sys").byteorder},
        "method": "GNU ld --wrap; -O0; initialization and verification reads excluded from hybrid write costs",
    }
    assert struct.calcsize("@i") == 4, "The legacy file formats require 4-byte int"
    wrap = ["-Wl,--wrap=fdd_read", "-Wl,--wrap=fdd_write", "-Wl,--wrap=fdd_erase"]
    with tempfile.TemporaryDirectory(prefix="storage-measure-") as temp:
        directory = Path(temp)
        flash = directory / "flash-probe"
        hybrid = directory / "hybrid-probe"
        run(compiler + ["-O0", "-g", "-DREPORT_AT_EXIT", "-o", flash,
                        ROOT / "01-flash-io/src/ftlmgr.c", ROOT / "01-flash-io/src/fdevicedriver.c",
                        ROOT / "experiments/driver_counts.c"] + wrap)
        run(compiler + ["-O0", "-g", "-I", ROOT / "02-hybrid-ftl/src", "-o", hybrid,
                        ROOT / "experiments/hybrid_probe.c", ROOT / "02-hybrid-ftl/src/ftlmgr.c",
                        ROOT / "02-hybrid-ftl/src/fdevicedriver.c", ROOT / "experiments/driver_counts.c"] + wrap)
        result["flash_update"] = measure_flash(flash, directory)
        result["hybrid"] = {mode: json.loads(run([hybrid, mode]).stdout)
                            for mode in ("hot", "full", "mixed")}
        for data in result["hybrid"].values():
            data["write_amplification"] = data["writes"] / data["logical_writes"]
        result["records"] = measure_records(ROOT / "build/record-store", directory)
        demo = run([ROOT / "build/hybrid-ftl"], cwd=directory)
        assert "LSN 8: A2" in demo.stdout
        result["hybrid_demo"] = demo.stdout.splitlines()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
