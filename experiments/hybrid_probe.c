/* Deterministic write workloads with byte-for-byte read-back verification. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hybridmapping.h"

FILE *flashmemoryfp;
extern unsigned long driver_reads, driver_writes, driver_erases;
void ftl_open(void);
void ftl_write(int lsn, char *sectorbuf);
void ftl_read(int lsn, char *sectorbuf);

static char expected[DATAPAGES_PER_DEVICE][SECTOR_SIZE];
static int present[DATAPAGES_PER_DEVICE];

static void write_version(int lsn, int version)
{
    char sector[SECTOR_SIZE];
    for (int i = 0; i < SECTOR_SIZE; i++)
        sector[i] = (char)((lsn * 17 + version * 31 + i) % 251);
    memcpy(sector, &version, sizeof(version));
    ftl_write(lsn, sector);
    memcpy(expected[lsn], sector, SECTOR_SIZE);
    present[lsn] = 1;
}

static int verify(void)
{
    int checked = 0;
    for (int lsn = 0; lsn < DATAPAGES_PER_DEVICE; lsn++) {
        if (!present[lsn]) continue;
        char sector[SECTOR_SIZE];
        memset(sector, 0, sizeof(sector));
        ftl_read(lsn, sector);
        assert(memcmp(sector, expected[lsn], SECTOR_SIZE) == 0);
        checked++;
    }
    return checked;
}

int main(int argc, char **argv)
{
    assert(argc == 2);
    flashmemoryfp = tmpfile();
    assert(flashmemoryfp);
    char block[BLOCK_SIZE];
    memset(block, 0xff, sizeof(block));
    for (int i = 0; i < BLOCKS_PER_DEVICE; i++)
        assert(fwrite(block, sizeof(block), 1, flashmemoryfp) == 1);
    ftl_open();
    int writes = 0, verified = 0;

    if (strcmp(argv[1], "hot") == 0) {
        for (int i = 0; i < 1000; i++) {
            write_version(0, i);
            writes++;
        }
    } else if (strcmp(argv[1], "full") == 0) {
        for (int i = 0; i < PAGES_PER_BLOCK; i++) {
            write_version(i, i);
            writes++;
        }
        for (int i = 0; i < 1000; i++) {
            write_version(i % PAGES_PER_BLOCK, i + PAGES_PER_BLOCK);
            writes++;
        }
    } else {
        assert(strcmp(argv[1], "mixed") == 0);
        uint32_t state = 42;
        for (int i = 0; i < 4096; i++) {
            state = state * UINT32_C(1664525) + UINT32_C(1013904223);
            write_version((int)(state % DATAPAGES_PER_DEVICE), i);
            writes++;
            /* Save counts: these reads validate behavior, not write cost. */
            if ((i + 1) % 64 == 0) {
                unsigned long previous = driver_reads;
                verified += verify();
                driver_reads = previous;
            }
        }
    }
    unsigned long reads = driver_reads;
    verified += verify();
    assert(fseek(flashmemoryfp, 0, SEEK_END) == 0);
    long image_bytes = ftell(flashmemoryfp);
    assert(image_bytes == BLOCKS_PER_DEVICE * BLOCK_SIZE);
    printf("{\"logical_writes\":%d,\"reads\":%lu,\"writes\":%lu,"
           "\"erases\":%lu,\"verified_sectors\":%d,\"image_bytes\":%ld}\n",
           writes, reads, driver_writes, driver_erases, verified,
           image_bytes);
    assert(fclose(flashmemoryfp) == 0);
    return 0;
}
