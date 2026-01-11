#ifndef SPACETIME_H
#define SPACETIME_H

#include <stdint.h>
#include <stdio.h>
#include <time.h>

/* Version information */
#define DISKOGRAM_VERSION_MAJOR 1
#define DISKOGRAM_VERSION_MINOR 3
#define DISKOGRAM_VERSION_PATCH 1
#define DISKOGRAM_VERSION "1.3.1"

/* Platform detection */
#ifdef _WIN32
    #define PATH_SEPARATOR '\\'
    #define PATH_SEPARATOR_STR "\\"
#else
    #define PATH_SEPARATOR '/'
    #define PATH_SEPARATOR_STR "/"
#endif

/* Maximum path length */
#ifndef MAX_PATH_LEN
    #ifdef _WIN32
        #define MAX_PATH_LEN 260
    #else
        #define MAX_PATH_LEN 4096
    #endif
#endif

/* Date grouping modes */
typedef enum {
    GROUP_BY_MTIME,     /* modification time */
    GROUP_BY_CTIME,     /* creation/change time */
    GROUP_BY_ATIME      /* access time */
} grouping_mode_t;

/* Time interval granularity */
typedef enum {
    INTERVAL_HOUR,
    INTERVAL_DAY,
    INTERVAL_MONTH,
    INTERVAL_YEAR
} interval_t;

/* Export formats */
typedef enum {
    FORMAT_TEXT,        /* default terminal output */
    FORMAT_CSV,
    FORMAT_JSON,
    FORMAT_XML
} export_format_t;

/* Time bucket (e.g., a day, week, month, or year) */
typedef struct {
    time_t start_time;
    uint64_t total_bytes;
    uint64_t file_count;
} time_bucket_t;

/* Histogram structure */
typedef struct {
    time_bucket_t *buckets;
    size_t bucket_count;
    size_t bucket_capacity;
    uint64_t total_bytes;
    uint64_t total_files;
    interval_t interval;

    /* Scan metadata */
    time_t scan_start_time;
    time_t scan_end_time;
    uint64_t error_count;
    uint64_t directories_scanned;
    char last_error[256];

    /* Error logging */
    FILE *error_log_file;
    int log_errors_to_stderr;
} histogram_t;

/* Function declarations */

/* Directory traversal */
int scan_directory(const char *path, grouping_mode_t mode, histogram_t *hist);

/* Histogram management */
histogram_t* histogram_create(interval_t interval);
void histogram_destroy(histogram_t *hist);
void histogram_add_file(histogram_t *hist, time_t file_time, uint64_t size);
void histogram_finalize(histogram_t *hist);
void histogram_set_error_log(histogram_t *hist, FILE *log_file);
void histogram_set_error_stderr(histogram_t *hist, int enabled);
void histogram_log_error(histogram_t *hist, const char *error_msg);

/* Display and export */
void display_histogram(const histogram_t *hist, const char *title);
void export_csv(const histogram_t *hist, const char *title);
void export_json(const histogram_t *hist, const char *title);
void export_xml(const histogram_t *hist, const char *title);

/* Utilities */
const char* format_size(uint64_t bytes, char *buf, size_t bufsize);
const char* format_time(time_t t, char *buf, size_t bufsize);

#endif /* SPACETIME_H */
