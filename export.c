#include "spacetime.h"
#include <stdio.h>
#include <string.h>

static const char* get_interval_format(interval_t interval) {
    switch (interval) {
        case INTERVAL_HOUR:
            return "%Y-%m-%d %H:00";
        case INTERVAL_DAY:
            return "%Y-%m-%d";
        case INTERVAL_MONTH:
            return "%Y-%m";
        case INTERVAL_YEAR:
            return "%Y";
        default:
            return "%Y-%m-%d";
    }
}

void export_csv(const histogram_t *hist, const char *title) {
    if (!hist || hist->bucket_count == 0) {
        fprintf(stderr, "No data to export.\n");
        return;
    }

    char time_buf[64];
    char size_buf[64];
    const char *format = get_interval_format(hist->interval);

    printf("# %s\n", title);
    printf("Time,Bytes,Files,Human-Readable Size\n");

    for (size_t i = 0; i < hist->bucket_count; i++) {
        time_bucket_t *bucket = &hist->buckets[i];
        struct tm *tm_info = localtime(&bucket->start_time);

        if (tm_info) {
            strftime(time_buf, sizeof(time_buf), format, tm_info);
        } else {
            snprintf(time_buf, sizeof(time_buf), "unknown");
        }

        printf("%s,%lu,%lu,%s\n",
               time_buf,
               (unsigned long)bucket->total_bytes,
               (unsigned long)bucket->file_count,
               format_size(bucket->total_bytes, size_buf, sizeof(size_buf)));
    }
}

void export_json(const histogram_t *hist, const char *title) {
    if (!hist || hist->bucket_count == 0) {
        fprintf(stderr, "No data to export.\n");
        return;
    }

    char time_buf[64];
    const char *format = get_interval_format(hist->interval);

    printf("{\n");
    printf("  \"title\": \"%s\",\n", title);
    printf("  \"total_bytes\": %lu,\n", (unsigned long)hist->total_bytes);
    printf("  \"total_files\": %lu,\n", (unsigned long)hist->total_files);
    printf("  \"interval\": \"");
    switch (hist->interval) {
        case INTERVAL_HOUR: printf("hour"); break;
        case INTERVAL_DAY: printf("day"); break;
        case INTERVAL_MONTH: printf("month"); break;
        case INTERVAL_YEAR: printf("year"); break;
    }
    printf("\",\n");
    printf("  \"buckets\": [\n");

    for (size_t i = 0; i < hist->bucket_count; i++) {
        time_bucket_t *bucket = &hist->buckets[i];
        struct tm *tm_info = localtime(&bucket->start_time);

        if (tm_info) {
            strftime(time_buf, sizeof(time_buf), format, tm_info);
        } else {
            snprintf(time_buf, sizeof(time_buf), "unknown");
        }

        printf("    {\n");
        printf("      \"time\": \"%s\",\n", time_buf);
        printf("      \"bytes\": %lu,\n", (unsigned long)bucket->total_bytes);
        printf("      \"files\": %lu\n", (unsigned long)bucket->file_count);
        printf("    }%s\n", (i < hist->bucket_count - 1) ? "," : "");
    }

    printf("  ]\n");
    printf("}\n");
}

void export_xml(const histogram_t *hist, const char *title) {
    if (!hist || hist->bucket_count == 0) {
        fprintf(stderr, "No data to export.\n");
        return;
    }

    char time_buf[64];
    const char *format = get_interval_format(hist->interval);

    printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    printf("<histogram>\n");
    printf("  <title>%s</title>\n", title);
    printf("  <total_bytes>%lu</total_bytes>\n", (unsigned long)hist->total_bytes);
    printf("  <total_files>%lu</total_files>\n", (unsigned long)hist->total_files);
    printf("  <interval>");
    switch (hist->interval) {
        case INTERVAL_HOUR: printf("hour"); break;
        case INTERVAL_DAY: printf("day"); break;
        case INTERVAL_MONTH: printf("month"); break;
        case INTERVAL_YEAR: printf("year"); break;
    }
    printf("</interval>\n");
    printf("  <buckets>\n");

    for (size_t i = 0; i < hist->bucket_count; i++) {
        time_bucket_t *bucket = &hist->buckets[i];
        struct tm *tm_info = localtime(&bucket->start_time);

        if (tm_info) {
            strftime(time_buf, sizeof(time_buf), format, tm_info);
        } else {
            snprintf(time_buf, sizeof(time_buf), "unknown");
        }

        printf("    <bucket>\n");
        printf("      <time>%s</time>\n", time_buf);
        printf("      <bytes>%lu</bytes>\n", (unsigned long)bucket->total_bytes);
        printf("      <files>%lu</files>\n", (unsigned long)bucket->file_count);
        printf("    </bucket>\n");
    }

    printf("  </buckets>\n");
    printf("</histogram>\n");
}
