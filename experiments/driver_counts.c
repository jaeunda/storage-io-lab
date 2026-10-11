/* Link-time instrumentation: count real driver entries without changing it. */
#include <stdio.h>
#include <stdlib.h>

unsigned long driver_reads, driver_writes, driver_erases;
int __real_fdd_read(int ppn, char *buffer);
int __real_fdd_write(int ppn, char *buffer);
int __real_fdd_erase(int pbn);

int __wrap_fdd_read(int ppn, char *buffer)
{
    driver_reads++;
    return __real_fdd_read(ppn, buffer);
}

int __wrap_fdd_write(int ppn, char *buffer)
{
    driver_writes++;
    return __real_fdd_write(ppn, buffer);
}

int __wrap_fdd_erase(int pbn)
{
    driver_erases++;
    return __real_fdd_erase(pbn);
}

#ifdef REPORT_AT_EXIT
static void report(void)
{
    fprintf(stderr, "DRIVER reads=%lu writes=%lu erases=%lu\n",
            driver_reads, driver_writes, driver_erases);
}

__attribute__((constructor)) static void register_report(void)
{
    atexit(report);
}
#endif
