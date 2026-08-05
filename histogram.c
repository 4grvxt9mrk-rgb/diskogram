#include "diskogram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUCKET_CAPACITY 128
#define INITIAL_INDEX_CAPACITY 256   /* power of two; load factor kept < 0.75 */
#define SECONDS_PER_HOUR (60 * 60)
#define SECONDS_PER_DAY (24 * 60 * 60)

/* Hash a normalized time_t into a slot. cap must be a power of two. */
static size_t hash_time(time_t t, size_t cap) {
    /* SplitMix64-style finalizer for good dispersion of clustered timestamps. */
    uint64_t x = (uint64_t)t;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    x ^= (x >> 31);
    return (size_t)(x & (uint64_t)(cap - 1));
}

static time_t normalize_time(time_t t, interval_t interval) {
    struct tm *tm_info;
    struct tm tm_copy;

    switch (interval) {
        case INTERVAL_HOUR:
            return (t / SECONDS_PER_HOUR) * SECONDS_PER_HOUR;

        case INTERVAL_DAY:
            return (t / SECONDS_PER_DAY) * SECONDS_PER_DAY;

        case INTERVAL_MONTH:
            tm_info = localtime(&t);
            if (!tm_info) return t;
            tm_copy = *tm_info;
            tm_copy.tm_mday = 1;
            tm_copy.tm_hour = 0;
            tm_copy.tm_min = 0;
            tm_copy.tm_sec = 0;
            tm_copy.tm_isdst = -1;  /* Let mktime() determine DST */
            return mktime(&tm_copy);

        case INTERVAL_YEAR:
            tm_info = localtime(&t);
            if (!tm_info) return t;
            tm_copy = *tm_info;
            tm_copy.tm_mon = 0;
            tm_copy.tm_mday = 1;
            tm_copy.tm_hour = 0;
            tm_copy.tm_min = 0;
            tm_copy.tm_sec = 0;
            tm_copy.tm_isdst = -1;  /* Let mktime() determine DST */
            return mktime(&tm_copy);

        default:
            return (t / SECONDS_PER_DAY) * SECONDS_PER_DAY;
    }
}

/* Insert bucket index bucket_idx into the hash, recomputing its slot.
 * Load factor is kept below 1 by the caller, so an empty slot always exists. */
static void index_insert(histogram_t *hist, size_t bucket_idx) {
    size_t slot = hash_time(hist->buckets[bucket_idx].start_time, hist->index_capacity);
    while (hist->index_slots[slot] != 0) {
        slot = (slot + 1) & (hist->index_capacity - 1);
    }
    hist->index_slots[slot] = bucket_idx + 1;
}

/* Grow the hash to new_capacity and re-insert every existing bucket.
 * Returns 0 on success, -1 on allocation failure (leaving the old table intact). */
static int index_rehash(histogram_t *hist, size_t new_capacity) {
    size_t *new_slots = calloc(new_capacity, sizeof(size_t));
    if (!new_slots) return -1;
    free(hist->index_slots);
    hist->index_slots = new_slots;
    hist->index_capacity = new_capacity;
    for (size_t i = 0; i < hist->bucket_count; i++) {
        index_insert(hist, i);
    }
    return 0;
}

static int compare_buckets(const void *a, const void *b) {
    const time_bucket_t *ba = (const time_bucket_t *)a;
    const time_bucket_t *bb = (const time_bucket_t *)b;
    if (ba->start_time < bb->start_time) return -1;
    if (ba->start_time > bb->start_time) return 1;
    return 0;
}

histogram_t* histogram_create(interval_t interval) {
    histogram_t *hist = malloc(sizeof(histogram_t));
    if (!hist) return NULL;

    hist->buckets = malloc(sizeof(time_bucket_t) * INITIAL_BUCKET_CAPACITY);
    if (!hist->buckets) {
        free(hist);
        return NULL;
    }

    hist->index_slots = calloc(INITIAL_INDEX_CAPACITY, sizeof(size_t));
    if (!hist->index_slots) {
        free(hist->buckets);
        free(hist);
        return NULL;
    }
    hist->index_capacity = INITIAL_INDEX_CAPACITY;
    hist->alloc_failed = 0;

    hist->bucket_count = 0;
    hist->bucket_capacity = INITIAL_BUCKET_CAPACITY;
    hist->total_bytes = 0;
    hist->total_files = 0;
    hist->interval = interval;

    /* Initialize scan metadata */
    hist->scan_start_time = time(NULL);
    hist->scan_end_time = 0;
    hist->error_count = 0;
    hist->directories_scanned = 0;
    hist->last_error[0] = '\0';

    /* Initialize error logging */
    hist->error_log_file = NULL;
    hist->log_errors_to_stderr = 0;

    /* Initialize time filter */
    hist->cutoff_time = 0;
    hist->filter_last_n = 0;
    hist->filter_unit[0] = '\0';

    /* Stay on one filesystem by default */
    hist->one_file_system = 1;

    return hist;
}

