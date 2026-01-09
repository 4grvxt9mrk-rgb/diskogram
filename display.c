#include "spacetime.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

#define BAR_WIDTH 50
#define BLOCK_CHAR "#"

const char* format_size(uint64_t bytes, char *buf, size_t bufsize) {
    const char *units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
    int unit_idx = 0;
    double size = (double)bytes;

    while (size >= 1024.0 && unit_idx < 5) {
        size /= 1024.0;
        unit_idx++;
    }

    if (unit_idx == 0) {
        snprintf(buf, bufsize, "%lu B", (unsigned long)bytes);
    } else {
        snprintf(buf, bufsize, "%.2f %s", size, units[unit_idx]);
    }

    return buf;
}

const char* format_time(time_t t, char *buf, size_t bufsize) {
    struct tm *tm_info = localtime(&t);
    if (tm_info) {
        strftime(buf, bufsize, "%Y-%m-%d", tm_info);
    } else {
        snprintf(buf, bufsize, "unknown");
    }
    return buf;
}

void display_histogram(const histogram_t *hist, const char *title) {
    if (!hist || hist->bucket_count == 0) {
        printf("No data to display.\n");
        return;
    }

    char size_buf[64];
    char time_buf[64];

    printf("\n%s\n", title);
    printf("Total: %s in %lu files\n\n",
           format_size(hist->total_bytes, size_buf, sizeof(size_buf)),
           (unsigned long)hist->total_files);

    /* Find maximum size for scaling */
    uint64_t max_size = 0;
    for (size_t i = 0; i < hist->bucket_count; i++) {
        if (hist->buckets[i].total_bytes > max_size) {
            max_size = hist->buckets[i].total_bytes;
        }
    }

    if (max_size == 0) {
        printf("No data to display.\n");
        return;
    }

    /* Display each bucket */
    for (size_t i = 0; i < hist->bucket_count; i++) {
        time_bucket_t *bucket = &hist->buckets[i];

        /* Calculate bar length */
        int bar_len = (int)((double)bucket->total_bytes / (double)max_size * BAR_WIDTH);
        if (bar_len < 1 && bucket->total_bytes > 0) {
            bar_len = 1;
        }

        /* Print date and bar */
        printf("%s  ", format_time(bucket->start_time, time_buf, sizeof(time_buf)));
        for (int j = 0; j < bar_len; j++) {
            printf(BLOCK_CHAR);
        }

        /* Print size and file count */
        printf("  %s (%lu files)\n",
               format_size(bucket->total_bytes, size_buf, sizeof(size_buf)),
               (unsigned long)bucket->file_count);
    }

    printf("\n");
}
