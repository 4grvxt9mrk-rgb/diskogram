# Diskogram

A cross-platform command-line tool for visualizing disk space consumption over time. Diskogram generates histograms that show how files in a directory tree are distributed across time periods based on their modification, creation, or access dates.

## Features

- Generate bar graph histograms of disk space grouped by date
- Three grouping modes: modification time, creation time, and access time
- Four time interval granularities: hour, day, month, and year
- Multiple export formats: terminal output (default), CSV, JSON, and XML
- **Time window filter**: Restrict output to the last N hours/days/months/years (`--last`)
- **Filesystem boundary control**: Stays on one filesystem by default to avoid hangs on cloud/network mounts; opt out with `--follow-mounts`
- **Stdin support**: Read paths from pipes following Unix philosophy (aggregate or batch mode)
- Portable C code that runs on macOS, Linux, FreeBSD, and Windows
- Recursive directory scanning with robust error handling
- Human-readable size formatting (B, KB, MB, GB, etc.)
- Comprehensive metadata in all export formats: scan timing, error counts, filter windows
- Detailed error logging to file or stderr with timestamps

## Building

### Prerequisites

- C compiler (gcc, clang, or MSVC)
- make (on Unix-like systems)

### Compilation

On macOS, Linux, or FreeBSD:

```bash
make
```

On Windows (using MinGW or similar):

```bash
make
```

Or with MSVC:

```bash
cl /O2 /W3 main.c scan.c histogram.c display.c export.c /Fe:diskogram.exe
```

## Usage

```
diskogram [OPTIONS] <directory>
   or: diskogram [OPTIONS] --stdin
```

### Options

#### Time Grouping Options
- `-m, --mtime` - Group by modification time (default)
- `-c, --ctime` - Group by creation time (macOS/BSD) or change time (Linux)
- `-a, --atime` - Group by access time

#### Interval Options
- `--hour` - Group by hour
- `--day` - Group by day (default)
- `--month` - Group by month
- `--year` - Group by year

#### Export Format Options
- `--csv` - Export as CSV format
- `--json` - Export as JSON format
- `--xml` - Export as XML format
- (default) - Display as bar graph in terminal

#### Filter Options
- `--last <N> <unit>` - Only include files from the last N time units. Units: `hours`, `days`, `months`, `years` (singular forms also accepted). Applied during scan — old files are never loaded into memory.
- `--follow-mounts` - Cross filesystem boundaries. By default diskogram stays on one filesystem to prevent hangs caused by iCloud Drive, FUSE mounts, NFS shares, or other virtual/network filesystems.

#### Error Logging Options
- `--error-log <file>` - Log all errors to specified file with timestamps
- `--log-errors-stderr` - Log all errors to stderr with timestamps

#### Stdin Options
- `--stdin` - Read directory paths from stdin (one per line)
- `--batch` - Output a separate histogram per path (requires `--stdin`). Without `--batch`, all paths are aggregated into one histogram.

#### Other Options
- `-h, --help` - Show help message
- `--version` - Show version information

### Examples

Analyze current directory by modification time (default):
```bash
diskogram .
```

Analyze a specific directory by creation time, grouped by month:
```bash
diskogram -c --month /path/to/directory
```

Audit files accessed in the last 30 days:
```bash
diskogram --atime --last 30 days /home/user
```

Show disk usage for files modified in the last 6 months, grouped by month:
```bash
diskogram --month --last 6 months /var/log
```

Show files from the last year, export as JSON:
```bash
diskogram --last 1 year --json --month ~/Documents
```

Export as CSV for import into a spreadsheet:
```bash
diskogram --csv /data > report.csv
```

Scan an NFS share or network mount (disable filesystem boundary check):
```bash
diskogram --follow-mounts /mnt/nas
```

Log all errors to a file while scanning:
```bash
diskogram --error-log errors.txt /var
```

### Stdin / Pipe Support

Aggregate multiple paths into one histogram:
```bash
find /var/log -type d | diskogram --stdin
echo -e "/home\n/var\n/tmp" | diskogram --stdin --json
```

