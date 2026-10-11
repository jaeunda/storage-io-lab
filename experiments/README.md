# Reproducible storage experiments

This directory measures implemented mechanisms while checking their data results. It does not add timing assumptions to the emulator or change the device-driver source.

| File | Responsibility |
| --- | --- |
| [measure.py](measure.py) | Build, create temporary images, run workloads, verify contents, and save a JSON report |
| [driver_counts.c](driver_counts.c) | GNU ld wrappers count physical read/write/erase calls; an optional exit handler reports CLI totals |
| [hybrid_probe.c](hybrid_probe.c) | Run hot/full/mixed write sequences through the real FTL and compare complete sector contents |
| [results/latest.json](results/latest.json) | Saved results referenced by READMEs and the measurement chart |

## Run

```bash
# From the repository root:
make measure
python3 experiments/measure.py --output /tmp/storage-results.json
```

Required tools: Python 3 standard library, Make, GCC or a compatible C compiler, and GNU ld with `--wrap`. The runner uses `CC` if supplied, rebuilds the normal executables, compiles probes with `-O0 -g`, and refuses formats where native `int` is not four bytes. All workload files live in a temporary directory; the normal build outputs remain under `build/`.

The C wrappers count entries, then call the original `fdd_*` function. The hybrid probe omits validation reads from write-cost totals and excludes image initialization. The flash CLI's exit report includes its whole update command, including hidden probes. Every CLI subprocess has a 30-second timeout. Assertions stop the experiment if a stored value, slot position, file size, rejection, or demo result differs from the fixture's expectation.

The runner also reproduces record append's known overwrite defect in a separate image. Successful workload results do not establish safety for maximum-length fields, invalid LSNs, restarts, or interrupted updates. See [measurement methodology](../docs/measurements.md) and [validation notes](../docs/validation.md).
