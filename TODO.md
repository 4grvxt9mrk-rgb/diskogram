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