Process each path separately (batch mode):
```bash
find ~ -maxdepth 1 -type d | diskogram --stdin --batch
cat paths.txt | diskogram --stdin --batch --csv
```

Combine with other Unix tools:
```bash
locate "*.log" -0 | xargs -0 dirname | sort -u | diskogram --stdin --month
```

## Sample Output

### Terminal Output (Default)

```
Scanning '/Users/username/Documents'...

Disk Space by Modification Time: /Users/username/Documents
Total: 2.34 GB in 1523 files (45 directories scanned)

2026-01-05  ####################  456.78 MB (234 files)
2026-01-09  ##################################################  1.72 GB (1199 files)
```

With a time window filter (`--last 30 days`):
```
Scanning '/var/log'...

Disk Space by Modification Time: /var/log
Window: last 30 days (since 2026-02-21)
Total: 523.45 MB in 234 files (15 directories scanned)

2026-02-21  ####################  234.56 MB (123 files)
2026-03-01  ##################################################  288.89 MB (111 files)
```

When errors are encountered (e.g., permission denied), diskogram continues scanning and shows a warning:
```
WARNING: 3 error(s) occurred during scan
Last error: Cannot open directory: /var/log/private
Results may be incomplete.
```

### CSV Output

```csv
# Disk Space by Modification Time: /Users/username/Documents
# Version: 2.4.0
# Scan Duration: 3 seconds
# Directories Scanned: 45
# Errors: 0
# Filter: last 30 days (since 2026-02-21)
Time,Bytes,Files,Human-Readable Size
2026-02-21,47472640,23,45.23 MB
2026-03-01,129438720,67,123.45 MB
```

### JSON Output

JSON output always returns an array for consistency across all modes (single scan, stdin aggregate, and batch).

```json
[
  {
    "version": "2.4.0",
    "title": "Disk Space by Modification Time: /Users/username/Documents",
    "total_bytes": 2502534144,
    "total_files": 1523,
    "interval": "day",
    "scan_start": "2026-03-23T14:23:15",
    "scan_end": "2026-03-23T14:23:18",
    "scan_duration_seconds": 3,
    "directories_scanned": 45,
    "error_count": 0,
    "filter_last_n": 30,
    "filter_unit": "days",
    "filter_since": "2026-02-21T14:23:15",
    "buckets": [
      {
        "time": "2026-02-21",
        "bytes": 47472640,
        "files": 23
      },
      {
        "time": "2026-03-01",
        "bytes": 129438720,
        "files": 67
      }
    ]
  }
]
```

The `filter_last_n`, `filter_unit`, and `filter_since` fields are omitted when no `--last` filter is active.

### XML Output

XML output always uses a `<histograms>` root wrapper for consistency across all modes.

```xml
<?xml version="1.0" encoding="UTF-8"?>
<histograms>
  <histogram>
    <version>2.4.0</version>
    <title>Disk Space by Modification Time: /Users/username/Documents</title>
    <total_bytes>2502534144</total_bytes>
    <total_files>1523</total_files>
    <interval>day</interval>
    <scan_start>2026-03-23T14:23:15</scan_start>
    <scan_end>2026-03-23T14:23:18</scan_end>
    <scan_duration_seconds>3</scan_duration_seconds>
    <directories_scanned>45</directories_scanned>
    <error_count>0</error_count>
    <filter_last_n>30</filter_last_n>
    <filter_unit>days</filter_unit>
    <filter_since>2026-02-21T14:23:15</filter_since>
    <buckets>
      <bucket>
        <time>2026-02-21</time>
        <bytes>47472640</bytes>
        <files>23</files>
      </bucket>
    </buckets>
  </histogram>
</histograms>
```

## Filesystem Boundary Behaviour

By default diskogram will not cross filesystem boundaries during a recursive scan. This prevents indefinite hangs that occur when a directory tree contains:

- **iCloud Drive** (`~/Library/Mobile Documents`) — accessing cloud-only files triggers a network download that blocks `opendir()`
- **CloudStorage mounts** (`~/Library/CloudStorage`) — Dropbox, OneDrive, Google Drive integrations
- **NFS / SMB shares** — network latency or unreachable servers cause `opendir()` to block with 0 CPU and 0 disk activity
- **FUSE filesystems** — virtual filesystems with unpredictable performance

