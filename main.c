#include "spacetime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void print_usage(const char *progname) {
    printf("Usage: %s [OPTIONS] <directory>\n\n", progname);
    printf("Generate a histogram of disk space consumption grouped by date.\n\n");
    printf("Time Grouping Options:\n");
    printf("  -m, --mtime     Group by modification time (default)\n");
    printf("  -c, --ctime     Group by creation time\n");
    printf("  -a, --atime     Group by access time\n\n");
    printf("Interval Options:\n");
    printf("  --hour          Group by hour\n");
    printf("  --day           Group by day (default)\n");
    printf("  --month         Group by month\n");
    printf("  --year          Group by year\n\n");
    printf("Export Format Options:\n");
    printf("  --csv           Export as CSV\n");
    printf("  --json          Export as JSON\n");
    printf("  --xml           Export as XML\n\n");
    printf("Other Options:\n");
    printf("  -h, --help      Show this help message\n\n");
    printf("Examples:\n");
    printf("  %s .\n", progname);
    printf("  %s -c --month /path/to/directory\n", progname);
    printf("  %s --atime --year --json ~/Documents\n\n", progname);
}

int main(int argc, char *argv[]) {
    const char *target_dir = NULL;
    grouping_mode_t mode = GROUP_BY_MTIME;
    const char *mode_name = "Modification Time";
    interval_t interval = INTERVAL_DAY;
    export_format_t format = FORMAT_TEXT;

    /* Parse command-line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--mtime") == 0) {
            mode = GROUP_BY_MTIME;
            mode_name = "Modification Time";
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--ctime") == 0) {
            mode = GROUP_BY_CTIME;
#ifdef __APPLE__
            mode_name = "Creation Time";
#else
            mode_name = "Change Time";
#endif
        } else if (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--atime") == 0) {
            mode = GROUP_BY_ATIME;
            mode_name = "Access Time";
        } else if (strcmp(argv[i], "--hour") == 0) {
            interval = INTERVAL_HOUR;
        } else if (strcmp(argv[i], "--day") == 0) {
            interval = INTERVAL_DAY;
        } else if (strcmp(argv[i], "--month") == 0) {
            interval = INTERVAL_MONTH;
        } else if (strcmp(argv[i], "--year") == 0) {
            interval = INTERVAL_YEAR;
        } else if (strcmp(argv[i], "--csv") == 0) {
            format = FORMAT_CSV;
        } else if (strcmp(argv[i], "--json") == 0) {
            format = FORMAT_JSON;
        } else if (strcmp(argv[i], "--xml") == 0) {
            format = FORMAT_XML;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            if (target_dir != NULL) {
                fprintf(stderr, "Error: multiple directories specified\n");
                print_usage(argv[0]);
                return 1;
            }
            target_dir = argv[i];
        }
    }

    if (target_dir == NULL) {
        fprintf(stderr, "Error: no directory specified\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Create histogram */
    histogram_t *hist = histogram_create(interval);
    if (!hist) {
        fprintf(stderr, "Error: failed to create histogram\n");
        return 1;
    }

    /* Scan directory */
    if (format == FORMAT_TEXT) {
        printf("Scanning '%s'...\n", target_dir);
    }
    if (scan_directory(target_dir, mode, hist) != 0) {
        fprintf(stderr, "Error: failed to scan directory\n");
        histogram_destroy(hist);
        return 1;
    }

    /* Finalize and display/export */
    histogram_finalize(hist);

    char title[256];
    snprintf(title, sizeof(title), "Disk Space by %s: %s", mode_name, target_dir);

    switch (format) {
        case FORMAT_CSV:
            export_csv(hist, title);
            break;
        case FORMAT_JSON:
            export_json(hist, title);
            break;
        case FORMAT_XML:
            export_xml(hist, title);
            break;
        case FORMAT_TEXT:
        default:
            display_histogram(hist, title);
            break;
    }

    /* Cleanup */
    histogram_destroy(hist);

    return 0;
}
