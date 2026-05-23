#include "sleepsize.h"
#include <stdio.h>
#include <string.h>
/*
 * Retorna o uso atual de RAM em MB lendo /proc/meminfo.
 * Calcula (MemTotal - MemAvailable) / 1024
 */
long sleep_get_ram_usage_mb(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return 0;
    long mem_total = 0;
    long mem_avail = 0;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "MemTotal:", 9) == 0) {
            sscanf(line, "MemTotal: %ld kB", &mem_total);
        } else if (strncmp(line, "MemAvailable:", 13) == 0) {
            sscanf(line, "MemAvailable: %ld kB", &mem_avail);
        }
        if (mem_total > 0 && mem_avail > 0) break;
    }
    fclose(f);
    if (mem_total > 0 && mem_avail > 0) {
        return (mem_total - mem_avail) / 1024;
    }
    return 0;
}
