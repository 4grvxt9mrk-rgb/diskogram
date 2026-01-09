#include "spacetime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_BUCKET_CAPACITY 128
#define SECONDS_PER_HOUR (60 * 60)
#define SECONDS_PER_DAY (24 * 60 * 60)

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
            return mktime(&tm_copy);

        default:
            return (t / SECONDS_PER_DAY) * SECONDS_PER_DAY;
    }
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

    hist->bucket_count = 0;
    hist->bucket_capacity = INITIAL_BUCKET_CAPACITY;
    hist->total_bytes = 0;
    hist->total_files = 0;
    hist->interval = interval;

    return hist;
}

void histogram_destroy(histogram_t *hist) {
    if (!hist) return;
    free(hist->buckets);
    free(hist);
}

void histogram_add_file(histogram_t *hist, time_t file_time, uint64_t size) {
    time_t bucket_time = normalize_time(file_time, hist->interval);

    /* Find existing bucket or create new one */
    size_t i;
    for (i = 0; i < hist->bucket_count; i++) {
        if (hist->buckets[i].start_time == bucket_time) {
            hist->buckets[i].total_bytes += size;
            hist->buckets[i].file_count++;
            hist->total_bytes += size;
            hist->total_files++;
            return;
        }
    }

    /* Need to add a new bucket */
    if (hist->bucket_count >= hist->bucket_capacity) {
        size_t new_capacity = hist->bucket_capacity * 2;
        time_bucket_t *new_buckets = realloc(hist->buckets,
                                              sizeof(time_bucket_t) * new_capacity);
        if (!new_buckets) {
            fprintf(stderr, "Error: out of memory\n");
            return;
        }
        hist->buckets = new_buckets;
        hist->bucket_capacity = new_capacity;
    }

    hist->buckets[hist->bucket_count].start_time = bucket_time;
    hist->buckets[hist->bucket_count].total_bytes = size;
    hist->buckets[hist->bucket_count].file_count = 1;
    hist->bucket_count++;

    hist->total_bytes += size;
    hist->total_files++;
}

void histogram_finalize(histogram_t *hist) {
    if (!hist || hist->bucket_count == 0) return;

    /* Sort buckets by time */
    qsort(hist->buckets, hist->bucket_count, sizeof(time_bucket_t), compare_buckets);
}
