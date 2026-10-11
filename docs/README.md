# Documentation

The main README presents measured results first, then explains the three storage mechanisms. Each project README covers its data format, functions, commands, measured behavior, and remaining limits; each `src/README.md` maps those operations to source files.

| Document | Purpose |
| --- | --- |
| [measurements.md](measurements.md) | Exact workloads, driver-call accounting, record sizes, and reproduction |
| [validation.md](validation.md) | Build and execution checks, source-derived limits, and verified behavior |
| [assets/README.md](assets/README.md) | SVG design, diagram inventory, accessibility, and font licensing |
| [../experiments/README.md](../experiments/README.md) | Measurement runner and C instrumentation |
| [../tools/README.md](../tools/README.md) | How to regenerate the figures |

Descriptions and figures document the checked-in C implementation and the reproducible measurement fixtures. The measurement report records the environment, counting method, workload sequence, and complete verification results.