On macOS, diskogram additionally skips FileProvider directories (identifiable by `st_nlink == 65535`) that share a device ID with the host APFS volume but still block on open.

To intentionally scan across mount points (e.g., a local NAS you know is responsive):
```bash
diskogram --follow-mounts /mnt/nas
```

On Windows, junction points and volume mount points are already skipped via `FILE_ATTRIBUTE_REPARSE_POINT` — `--follow-mounts` is accepted but has no effect.

## Error Logging

Diskogram continues scanning even when it encounters permission errors or inaccessible directories. Error counts are reported in all output formats.

For a full error log with timestamps:
```bash
diskogram --error-log errors.txt /var
```

```
[2026-03-23 16:22:06] Cannot open directory: /var/install
[2026-03-23 16:22:06] Cannot open directory: /var/spool/mqueue
```

Log to stderr (useful for real-time monitoring):
```bash
diskogram --log-errors-stderr /var 2>errors.log
```

Both can be combined:
```bash
diskogram --error-log full.txt --log-errors-stderr /var
```

## Platform Notes

### macOS
- Uses `st_birthtime` for true creation time with `-c`
- Default filesystem boundary check also detects FileProvider virtual directories (`st_nlink == 65535`)

### Linux
- `-c` shows inode change time (not birth time — Linux doesn't reliably store birth time across all filesystems)
- Requires glibc or musl

### FreeBSD
- Full support for all three time modes using standard BSD stat structures

### Windows
- Uses Windows API (`FindFirstFile`/`FindNextFile`) for directory traversal
- Junction points and volume mount points are skipped via `FILE_ATTRIBUTE_REPARSE_POINT`
- `--follow-mounts` is accepted but has no effect (reparse point skipping handles mount boundaries)
- Compile with MinGW (`make`) or MSVC (`cl /O2 /W3 *.c /Fe:diskogram.exe`)
- All integer sizes use `PRIu64` for correct 64-bit output (no truncation on values >4 GB)

## Architecture

- `main.c` — Command-line parsing and program entry point
- `scan.c` — Cross-platform directory traversal; POSIX `st_dev` boundary check; Windows reparse point skip
- `histogram.c` — Time bucket management, data aggregation, cutoff filtering
- `display.c` — Terminal bar graph rendering with window header
- `export.c` — CSV, JSON, and XML export with filter metadata fields
- `diskogram.h` — Shared definitions, struct declarations, function prototypes

Platform-specific code is isolated with `#ifdef _WIN32` / `#else` preprocessor blocks.

## Use Cases

- **Disk cleanup planning** — Identify when large amounts of data were added
- **Compliance and auditing** — `--last 90 days --atime` to find recently accessed files for audit trails
- **Project timeline analysis** — Visualize when files were created during development
- **Capacity planning** — Export historical data to CSV/JSON for trend analysis
- **Automated reporting** — JSON/XML output for integration with monitoring pipelines

## Version History

| Version | Changes |
|---------|---------|
| 2.4.0 | `--last N unit` time window filter; `--one-file-system` default ON with `--follow-mounts` opt-out |
| 2.2.0 | Fix 64-bit integer printing on Windows (`PRIu64`) |
| 2.1.0 | JSON/XML always use array/collection wrapper; `Path` column in batch CSV |
| 2.0.0 | Stdin support (`--stdin`, `--batch`) |
| 1.3.0 | Error logging (`--error-log`, `--log-errors-stderr`) |
| 1.2.0 | Scan metadata (timing, directory counts, error tracking) in all formats |
| 1.0.0 | Initial release |

## License

This software is free to use, modify, and distribute for any purpose, including commercial use, with attribution required.

**Required attribution:** "diskogram" by Jethro Rose, with Claude (Anthropic)

See the [LICENSE](LICENSE) file for full details.

## Contributing

Contributions are welcome. When adding features or fixing bugs, please ensure the code remains portable across all supported platforms.
