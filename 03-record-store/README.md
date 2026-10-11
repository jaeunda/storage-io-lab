# Fixed-Slot Record Storage

**Reuse a deleted record's space without shifting its neighbors.** This C program stores student records in 85-byte slots, searches selected fields sequentially, and maintains a persistent free chain inside deleted slots. Insertion consumes the chain's head before extending the file.

![An eight-byte header points to a chain of deleted 85-byte slots. Inserting at the head advances the link and preserves the physical file size.](../docs/assets/record-reuse.svg)

## Header and records

| Location | Representation |
| --- | --- |
| Header bytes `0..3` | Native four-byte integer: number of live records |
| Header bytes `4..7` | Native four-byte integer: deleted-slot head RRN, or `-1` |
| Slot RRN `r` | 85 bytes at `8 + 85*r` |
| Live slot | `SID#NAME#DEPT#ADDR#EMAIL#`, followed by unused bytes |
| Deleted slot byte `0` | `*` marker |
| Deleted slot bytes `1..4` | Native four-byte integer: next deleted RRN, or `-1` |

The header's second integer is actively used as the free-list head, rather than merely reserved. RRN means relative record number, starting at zero. The live count can be smaller than the physical slot count after deletion. Neither deletion nor reuse compacts the file.

| Field | Maximum declared data length | CLI search/delete support |
| --- | ---: | --- |
| `SID` | 8 bytes | Yes |
| `NAME` | 10 bytes | Yes |
| `DEPT` | 12 bytes | Yes |
| `ADDR` | 30 bytes | No |
| `EMAIL` | 20 bytes | No |

The total is 80 data bytes plus five delimiters. Limits are byte counts, not Unicode character counts. The structure reserves NUL terminators separately, but the packed on-disk slot has only 85 bytes. `ADDR` and `EMAIL` exist in the enum and data format, but the search/delete switches handle only `SID`, `NAME`, and `DEPT`.

## Commands

```bash
# From the repository root; use an unused file path.
make -C 03-record-store
./build/record-store -a /tmp/students-example.dat 20260001 Ada CS Seoul ada@example.com
./build/record-store -a /tmp/students-example.dat 20260002 Lin CS Busan lin@example.com
./build/record-store -s /tmp/students-example.dat 'DEPT=CS'
./build/record-store -d /tmp/students-example.dat 'SID=20260001'
./build/record-store -i /tmp/students-example.dat 20260003 Min EE Seoul min@example.com
./build/record-store -s /tmp/students-example.dat 'SID=20260003'
```

| Option | Arguments | Action |
| --- | --- | --- |
| `-a` | `FILE SID NAME DEPT ADDR EMAIL` | Append on a file without deletions; see the defect below |
| `-s` | `FILE FIELD=VALUE` | Exact-match sequential search; print all matches |
| `-d` | `FILE FIELD=VALUE` | Delete all exact matches and push their slots onto the free chain |
| `-i` | `FILE SID NAME DEPT ADDR EMAIL` | Reuse the free head; fall back to append if no free slot remains |

A missing file is created with header `(0, -1)`, including on search/delete. Query fields are uppercase. Searches print `#records = N`, followed by one `#`-delimited line per match. Use nonempty fields without `#`, `=`, or a leading deletion marker, and keep the combined packed text below the full-slot boundary described below.

## Free-chain behavior

`delete()` scans physical RRNs until it has visited the header's original live-record count. It skips already deleted slots. Each newly deleted slot stores the previous head, becomes the head, and reduces the live count after the scan. Multiple matches are therefore linked in reverse scan order.

`insert()` reads that head's next pointer, writes the replacement record at the same RRN, increments the live count, and stores the next pointer as the new head. This is last-in-first-out reuse. The links and header are stored on disk and survive a normal process exit; the format does not provide transactional crash recovery.

The controlled fixture appended 1,000 records, deleted the 250 `DEPT=DROP` records, and inserted 250 replacements. It verified reverse-order reuse of every freed slot and byte-for-byte preservation of all 750 survivors. The file remained **8 + 1,000 × 85 = 85,008 bytes** after deletion and reinsertion. [Measurement and reproduction](../docs/measurements.md#record-slot-reuse).

## Reproduced append defect

`append()` uses the live-record count as the next RRN. After deletion that count is no longer the end of the physical file:

```text
Append IDs 0, 1, 2 → slots [0, 1, 2], live count 3
Delete ID 0        → slots [*, 1, 2], live count 2, free head 0
Append ID 3        → writes RRN 2; slots [*, 1, 3]
```

ID 2 is overwritten even though it was not deleted, and the updated header no longer agrees with the number of live slots. A later search can continue beyond EOF. The measurement runner reproduces the overwrite on a separate disposable image and does not search that corrupted fixture. **Use `-i` after deletions.** The original record algorithm is retained so the documented limitation stays visible.

## Source and additional limits

[src/student.c](src/student.c) contains packing, header/slot I/O, append, search, deletion, insertion, and CLI dispatch. [src/student.h](src/student.h) declares the format constants, fields, structure, and functions. [Source guide](src/README.md).

`pack()` uses `sprintf` into an 85-byte buffer: five maximum-length fields occupy 85 bytes plus a terminating NUL, causing a one-byte overflow. Short records leave padding uninitialized. `unpack()` uses `strcpy` on a slot that may have no NUL, and `strtok` cannot represent empty fields or embedded delimiters. Search ignores failed reads, unknown field names have no defined return from `getFieldID`, and the filename buffer is copied without bounds checking. The format uses native integers, has no locking or portable encoding, and header/slot updates are not atomic. The fixture uses short, nonempty ASCII fields and validates only those exercised paths.
