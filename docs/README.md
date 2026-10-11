# Documentation

The main README introduces the three implemented storage mechanisms. Each project README explains its data format, functions, commands, measured behavior, and remaining limits; each `src/README.md` maps those operations to source files.

| Document | Purpose |
| --- | --- |
| [measurements.md](measurements.md) | Exact workloads, driver-call accounting, record sizes, and reproduction |
| [validation.md](validation.md) | Build and execution checks, preserved limitations, and scope of the changes |
| [assets/README.md](assets/README.md) | SVG design, diagram inventory, accessibility, and font licensing |
| [../experiments/README.md](../experiments/README.md) | Measurement runner and C instrumentation |
| [../tools/README.md](../tools/README.md) | How to regenerate the figures |

## Name and GitHub description

Recommended repository name: **`storage-io-lab`**. It covers both flash storage and structured file records, and fits the descriptive naming of `linux-filesystem-tools` and `xv6-kernel-extensions`. The README uses **Storage I/O Lab** as its display title.

Recommended description:

```text
C storage experiments with file-backed flash I/O, hybrid FTL mapping, and fixed-slot record reuse, with reproducible operation-count measurements.
```

An alternative, **`flash-record-io`**, names the two storage domains directly but gives less emphasis to the FTL experiment. These are suggestions; the current GitHub repository name and description are retained.

## Editorial reference

The documentation follows the current [compiler](https://github.com/jaeunda/compiler), [linux-filesystem-tools](https://github.com/jaeunda/linux-filesystem-tools), and [xv6-kernel-extensions](https://github.com/jaeunda/xv6-kernel-extensions) READMEs: begin with the implementation problem, explain the mechanism through diagrams, give reproducible commands, and distinguish measured results from known limits. Their GitHub READMEs were checked against the local reference copies on 2026-10-12 KST.

No original reports were supplied for this repository. Descriptions and figures come from the checked-in C implementation and the new disposable measurement fixtures.
