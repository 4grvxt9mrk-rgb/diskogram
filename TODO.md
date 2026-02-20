# TODO

## Completed Features

### ✅ Error Logging (Implemented in v1.3.0)
~~Add optional parameter to log ALL errors to a logfile (not just the last error).~~

**Status**: Implemented with `--error-log <file>` and `--log-errors-stderr` flags
- Simple text file format with timestamps: `[YYYY-MM-DD HH:MM:SS] error message`
- File is overwritten (not appended) on each run
- Errors are flushed immediately (no buffering) for reliability
- Both file and stderr logging can be used simultaneously
- Works across all platforms (macOS, Linux, FreeBSD, Windows)

## Future Feature Ideas

### Verbosity Levels
Add verbosity flags (`-v`, `-vv`, `-vvv`) to control output detail:
- `-v`: Show progress (directories as they're scanned)
- `-vv`: Show progress + statistics (files per second, etc.)
- `-vvv`: Debug mode (show every file processed)

**Use case**: Monitor long-running scans, troubleshoot performance issues

### Incremental/Differential Scans
Save scan results and compare against previous runs:
- `--save-state state.json`: Save full scan results
- `--compare-with state.json`: Show what changed since last scan
- Highlight new files, deleted files, size changes

**Use case**: Track disk usage growth over time, identify what's consuming space

### Filtering Options
Add filters to exclude/include specific files:
- `--exclude-pattern '*.log'`: Skip files matching glob patterns
- `--min-size 1M`: Only scan files larger than threshold
- `--max-depth 3`: Limit directory recursion depth

**Use case**: Focus on specific file types, avoid temporary files, performance optimization

## Issues Found

### ✅ 64-bit Size Printing Incorrect on Windows (Fixed)
~~Several outputs print `uint64_t` values with `%lu`, which is 32-bit on Windows and can overflow for sizes >4GB.~~

**Fixed**: Replaced all `(unsigned long)` casts + `%lu` with `PRIu64` from `<inttypes.h>` in `display.c` and `export.c`.

### JSON Batch Output Can Be Invalid If Last Histogram Is Empty
`export_json_array_item` skips empty histograms, but `main.c` computes `is_last` based on index, so a trailing comma can be emitted.

**Files**: `main.c`, `export.c`  
**Fix**: Track printed items and only emit commas between actual items, or pre-filter non-empty histograms.

### POSIX Path Truncation Not Checked
`scan_directory_posix` uses `snprintf` without checking for truncation, which can lead to incorrect `lstat` calls and misleading errors.

**File**: `scan.c`  
**Fix**: Check `snprintf` return value and log a “path too long” error similar to the Win32 branch.
