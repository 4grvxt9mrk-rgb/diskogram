#ifndef SPACETIME_H
#define SPACETIME_H

#include <stdint.h>
#include <time.h>

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
} histogram_t;

/* Function declarations */

/* Directory traversal */
int scan_directory(const char *path, grouping_mode_t mode, histogram_t *hist);

/* Histogram management */
histogram_t* histogram_create(void);
void histogram_destroy(histogram_t *hist);
void histogram_add_file(histogram_t *hist, time_t file_time, uint64_t size);
void histogram_finalize(histogram_t *hist);

/* Display */
void display_histogram(const histogram_t *hist, const char *title);

/* Utilities */
const char* format_size(uint64_t bytes, char *buf, size_t bufsize);
const char* format_time(time_t t, char *buf, size_t bufsize);

#endif /* SPACETIME_H */
