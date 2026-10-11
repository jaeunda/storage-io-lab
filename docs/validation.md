# Implementation and documentation validation

Checked on **2026-10-12 KST**, on the environment recorded in [measurements.md](measurements.md). Descriptions and diagrams were checked against the C source rather than treating the prior README as a specification.

## Changes to the repository

| Previous path | Current path | Purpose |
| --- | --- | --- |
| `flash-io/` | `01-flash-io/` | Physical page/block operations |
| `hybrid-mapping/` | `02-hybrid-ftl/` | Logical mapping, append, and merge |
| `record-io/` | `03-record-store/` | Structured records and slot reuse |
| Per-project `a.out` | Root `build/flash-io`, `build/hybrid-ftl`, `build/record-store` | Named, ignored build artifacts |

The manager algorithms, supplied device drivers, headers, and record implementation retain their original source. The hybrid demonstration was updated to pass the complete 512-byte buffers required by its API and assert a latest-version read. Makefiles now track header dependencies, enable warnings, and remove only the executable on `clean`; the old record clean rule also removed `*.dat`.

Added documentation covers all project and source directories, measurement fixtures and results, figure design, and regeneration. Root `build/` is generated output, so it is ignored and has no tracked README. GitHub repository rename/description changes are recommendations only.

## Build and execution checks

`make -j3` built all programs using GCC 9.5. The original record source emits warnings for unhandled enum cases, an unused variable, `void main`, and a missing default return in `getFieldID`. Those warnings match the limitations documented in its README. No warning was treated as evidence of correct runtime behavior.

| Check | Verified result |
| --- | --- |
| Fresh flash creation and explicit erase | All image bytes `0xff`, correct file size |
| Flash update with 1, 4, and 8 occupied pages | Correct target prefix/spare, all neighboring pages unchanged, temporary block erased |
| Invalid flash requests | Occupied-page write and out-of-range page/block rejected |
| Instrumented flash update | CLI read totals differ from actual driver entries; counts match the derived formula |
| Hybrid hot, full, and mixed workloads | 6,100 logical writes, repeated merges, bounded image size, and 1,820 complete 512-byte sector comparisons |
| Hybrid demo | `LSN 8: A2` and logical block 2 mapped to PBN 0, offset 3 |
| Record append/search/delete/insert | 250 matches, persistent reverse-order reuse, 750 unchanged complete slots, stable 85,008-byte file |
| Record append after deletion | Live slot overwritten in a separate three-record fixture |
| AddressSanitizer + UndefinedBehaviorSanitizer on hybrid demo | Complete-buffer demo finishes with no reported address/undefined-behavior error |
| AddressSanitizer on maximum-length record append | Reproduces an 86-byte `sprintf` write into the 85-byte `pack()` buffer |
| Clean and rebuild | All three executable targets regenerate; unrelated record/image files remain intact |
| README command examples | Documented flash output, latest-version demo, and record queries reproduced in disposable directories |
| Documentation links and SVGs | Local targets resolve, XML parses, glyph references resolve, no external SVG resources |
| Figure inspection | All five SVGs rasterized to temporary PNGs and checked for legibility and clipping |

The measurement runner was repeated after strengthening survivor checks to compare full 85-byte slots, rather than only their text. Operation-count and allocation results agreed. For the hybrid demo sanitizer check, `ASAN_OPTIONS=detect_leaks=0` excludes free-list lifetime allocations from the check; this is not a claim of leak-free process state.

## Reproduced defects and boundaries

The append-after-delete overwrite and full-slot record packing overflow were reproduced. Flash read-counter omissions were reproduced with external driver instrumentation. The prior hybrid README's separate "log and data blocks" description was corrected to the implemented single mapped-block append layout.

Other limitations are identified directly from source: incomplete numeric/field validation, uninitialized padding, missing error status in the FTL API, volatile mappings with no reopen scan, record parsing bounds, and non-atomic data/header or merge updates. Successful fixtures cover valid LSNS and short, nonempty ASCII record fields; they do not establish correctness for malformed images, maximum-length strings, concurrency, crashes, or real NAND devices.

## Visual references

The editorial references were the current public `jaeunda/compiler`, `jaeunda/linux-filesystem-tools`, and `jaeunda/xv6-kernel-extensions` READMEs. They matched the local reference copies. The new figures use the same outlined Noto Sans approach, with storage-specific page strips, linked-slot arrows, and workload bars. FontTools 4.66.1 produced the outlines; CairoSVG 2.9.1 generated temporary inspection previews. The font license is included under [assets](assets/NotoSans-OFL.txt).