void histogram_destroy(histogram_t *hist) {
    if (!hist) return;
    free(hist->buckets);
    free(hist->index_slots);
    free(hist);
}

void histogram_add_file(histogram_t *hist, time_t file_time, uint64_t size) {
    /* Apply time filter if set */
    if (hist->cutoff_time > 0 && file_time < hist->cutoff_time) {
        return;
    }

    time_t bucket_time = normalize_time(file_time, hist->interval);

    /* Look up an existing bucket via the hash index (O(1) average). */
    size_t slot = hash_time(bucket_time, hist->index_capacity);
    while (hist->index_slots[slot] != 0) {
        size_t bi = hist->index_slots[slot] - 1;
        if (hist->buckets[bi].start_time == bucket_time) {
            hist->buckets[bi].total_bytes += size;
            hist->buckets[bi].file_count++;
            hist->total_bytes += size;
            hist->total_files++;
            return;
        }
        slot = (slot + 1) & (hist->index_capacity - 1);
    }

    /* Not found: grow the bucket array if full (overflow-checked). */
    if (hist->bucket_count >= hist->bucket_capacity) {
        if (hist->bucket_capacity > SIZE_MAX / 2 ||
            hist->bucket_capacity * 2 > SIZE_MAX / sizeof(time_bucket_t)) {
            fprintf(stderr, "Error: bucket capacity overflow\n");
            hist->alloc_failed = 1;
            return;
        }
        size_t new_capacity = hist->bucket_capacity * 2;
        time_bucket_t *new_buckets = realloc(hist->buckets,
                                             sizeof(time_bucket_t) * new_capacity);
        if (!new_buckets) {
            fprintf(stderr, "Error: out of memory\n");
            hist->alloc_failed = 1;
            return;
        }
        hist->buckets = new_buckets;
        hist->bucket_capacity = new_capacity;
    }

    /* Keep the hash load factor below 0.75; rehash (grow) if needed. A stored
     * slot becomes stale after a rehash, so the new bucket is always inserted
     * afterward via index_insert(), which recomputes its slot from scratch. */
    if (hist->bucket_count + 1 > (hist->index_capacity / 4) * 3) {
        if (hist->index_capacity > SIZE_MAX / 2 ||
            index_rehash(hist, hist->index_capacity * 2) != 0) {
            fprintf(stderr, "Error: out of memory\n");
            hist->alloc_failed = 1;
            return;
        }
    }

    hist->buckets[hist->bucket_count].start_time = bucket_time;
    hist->buckets[hist->bucket_count].total_bytes = size;
    hist->buckets[hist->bucket_count].file_count = 1;
    index_insert(hist, hist->bucket_count);
    hist->bucket_count++;

    hist->total_bytes += size;
    hist->total_files++;
}

void histogram_finalize(histogram_t *hist) {
    if (!hist) return;

    /* Record scan end time */
    hist->scan_end_time = time(NULL);

    if (hist->bucket_count == 0) return;

    /* Sort buckets by time */
    qsort(hist->buckets, hist->bucket_count, sizeof(time_bucket_t), compare_buckets);
}

void histogram_set_error_log(histogram_t *hist, FILE *log_file) {
    if (!hist) return;
    hist->error_log_file = log_file;
}

void histogram_set_error_stderr(histogram_t *hist, int enabled) {
    if (!hist) return;
    hist->log_errors_to_stderr = enabled;
}

void histogram_set_cutoff(histogram_t *hist, time_t cutoff, int n, const char *unit) {
    if (!hist) return;
    hist->cutoff_time = cutoff;
    hist->filter_last_n = n;
    snprintf(hist->filter_unit, sizeof(hist->filter_unit), "%s", unit);
}

void histogram_log_error(histogram_t *hist, const char *error_msg) {
    if (!hist || !error_msg) return;

    /* Get current timestamp */
    time_t now = time(NULL);
    char time_buf[64];
    struct tm *tm_info = localtime(&now);
    if (tm_info) {
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", tm_info);
    } else {
        snprintf(time_buf, sizeof(time_buf), "unknown time");
    }

    /* Log to file if enabled (escape control bytes; the log may be viewed
     * later in a terminal). */
    if (hist->error_log_file) {
        fprintf(hist->error_log_file, "[%s] ", time_buf);
        print_terminal_safe(error_msg, hist->error_log_file);
        fputc('\n', hist->error_log_file);
        fflush(hist->error_log_file);
    }

    /* Log to stderr if enabled */
    if (hist->log_errors_to_stderr) {
        fprintf(stderr, "[%s] ERROR: ", time_buf);
        print_terminal_safe(error_msg, stderr);
        fputc('\n', stderr);
    }
}
