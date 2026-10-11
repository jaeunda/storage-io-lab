# Record storage source guide

| File | Contents |
| --- | --- |
| [student.h](student.h) | 85-byte slot and eight-byte header constants, `FIELD` enum, `STUDENT` structure, and prototypes |
| [student.c](student.c) | All serialization, record/header I/O, storage operations, and CLI dispatch |

| Function group | Data flow |
| --- | --- |
| `pack` / `unpack` | Convert the structure to/from `#`-delimited slot text |
| `readRecord` / `writeRecord` | Seek to `HEADER_SIZE + RECORD_SIZE*rrn` and transfer one slot |
| `readHeader` / `writeHeader` | Transfer the live count and free-head native integers |
| `append` | Write at the live count and increment the header; incorrect after deletions |
| `search` / `print` | Scan past deleted slots, collect SID/NAME/DEPT matches, and print records |
| `delete` | Mark each matching slot and link it to the prior free head |
| `insert` | Consume the free head or call `append` when the chain is empty |
| `getFieldID` / `main` | Parse uppercase query fields and dispatch CLI operations |

Search and deletion use their own raw slot reads to identify the deletion marker before unpacking. A deleted slot embeds its next RRN in bytes 1–4; insertion reads that pointer before overwriting the slot. The header count tracks live records rather than physical slots.

This source retains the original record implementation, including packing bounds, incomplete input validation, and the reproduced append-after-delete defect. The [project README](../README.md) describes those limits and the supported command examples. The measurement runner verifies the successful insert-reuse path and isolates the defective append path.
