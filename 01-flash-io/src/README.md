# Flash I/O source guide

| File | Contents |
| --- | --- |
| [flash.h](flash.h) | Fixed geometry: 512-byte sector, 16-byte spare, eight pages per block; driver declarations |
| [fdevicedriver.c](fdevicedriver.c) | Page-sized `fread`/`fwrite` at `PAGE_SIZE*ppn`; block-sized `0xff` erase at `BLOCK_SIZE*pbn` |
| [ftlmgr.c](ftlmgr.c) | CLI, native spare serialization, physical range checks, empty-page checks, and temporary-block update |

`main()` dispatches `c/w/r/e/u` to `ftl_makefile`, `ftl_write`, `ftl_read`, `ftl_erase`, and `ftl_update`. The manager owns `flashmemoryfp`; the driver accesses that stream through `extern`. `is_page_empty()` reads and examines a complete page. `find_empty_block()` probes candidate blocks from highest PBN downward.

For the update path, follow `ftl_update()` through neighbor copy-out, source erase, target write, neighbor copy-back, and temporary erase. Its local `cnt_read` differs from actual driver entries; the external measurement wrapper captures the missing probes without modifying this source.

The driver and geometry retain the original assignment interface. Build and input/output contracts are documented in the [project README](../README.md); generated binaries are outside this directory under the repository's `build/`.
