#include "diskogram.h"
#include <stdio.h>
#include <string.h>

/* Escape a string for safe JSON output */
static void print_json_escaped(const char *str) {
    if (!str) {
        printf("null");
        return;
    }

    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  printf("\\\""); break;
            case '\\': printf("\\\\"); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\n': printf("\\n"); break;
            case '\r': printf("\\r"); break;
            case '\t': printf("\\t"); break;
            default:
                if ((unsigned char)*p < 0x20) {
                    /* Control characters */
                    printf("\\u%04x", (unsigned char)*p);
                } else {
                    putchar(*p);
                }
                break;
        }
    }
}

/* Escape a string for safe XML output */
static void print_xml_escaped(const char *str) {
    if (!str) return;

    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '<':  printf("&lt;"); break;
            case '>':  printf("&gt;"); break;
            case '&':  printf("&amp;"); break;
            case '"':  printf("&quot;"); break;
            case '\'': printf("&apos;"); break;
            default:
                putchar(*p);
                break;
        }
    }
}

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
    printf("# Version: %s\n", DISKOGRAM_VERSION);
    printf("# Scan Duration: %ld seconds\n",
           (long)(hist->scan_end_time - hist->scan_start_time));
    printf("# Directories Scanned: %lu\n", (unsigned long)hist->directories_scanned);
    printf("# Errors: %lu\n", (unsigned long)hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("# Last Error: %s\n", hist->last_error);
    }
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

    char start_buf[64], end_buf[64];
    struct tm *tm_info;

    printf("{\n");
    printf("  \"version\": \"%s\",\n", DISKOGRAM_VERSION);
    printf("  \"title\": \"");
    print_json_escaped(title);
    printf("\",\n");
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

    /* Scan metadata */
    tm_info = localtime(&hist->scan_start_time);
    if (tm_info) {
        strftime(start_buf, sizeof(start_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
    } else {
        snprintf(start_buf, sizeof(start_buf), "unknown");
    }
    printf("  \"scan_start\": \"%s\",\n", start_buf);

    tm_info = localtime(&hist->scan_end_time);
    if (tm_info) {
        strftime(end_buf, sizeof(end_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
    } else {
        snprintf(end_buf, sizeof(end_buf), "unknown");
    }
    printf("  \"scan_end\": \"%s\",\n", end_buf);
    printf("  \"scan_duration_seconds\": %ld,\n",
           (long)(hist->scan_end_time - hist->scan_start_time));
    printf("  \"directories_scanned\": %lu,\n", (unsigned long)hist->directories_scanned);
    printf("  \"error_count\": %lu,\n", (unsigned long)hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("  \"last_error\": \"");
        print_json_escaped(hist->last_error);
        printf("\",\n");
    }

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

    char start_buf[64], end_buf[64];
    struct tm *tm_info;

    printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    printf("<histogram>\n");
    printf("  <version>%s</version>\n", DISKOGRAM_VERSION);
    printf("  <title>");
    print_xml_escaped(title);
    printf("</title>\n");
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

    /* Scan metadata */
    tm_info = localtime(&hist->scan_start_time);
    if (tm_info) {
        strftime(start_buf, sizeof(start_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
        printf("  <scan_start>%s</scan_start>\n", start_buf);
    }
    tm_info = localtime(&hist->scan_end_time);
    if (tm_info) {
        strftime(end_buf, sizeof(end_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
        printf("  <scan_end>%s</scan_end>\n", end_buf);
    }
    printf("  <scan_duration_seconds>%ld</scan_duration_seconds>\n",
           (long)(hist->scan_end_time - hist->scan_start_time));
    printf("  <directories_scanned>%lu</directories_scanned>\n",
           (unsigned long)hist->directories_scanned);
    printf("  <error_count>%lu</error_count>\n", (unsigned long)hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("  <last_error>");
        print_xml_escaped(hist->last_error);
        printf("</last_error>\n");
    }

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

/* Batch export helpers for JSON arrays */
void export_json_array_start(void) {
    printf("[\n");
}

void export_json_array_item(const histogram_t *hist, const char *title, int is_last) {
    if (!hist || hist->bucket_count == 0) {
        /* Skip empty histograms in batch mode */
        return;
    }

    char time_buf[64];
    const char *format = get_interval_format(hist->interval);

    char start_buf[64], end_buf[64];
    struct tm *tm_info;

    printf("  {\n");
    printf("    \"version\": \"%s\",\n", DISKOGRAM_VERSION);
    printf("    \"title\": \"");
    print_json_escaped(title);
    printf("\",\n");
    printf("    \"total_bytes\": %lu,\n", (unsigned long)hist->total_bytes);
    printf("    \"total_files\": %lu,\n", (unsigned long)hist->total_files);
    printf("    \"interval\": \"");
    switch (hist->interval) {
        case INTERVAL_HOUR: printf("hour"); break;
        case INTERVAL_DAY: printf("day"); break;
        case INTERVAL_MONTH: printf("month"); break;
        case INTERVAL_YEAR: printf("year"); break;
    }
    printf("\",\n");

    /* Scan metadata */
    tm_info = localtime(&hist->scan_start_time);
    if (tm_info) {
        strftime(start_buf, sizeof(start_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
    } else {
        snprintf(start_buf, sizeof(start_buf), "unknown");
    }
    printf("    \"scan_start\": \"%s\",\n", start_buf);

    tm_info = localtime(&hist->scan_end_time);
    if (tm_info) {
        strftime(end_buf, sizeof(end_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
    } else {
        snprintf(end_buf, sizeof(end_buf), "unknown");
    }
    printf("    \"scan_end\": \"%s\",\n", end_buf);
    printf("    \"scan_duration_seconds\": %ld,\n",
           (long)(hist->scan_end_time - hist->scan_start_time));
    printf("    \"directories_scanned\": %lu,\n", (unsigned long)hist->directories_scanned);
    printf("    \"error_count\": %lu,\n", (unsigned long)hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("    \"last_error\": \"");
        print_json_escaped(hist->last_error);
        printf("\",\n");
    }

    printf("    \"buckets\": [\n");

    for (size_t i = 0; i < hist->bucket_count; i++) {
        time_bucket_t *bucket = &hist->buckets[i];
        struct tm *tm_info = localtime(&bucket->start_time);

        if (tm_info) {
            strftime(time_buf, sizeof(time_buf), format, tm_info);
        } else {
            snprintf(time_buf, sizeof(time_buf), "unknown");
        }

        printf("      {\n");
        printf("        \"time\": \"%s\",\n", time_buf);
        printf("        \"bytes\": %lu,\n", (unsigned long)bucket->total_bytes);
        printf("        \"files\": %lu\n", (unsigned long)bucket->file_count);
        printf("      }%s\n", (i < hist->bucket_count - 1) ? "," : "");
    }

    printf("    ]\n");
    printf("  }%s\n", is_last ? "" : ",");
}

void export_json_array_end(void) {
    printf("]\n");
}

/* Batch export helpers for XML collections */
void export_xml_collection_start(void) {
    printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
    printf("<histograms>\n");
}

void export_xml_collection_item(const histogram_t *hist, const char *title) {
    if (!hist || hist->bucket_count == 0) {
        /* Skip empty histograms in batch mode */
        return;
    }

    char time_buf[64];
    const char *format = get_interval_format(hist->interval);

    char start_buf[64], end_buf[64];
    struct tm *tm_info;

    printf("  <histogram>\n");
    printf("    <version>%s</version>\n", DISKOGRAM_VERSION);
    printf("    <title>");
    print_xml_escaped(title);
    printf("</title>\n");
    printf("    <total_bytes>%lu</total_bytes>\n", (unsigned long)hist->total_bytes);
    printf("    <total_files>%lu</total_files>\n", (unsigned long)hist->total_files);
    printf("    <interval>");
    switch (hist->interval) {
        case INTERVAL_HOUR: printf("hour"); break;
        case INTERVAL_DAY: printf("day"); break;
        case INTERVAL_MONTH: printf("month"); break;
        case INTERVAL_YEAR: printf("year"); break;
    }
    printf("</interval>\n");

    /* Scan metadata */
    tm_info = localtime(&hist->scan_start_time);
    if (tm_info) {
        strftime(start_buf, sizeof(start_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
        printf("    <scan_start>%s</scan_start>\n", start_buf);
    }
    tm_info = localtime(&hist->scan_end_time);
    if (tm_info) {
        strftime(end_buf, sizeof(end_buf), "%Y-%m-%dT%H:%M:%S", tm_info);
        printf("    <scan_end>%s</scan_end>\n", end_buf);
    }
    printf("    <scan_duration_seconds>%ld</scan_duration_seconds>\n",
           (long)(hist->scan_end_time - hist->scan_start_time));
    printf("    <directories_scanned>%lu</directories_scanned>\n",
           (unsigned long)hist->directories_scanned);
    printf("    <error_count>%lu</error_count>\n", (unsigned long)hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("    <last_error>");
        print_xml_escaped(hist->last_error);
        printf("</last_error>\n");
    }

    printf("    <buckets>\n");

    for (size_t i = 0; i < hist->bucket_count; i++) {
        time_bucket_t *bucket = &hist->buckets[i];
        struct tm *tm_info = localtime(&bucket->start_time);

        if (tm_info) {
            strftime(time_buf, sizeof(time_buf), format, tm_info);
        } else {
            snprintf(time_buf, sizeof(time_buf), "unknown");
        }

        printf("      <bucket>\n");
        printf("        <time>%s</time>\n", time_buf);
        printf("        <bytes>%lu</bytes>\n", (unsigned long)bucket->total_bytes);
        printf("        <files>%lu</files>\n", (unsigned long)bucket->file_count);
        printf("      </bucket>\n");
    }

    printf("    </buckets>\n");
    printf("  </histogram>\n");
}

void export_xml_collection_end(void) {
    printf("</histograms>\n");
}

/* Batch export helpers for CSV with Path column */
void export_csv_batch_start(const char *mode_name, interval_t interval) {
    (void)mode_name; /* Unused - kept for future metadata */
    (void)interval;  /* Unused - kept for future metadata */

    /* Output header with Path column */
    printf("Path,Time,Bytes,Files,Human-Readable Size\n");
}

void export_csv_batch_item(const histogram_t *hist, const char *path, interval_t interval) {
    if (!hist || hist->bucket_count == 0) {
        /* Skip empty histograms in batch mode */
        return;
    }

    char time_buf[64];
    char size_buf[64];
    const char *format = get_interval_format(interval);

    for (size_t i = 0; i < hist->bucket_count; i++) {
        time_bucket_t *bucket = &hist->buckets[i];
        struct tm *tm_info = localtime(&bucket->start_time);

        if (tm_info) {
            strftime(time_buf, sizeof(time_buf), format, tm_info);
        } else {
            snprintf(time_buf, sizeof(time_buf), "unknown");
        }

        /* CSV with Path column - need to escape path if it contains commas/quotes */
        int needs_quoting = 0;
        for (const char *p = path; *p; p++) {
            if (*p == ',' || *p == '"' || *p == '\n') {
                needs_quoting = 1;
                break;
            }
        }

        if (needs_quoting) {
            printf("\"");
            for (const char *p = path; *p; p++) {
                if (*p == '"') {
                    printf("\"\""); /* Escape quotes by doubling */
                } else {
                    putchar(*p);
                }
            }
            printf("\"");
        } else {
            printf("%s", path);
        }

        printf(",%s,%lu,%lu,%s\n",
               time_buf,
               (unsigned long)bucket->total_bytes,
               (unsigned long)bucket->file_count,
               format_size(bucket->total_bytes, size_buf, sizeof(size_buf)));
    }
}
