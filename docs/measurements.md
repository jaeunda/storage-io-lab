# Storage measurements

Measured on **2026-10-12 KST** using the current source. The saved [JSON result](../experiments/results/latest.json) records the UTC timestamp and environment. These experiments count emulator driver calls and physical record slots; they do not measure NAND latency, durable storage throughput, or a speedup against another implementation.

## Reproduce

From the repository root on Linux or WSL with GCC, Make, Python 3, and GNU ld:

```bash
make measure
# Or retain the checked-in result and write a separate report:
python3 experiments/measure.py --output /tmp/storage-results.json
```

The runner builds the ordinary programs and two instrumented executables. GNU ld `--wrap=fdd_read`, `--wrap=fdd_write`, and `--wrap=fdd_erase` intercept calls from the manager to the unchanged device driver. Each wrapper counts an entry, then calls the real function. Failed driver calls would also count, but all measured workload operations completed successfully and passed read-back checks.

Instrumented executables use `-O0 -g`; default project builds also use `-O0 -g` with warnings enabled. Fixtures and instrumented executables live in a Python `TemporaryDirectory` and are removed when the runner finishes. The only persistent measurement output is the requested JSON file. `CC` can select another compiler compatible with GNU ld.

## Environment

| Item | Recorded environment |
| --- | --- |
| OS/kernel | Linux / WSL2, `6.18.33.2-microsoft-standard-WSL2` |
| Architecture | x86_64, little endian |
| CPU | Intel Core Ultra 5 125H |
| Compiler | GCC 9.5.0, Ubuntu `9.5.0-6ubuntu2` |
| Python | 3.12.3 |
| Native `int` | 4 bytes |

The original formats use native four-byte integers; the runner rejects other `int` sizes. Operation counts depend on algorithm and workload geometry, not CPU performance. Repeated identical workloads produced identical counts; byte-for-byte whole-image equality is not expected because unused padding is not fully initialized.

## Flash update

Three fresh two-block images contain 1, 4, or 8 occupied pages at the start of PBN 0; PBN 1 is fully erased. Sector text is `page-N`, with spare `N + 100`. The measured request updates PPN 0 to `updated`, spare 999.

The count window is one complete `u` process, including target validation, empty-block search, page-emptiness tests, data movement, and both erases. Image creation, initial writes, read-back, rejection checks, and final explicit erase are outside that window.

| Occupied source pages | Printed reads | Instrumented reads | Instrumented writes | Instrumented erases | Image bytes |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 7 | 23 | 1 | 2 | 8,448 |
| 4 | 10 | 29 | 7 | 2 | 8,448 |
| 8 | 14 | 37 | 15 | 2 | 8,448 |

For `k` occupied pages, the full update writes each of `k-1` neighbors twice, plus the target once: **`2k-1` writes**. Actual reads are **`21+2k` in this fixture**: one target probe, eight free-block probes, seven source-neighbor probes, `k-1` explicit copy-out reads, seven temporary-neighbor probes, and `k-1` explicit copy-back reads. Different empty-block search paths change that total.

The CLI prints only its manually maintained counter. It includes seven first-pass neighbor counts and `k-1` explicit copy-back reads, producing `k+6`, while omitting other calls. It is not a reliable total-driver-read metric.

Verification compared every non-target page in the source block byte for byte, checked the replacement text prefix and native spare value, confirmed a fully erased temporary block, and checked the 8,448-byte image size. Additional checks rejected writing an occupied page, reading PPN 16, and erasing PBN 2; a final erase restored the whole image to `0xff`.

## Hybrid FTL

Each workload runs in a fresh process with a fresh erased **8-block × 4-page × 528-byte = 16,896-byte** image. There are 28 usable logical sectors. Full 512-byte payloads are deterministic functions of LSN, version, and byte position; the version integer also occupies their first four bytes.

| Workload | Exact write sequence |
| --- | --- |
| `hot` | 1,000 successive versions of LSN 0 |
| `full` | Initial writes to LSNs 0, 1, 2, 3, then 1,000 writes cycling through those LSNs |
| `mixed` | 4,096 writes selected by `state = state * 1664525 + 1013904223` in unsigned 32-bit arithmetic, initial state 42, LSN `state % 28` |

The mixed generator is a deterministic fixture, not a claim of uniformly random traffic. The four-live-sector result includes its four initial writes in the denominator.

| Workload | Logical writes | Driver reads during writes | Driver page writes | Driver block erases | Physical writes / logical writes |
| --- | ---: | ---: | ---: | ---: | ---: |
| `hot` | 1,000 | 996 | 1,000 | 249 | 1.000000 |
| `full` | 1,004 | 4,000 | 4,004 | 1,000 | 3.988048 |
| `mixed` | 4,096 | 16,156 | 16,184 | 4,039 | 3.951172 |

Counts exclude initialization and verification reads. Image creation uses direct `fwrite`; successful write requests call the real FTL implementation and instrumented driver. Each full-block merge scans all four old pages. In the hot workload no peer survives, so no copy writes are needed. With four live sectors, each merge preserves three peers and appends the replacement. This explains the observed range without establishing a comparison to a separate page-mapping implementation.

Read-back compared all 512 bytes of every written sector after each workload. The mixed run also checked all then-written sectors every 64 writes. Verification performed 1 sector comparison in `hot`, 4 in `full`, and 1,815 in `mixed`, for **1,820 complete sector comparisons** in total. The saved `verified_sectors` values count comparisons, including repeated checks, rather than unique logical sectors.

![Measured write amplification for the three workloads.](assets/measured-results.svg)

## Record slot reuse

The fixture appends 1,000 records with eight-digit SIDs, short names, `Seoul` addresses, and short email strings. Every fourth record has `DEPT=DROP`; others use `KEEP`. A separate CLI invocation performs each operation, exercising normal close/reopen behavior.

1. Search `DEPT=DROP` and verify 250 printed matches.
2. Delete those 250 records; verify header `(live_count=750, free_head=996)` and 250 deletion markers.
3. Insert 250 `DEPT=NEW` replacements using `-i`.
4. Verify header `(1000, -1)`, the exact reverse-scan reuse sequence `996, 992, …, 0`, all 750 surviving 85-byte slots unchanged, and 250 searchable replacements.

| Stage | Live records | Physical slots | File bytes |
| --- | ---: | ---: | ---: |
| After initial append | 1,000 | 1,000 | 85,008 |
| After deletion | 750 | 1,000 | 85,008 |
| After insertion | 1,000 | 1,000 | 85,008 |

The observed size is **`8 + 1000*85 = 85,008 bytes`**. All 250 replacement writes reused existing slots; growth was zero. This checks allocation behavior, not search performance or a storage reduction relative to another record implementation.

A separate three-record fixture reproduces the append-after-delete defect: append 0, 1, 2; delete 0; append 3; verify that slot 2 now contains 3 and ID 2 is lost. The runner keeps this defective path separate from the successful reuse measurement. [Format and additional limits](../03-record-store/README.md).
