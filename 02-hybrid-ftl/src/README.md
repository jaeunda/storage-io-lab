# Hybrid FTL source guide

| File | Contents |
| --- | --- |
| [hybridmapping.h](hybridmapping.h) | Device geometry and one-block reserve; defaults: four pages/block, eight blocks/device |
| [fdevicedriver.c](fdevicedriver.c) | Physical page read/write and block erase through a shared file stream |
| [ftlmgr.c](ftlmgr.c) | Block map, free-block linked list, append placement, backward reads, and merge |
| [main.c](main.c) | Fresh erased image, four sector writes, newest-version assertion, and printed mappings |

`ftl_open()` initializes each `_address_map` to `{pbn=-1, last_offset=-1}` and builds the free list through `insert_node()`. `get_empty_blk()` removes and frees its head node. The map and list exist only for the lifetime of this process.

`ftl_write()` performs first-block allocation, full-block merge, sector-plus-LSN serialization, and offset advancement. During merge, `is_copied[]` records which logical offsets have already been handled while scanning backward. The requested LSN starts marked, so its older versions are omitted. `ftl_read()` uses the same spare LSN identity to stop at the newest matching page.

The demonstration supplies complete 512-byte buffers. The separate [experiment harness](../../experiments/hybrid_probe.c) links this same FTL and driver, supplies binary payloads, and exercises sustained merges. See the [project README](../README.md) for API contracts, commands, limits, and measured results.
