#include "diskogram.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>

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
        snprintf(buf, bufsize, "%" PRIu64 " B", bytes);
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

static const char* format_time_interval(time_t t, interval_t interval, char *buf, size_t bufsize) {
    struct tm *tm_info = localtime(&t);
    if (!tm_info) {
        snprintf(buf, bufsize, "unknown");
        return buf;
    }

    switch (interval) {
        case INTERVAL_HOUR:
            strftime(buf, bufsize, "%Y-%m-%d %H:00", tm_info);
            break;
        case INTERVAL_DAY:
            strftime(buf, bufsize, "%Y-%m-%d", tm_info);
            break;
        case INTERVAL_MONTH:
            strftime(buf, bufsize, "%Y-%m", tm_info);
            break;
        case INTERVAL_YEAR:
            strftime(buf, bufsize, "%Y", tm_info);
            break;
        default:
            strftime(buf, bufsize, "%Y-%m-%d", tm_info);
            break;
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
    if (hist->cutoff_time > 0) {
        char since_buf[64];
        struct tm *tm_cutoff = localtime(&hist->cutoff_time);
        if (tm_cutoff) {
            strftime(since_buf, sizeof(since_buf), "%Y-%m-%d", tm_cutoff);
        } else {
            snprintf(since_buf, sizeof(since_buf), "unknown");
        }
        /* Strip trailing 's' for singular (n==1) */
        char unit_display[16];
        snprintf(unit_display, sizeof(unit_display), "%s", hist->filter_unit);
        if (hist->filter_last_n == 1) {
            size_t len = strlen(unit_display);
            if (len > 1 && unit_display[len - 1] == 's') unit_display[len - 1] = '\0';
        }
        printf("Window: last %d %s (since %s)\n", hist->filter_last_n, unit_display, since_buf);
    }
    printf("Total: %s in %" PRIu64 " files",
           format_size(hist->total_bytes, size_buf, sizeof(size_buf)),
           hist->total_files);
    printf(" (%" PRIu64 " directories scanned)\n",
           hist->directories_scanned);

    /* Show warnings if errors occurred */
    if (hist->error_count > 0) {
        printf("\nWARNING: %" PRIu64 " error(s) occurred during scan\n",
               hist->error_count);
        if (hist->last_error[0] != '\0') {
            printf("Last error: %s\n", hist->last_error);
        }
        printf("Results may be incomplete.\n");
    }
    printf("\n");

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
        printf("%s  ", format_time_interval(bucket->start_time, hist->interval, time_buf, sizeof(time_buf)));
        for (int j = 0; j < bar_len; j++) {
            printf(BLOCK_CHAR);
        }

        /* Print size and file count */
        printf("  %s (%" PRIu64 " files)\n",
               format_size(bucket->total_bytes, size_buf, sizeof(size_buf)),
               bucket->file_count);
    }

    printf("\n");
}
