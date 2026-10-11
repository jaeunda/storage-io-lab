# Saved measurement output

[latest.json](latest.json) is the complete report produced by `python3 experiments/measure.py`. It includes the UTC timestamp, platform/compiler details, both reported and instrumented flash counts, hybrid workload counts and verification totals, record sizes and slot checks, and the demo output.

`make measure` replaces this file after a successful run. To compare a new run while retaining this reference, pass `--output /tmp/storage-results.json`. Counts are deterministic for the documented geometry and sequence; timestamps and compiler metadata can differ.

The chart generator reads the `hybrid` section directly. If workloads or geometry change, update [../../docs/measurements.md](../../docs/measurements.md), the project tables, and the SVGs together. This JSON is an emulator experiment report, not a benchmark of physical storage latency.
