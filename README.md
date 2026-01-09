# Diskogram

A cross-platform command-line tool for visualizing disk space consumption over time. Diskogram generates histograms that show how files in a directory tree are distributed across time periods based on their modification, creation, or access dates.

## Features

- Generate bar graph histograms of disk space grouped by date
- Three grouping modes: modification time, creation time, and access time
- Four time interval granularities: hour, day, month, and year
- Multiple export formats: terminal output (default), CSV, JSON, and XML
- Portable C code that runs on macOS, Linux, FreeBSD, and Windows
- Recursive directory scanning
- Human-readable size formatting (B, KB, MB, GB, etc.)

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

#### Other Options
- `-h, --help` - Show help message

### Examples

Analyze current directory by modification time (default):
```bash
./diskogram .
```

Analyze a specific directory by creation time, grouped by month:
```bash
./diskogram -c --month /path/to/directory
```

Analyze by access time, grouped by year, export as JSON:
```bash
./diskogram --atime --year --json ~/Documents
```

Export as CSV for import into spreadsheet:
```bash
./diskogram --csv /data > report.csv
```

Generate XML for automated reporting:
```bash
./diskogram --month --xml /var/log > monthly_report.xml
```

## Sample Output

### Terminal Output (Default)

```
Scanning '/Users/username/Documents'...

Disk Space by Modification Time: /Users/username/Documents
Total: 2.34 GB in 1,523 files

2025-12-15  ###                 45.23 MB (23 files)
2025-12-20  ########            123.45 MB (67 files)
2026-01-05  ####################  456.78 MB (234 files)
2026-01-09  ##################################################  1.72 GB (1199 files)
```

### CSV Output

```csv
# Disk Space by Modification Time: /Users/username/Documents
Time,Bytes,Files,Human-Readable Size
2025-12-15,47472640,23,45.23 MB
2025-12-20,129438720,67,123.45 MB
2026-01-05,478752768,234,456.78 MB
2026-01-09,1846870016,1199,1.72 GB
```

### JSON Output

```json
{
  "title": "Disk Space by Modification Time: /Users/username/Documents",
  "total_bytes": 2502534144,
  "total_files": 1523,
  "interval": "day",
  "buckets": [
    {
      "time": "2025-12-15",
      "bytes": 47472640,
      "files": 23
    },
    {
      "time": "2025-12-20",
      "bytes": 129438720,
      "files": 67
    }
  ]
}
```

### XML Output

```xml
<?xml version="1.0" encoding="UTF-8"?>
<histogram>
  <title>Disk Space by Modification Time: /Users/username/Documents</title>
  <total_bytes>2502534144</total_bytes>
  <total_files>1523</total_files>
  <interval>day</interval>
  <buckets>
    <bucket>
      <time>2025-12-15</time>
      <bytes>47472640</bytes>
      <files>23</files>
    </bucket>
    <bucket>
      <time>2025-12-20</time>
      <bytes>129438720</bytes>
      <files>67</files>
    </bucket>
  </buckets>
</histogram>
```

## Platform Notes

### macOS
- Uses `st_birthtime` for true creation time when using `-c` flag
- Supports all modern macOS versions

### Linux
- The `-c` flag shows change time (inode modification) rather than creation time, as Linux doesn't reliably store creation time in all filesystems
- Requires glibc or musl

### FreeBSD
- Full support for all three time modes
- Uses standard BSD stat structures

### Windows
- Uses Windows API for directory traversal and file metadata
- Creation time is always available
- Compile with MinGW or MSVC

## Architecture

The codebase is organized into modular components:

- `main.c` - Command-line parsing and program entry point
- `scan.c` - Cross-platform directory traversal and file metadata collection
- `histogram.c` - Time bucket management and data aggregation with configurable intervals
- `display.c` - Terminal output and bar graph rendering
- `export.c` - CSV, JSON, and XML export functionality
- `diskogram.h` - Common definitions and function declarations

Platform-specific code is isolated using `#ifdef` preprocessor directives, with separate implementations for POSIX (macOS/Linux/FreeBSD) and Windows systems.

## Use Cases

- **Disk cleanup planning**: Identify when large amounts of data were added to plan cleanup strategies
- **Project timeline analysis**: Visualize when files were created during project development
- **Compliance and auditing**: Track file access patterns for security audits
- **Capacity planning**: Export historical data to spreadsheets for trend analysis
- **Automated reporting**: Generate JSON/XML for integration with monitoring systems

## License

This is free and unencumbered software released into the public domain.

## Contributing

Contributions are welcome. When adding features or fixing bugs, please ensure the code remains portable across all supported platforms.
