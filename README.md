# Spacetime

A cross-platform command-line tool for visualizing disk space consumption over time. Spacetime generates histograms that show how files in a directory tree are distributed across time periods based on their modification, creation, or access dates.

## Features

- Generate bar graph histograms of disk space grouped by date
- Three grouping modes: modification time, creation time, and access time
- Portable C code that runs on macOS, Linux, FreeBSD, and Windows
- Recursive directory scanning
- Human-readable size formatting (B, KB, MB, GB, etc.)
- Daily time bucket granularity

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
cl /O2 /W3 main.c scan.c histogram.c display.c /Fe:spacetime.exe
```

## Usage

```
spacetime [OPTIONS] <directory>
```

### Options

- `-m, --mtime` - Group by modification time (default)
- `-c, --ctime` - Group by creation time (macOS/BSD) or change time (Linux)
- `-a, --atime` - Group by access time
- `-h, --help` - Show help message

### Examples

Analyze current directory by modification time:
```bash
./spacetime .
```

Analyze a specific directory by creation time:
```bash
./spacetime -c /path/to/directory
```

Analyze by access time:
```bash
./spacetime --atime ~/Documents
```

## Sample Output

```
Scanning '/Users/username/Documents'...

Disk Space by Modification Time: /Users/username/Documents
Total: 2.34 GB in 1,523 files

2025-12-15  ###                 45.23 MB (23 files)
2025-12-20  ########            123.45 MB (67 files)
2026-01-05  ####################  456.78 MB (234 files)
2026-01-09  ##################################################  1.72 GB (1199 files)
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
- `histogram.c` - Time bucket management and data aggregation
- `display.c` - Terminal output and bar graph rendering
- `spacetime.h` - Common definitions and function declarations

Platform-specific code is isolated using `#ifdef` preprocessor directives, with separate implementations for POSIX (macOS/Linux/FreeBSD) and Windows systems.

## License

This is free and unencumbered software released into the public domain.

## Contributing

Contributions are welcome. When adding features or fixing bugs, please ensure the code remains portable across all supported platforms.
