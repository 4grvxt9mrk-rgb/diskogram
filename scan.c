#include "diskogram.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dirent.h>
    #include <sys/stat.h>
    #include <unistd.h>
#endif

#ifdef _WIN32

static time_t filetime_to_time_t(FILETIME ft) {
    ULARGE_INTEGER ull;
    ull.LowPart = ft.dwLowDateTime;
    ull.HighPart = ft.dwHighDateTime;
    return (time_t)(ull.QuadPart / 10000000ULL - 11644473600ULL);
}

static int scan_directory_win32(const char *path, grouping_mode_t mode, histogram_t *hist) {
    WIN32_FIND_DATAA find_data;
    HANDLE hFind;
    char search_path[MAX_PATH_LEN];
    char full_path[MAX_PATH_LEN];

    snprintf(search_path, sizeof(search_path), "%s\\*", path);

    hFind = FindFirstFileA(search_path, &find_data);
    if (hFind == INVALID_HANDLE_VALUE) {
        return -1;
    }

    do {
        if (strcmp(find_data.cFileName, ".") == 0 ||
            strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s\\%s", path, find_data.cFileName);

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            scan_directory_win32(full_path, mode, hist);
        } else {
            ULARGE_INTEGER file_size;
            file_size.LowPart = find_data.nFileSizeLow;
            file_size.HighPart = find_data.nFileSizeHigh;

            time_t file_time;
            switch (mode) {
                case GROUP_BY_MTIME:
                    file_time = filetime_to_time_t(find_data.ftLastWriteTime);
                    break;
                case GROUP_BY_CTIME:
                    file_time = filetime_to_time_t(find_data.ftCreationTime);
                    break;
                case GROUP_BY_ATIME:
                    file_time = filetime_to_time_t(find_data.ftLastAccessTime);
                    break;
                default:
                    file_time = filetime_to_time_t(find_data.ftLastWriteTime);
            }

            histogram_add_file(hist, file_time, file_size.QuadPart);
        }
    } while (FindNextFileA(hFind, &find_data) != 0);

    FindClose(hFind);
    return 0;
}

#else

static int scan_directory_posix(const char *path, grouping_mode_t mode, histogram_t *hist) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char full_path[MAX_PATH_LEN];

    dir = opendir(path);
    if (!dir) {
        perror(path);
        return -1;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(full_path, sizeof(full_path), "%s%s%s",
                 path, PATH_SEPARATOR_STR, entry->d_name);

        if (lstat(full_path, &st) != 0) {
            perror(full_path);
            continue;
        }

        if (S_ISDIR(st.st_mode)) {
            scan_directory_posix(full_path, mode, hist);
        } else if (S_ISREG(st.st_mode)) {
            time_t file_time;
            switch (mode) {
                case GROUP_BY_MTIME:
                    file_time = st.st_mtime;
                    break;
                case GROUP_BY_CTIME:
#ifdef __APPLE__
                    file_time = st.st_birthtime;
#else
                    file_time = st.st_ctime;
#endif
                    break;
                case GROUP_BY_ATIME:
                    file_time = st.st_atime;
                    break;
                default:
                    file_time = st.st_mtime;
            }

            histogram_add_file(hist, file_time, (uint64_t)st.st_size);
        }
    }

    closedir(dir);
    return 0;
}

#endif

int scan_directory(const char *path, grouping_mode_t mode, histogram_t *hist) {
#ifdef _WIN32
    return scan_directory_win32(path, mode, hist);
#else
    return scan_directory_posix(path, mode, hist);
#endif
}
