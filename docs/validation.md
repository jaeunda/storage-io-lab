# Implementation and documentation validation

Checked on **2026-10-12 KST**, on the environment recorded in [measurements.md](measurements.md). Descriptions and diagrams were checked against the C source and the recorded measurement fixtures.

## Program layout

| Source directory | Executable | Purpose |
| --- | --- | --- |
| `01-flash-io/` | `build/flash-io` | Physical page/block operations |
| `02-hybrid-ftl/` | `build/hybrid-ftl` | Logical mapping, append, and merge |
| `03-record-store/` | `build/record-store` | Structured records and slot reuse |

The hybrid demonstration passes complete 512-byte buffers and asserts a latest-version read. Makefiles track header dependencies, enable warnings, and remove only the executable on `clean`. Data files are preserved.

Project and source READMEs describe implementation paths and file formats. Measurement fixtures, results, figure design, and regeneration instructions have separate documentation. Root `build/` contains ignored generated output.

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

Repeated measurement runs compared complete 85-byte survivor slots and produced matching operation-count and allocation results. For the hybrid demo sanitizer check, `ASAN_OPTIONS=detect_leaks=0` excludes free-list lifetime allocations from the check; this is not a claim of leak-free process state.

## Reproduced defects and boundaries

The append-after-delete overwrite and full-slot record packing overflow were reproduced. Flash read-counter omissions were reproduced with external driver instrumentation. Hybrid FTL checks exercise the implemented single mapped-block append layout.

Other limitations are identified directly from source: incomplete numeric/field validation, uninitialized padding, missing error status in the FTL API, volatile mappings with no reopen scan, record parsing bounds, and non-atomic data/header or merge updates. Successful fixtures cover valid LSNS and short, nonempty ASCII record fields; they do not establish correctness for malformed images, maximum-length strings, concurrency, crashes, or real NAND devices.

## Figure validation

The figures use outlined Noto Sans labels with page strips, linked-slot arrows, and workload bars. FontTools 4.66.1 produced the outlines; CairoSVG 2.9.1 generated temporary inspection previews. All five SVGs reproduced byte for byte from the checked-in generator. The font license is included under [assets](assets/NotoSans-OFL.txt).
