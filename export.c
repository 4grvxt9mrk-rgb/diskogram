#include "diskogram.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

/* Write a CSV field with RFC 4180 quoting and spreadsheet formula-injection
 * neutralization. A field beginning with '=', '+', '-', '@', TAB, or CR can be
 * interpreted as a formula by spreadsheet software; such fields are prefixed
 * with a single apostrophe so they import as literal text. */
static void print_csv_field(const char *s) {
    if (!s) return;

    int needs_prefix = (s[0] == '=' || s[0] == '+' || s[0] == '-' ||
                        s[0] == '@' || s[0] == '\t' || s[0] == '\r');

    int needs_quoting = 0;
    for (const char *p = s; *p; p++) {
        if (*p == ',' || *p == '"' || *p == '\n' || *p == '\r') {
            needs_quoting = 1;
            break;
        }
    }

    if (needs_quoting) {
        putchar('"');
        if (needs_prefix) putchar('\'');
        for (const char *p = s; *p; p++) {
            if (*p == '"') putchar('"');  /* double embedded quotes */
            putchar(*p);
        }
        putchar('"');
    } else {
        if (needs_prefix) putchar('\'');
        printf("%s", s);
    }
}

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

static void print_json_filter(const histogram_t *hist, const char *indent) {
    if (hist->cutoff_time == 0) return;
    char since_buf[64];
    struct tm *tm_cutoff = localtime(&hist->cutoff_time);
    if (tm_cutoff) strftime(since_buf, sizeof(since_buf), "%Y-%m-%dT%H:%M:%S", tm_cutoff);
    else snprintf(since_buf, sizeof(since_buf), "unknown");
    printf("%s\"filter_last_n\": %d,\n", indent, hist->filter_last_n);
    printf("%s\"filter_unit\": \"%s\",\n", indent, hist->filter_unit);
    printf("%s\"filter_since\": \"%s\",\n", indent, since_buf);
}

static void print_xml_filter(const histogram_t *hist, const char *indent) {
    if (hist->cutoff_time == 0) return;
    char since_buf[64];
    struct tm *tm_cutoff = localtime(&hist->cutoff_time);
    if (tm_cutoff) strftime(since_buf, sizeof(since_buf), "%Y-%m-%dT%H:%M:%S", tm_cutoff);
    else snprintf(since_buf, sizeof(since_buf), "unknown");
    printf("%s<filter_last_n>%d</filter_last_n>\n", indent, hist->filter_last_n);
    printf("%s<filter_unit>%s</filter_unit>\n", indent, hist->filter_unit);
    printf("%s<filter_since>%s</filter_since>\n", indent, since_buf);
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
    printf("# Directories Scanned: %" PRIu64 "\n", hist->directories_scanned);
    printf("# Errors: %" PRIu64 "\n", hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("# Last Error: %s\n", hist->last_error);
    }
    if (hist->cutoff_time > 0) {
        char since_buf[64];
        struct tm *tm_cutoff = localtime(&hist->cutoff_time);
        if (tm_cutoff) strftime(since_buf, sizeof(since_buf), "%Y-%m-%d", tm_cutoff);
        else snprintf(since_buf, sizeof(since_buf), "unknown");
        char unit_display[16];
        snprintf(unit_display, sizeof(unit_display), "%s", hist->filter_unit);
        if (hist->filter_last_n == 1) {
            size_t len = strlen(unit_display);
            if (len > 1 && unit_display[len - 1] == 's') unit_display[len - 1] = '\0';
        }
        printf("# Filter: last %d %s (since %s)\n", hist->filter_last_n, unit_display, since_buf);
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

        printf("%s,%" PRIu64 ",%" PRIu64 ",%s\n",
               time_buf,
               bucket->total_bytes,
               bucket->file_count,
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
    printf("  \"total_bytes\": %" PRIu64 ",\n", hist->total_bytes);
    printf("  \"total_files\": %" PRIu64 ",\n", hist->total_files);
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
    printf("  \"directories_scanned\": %" PRIu64 ",\n", hist->directories_scanned);
    printf("  \"error_count\": %" PRIu64 ",\n", hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("  \"last_error\": \"");
        print_json_escaped(hist->last_error);
        printf("\",\n");
    }
    print_json_filter(hist, "  ");

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
        printf("      \"bytes\": %" PRIu64 ",\n", bucket->total_bytes);
        printf("      \"files\": %" PRIu64 "\n", bucket->file_count);
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
    printf("  <total_bytes>%" PRIu64 "</total_bytes>\n", hist->total_bytes);
    printf("  <total_files>%" PRIu64 "</total_files>\n", hist->total_files);
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
    printf("  <directories_scanned>%" PRIu64 "</directories_scanned>\n",
           hist->directories_scanned);
    printf("  <error_count>%" PRIu64 "</error_count>\n", hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("  <last_error>");
        print_xml_escaped(hist->last_error);
        printf("</last_error>\n");
    }
    print_xml_filter(hist, "  ");

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
        printf("      <bytes>%" PRIu64 "</bytes>\n", bucket->total_bytes);
        printf("      <files>%" PRIu64 "</files>\n", bucket->file_count);
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
    printf("    \"total_bytes\": %" PRIu64 ",\n", hist->total_bytes);
    printf("    \"total_files\": %" PRIu64 ",\n", hist->total_files);
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
    printf("    \"directories_scanned\": %" PRIu64 ",\n", hist->directories_scanned);
    printf("    \"error_count\": %" PRIu64 ",\n", hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("    \"last_error\": \"");
        print_json_escaped(hist->last_error);
        printf("\",\n");
    }
    print_json_filter(hist, "    ");

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
        printf("        \"bytes\": %" PRIu64 ",\n", bucket->total_bytes);
        printf("        \"files\": %" PRIu64 "\n", bucket->file_count);
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
    printf("    <total_bytes>%" PRIu64 "</total_bytes>\n", hist->total_bytes);
    printf("    <total_files>%" PRIu64 "</total_files>\n", hist->total_files);
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
    printf("    <directories_scanned>%" PRIu64 "</directories_scanned>\n",
           hist->directories_scanned);
    printf("    <error_count>%" PRIu64 "</error_count>\n", hist->error_count);
    if (hist->error_count > 0 && hist->last_error[0] != '\0') {
        printf("    <last_error>");
        print_xml_escaped(hist->last_error);
        printf("</last_error>\n");
    }
    print_xml_filter(hist, "    ");

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
        printf("        <bytes>%" PRIu64 "</bytes>\n", bucket->total_bytes);
        printf("        <files>%" PRIu64 "</files>\n", bucket->file_count);
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

        /* Path column: quote per RFC 4180 and neutralize formula injection. */
        print_csv_field(path);

        printf(",%s,%" PRIu64 ",%" PRIu64 ",%s\n",
               time_buf,
               bucket->total_bytes,
               bucket->file_count,
               format_size(bucket->total_bytes, size_buf, sizeof(size_buf)));
    }
}
