# Security and Robustness Review

Last reviewed: 2026-07-23

## Remediation Status

**Round 1** — 2026-08-05 (commit `1adacde`):

- ✅ **Fixed** — Quadratic CPU Consumption During Aggregation (hash index)
- ✅ **Fixed** — CSV Formula Injection (apostrophe prefix + RFC 4180 quoting)
- ✅ **Fixed** — Unchecked POSIX Path Truncation (checked `snprintf`)
- ⚠️ **Partially fixed** — Allocation and Counter Failure Handling (overflow-checked
  growth, checked `strdup`, allocation failure propagated to exit status; byte/counter
  wraparound and unbounded recursion still open)

**Round 2** — 2026-08-05:

- ✅ **Fixed** — Terminal Escape-Sequence Injection (`print_terminal_safe()` escapes
  control/DEL bytes for all attacker-controlled paths and error messages)
- ⚠️ **Partially fixed** — Invalid XML/Encoding Output (C0 control bytes illegal in
  XML 1.0 now replaced with U+FFFD; full invalid-UTF-8 validation still open)
- ✅ **Fixed** — `atoi()` partial numeric input (now `strtol` with full validation)
- ✅ **Fixed** — Batch JSON trailing comma (commas keyed to last non-empty item)
- ✅ **Fixed** — Stdin line truncation / CRLF (overlong lines rejected; `\r` stripped)

Verified warning-free under `-Wall -Wextra -std=c99` and clean under AddressSanitizer
and UBSan across the hash, batch, aggregate, and structured-export paths.

Still open: Path-Based Traversal Race (#4), Error-Log Symlink/Truncation (#7),
full UTF-8 validation, unbounded recursion + counter wraparound (#8 remainder),
hour/day UTC-boundary accuracy, and empty-results-hide-errors.

## Scope and Threat Model

This review covers `main.c`, `scan.c`, `histogram.c`, `display.c`, and
`export.c` in Diskogram 2.4.0.

Diskogram is a local command-line program. It does not process file contents,
listen on a network socket, invoke a shell, or execute scanned files. Its main
untrusted inputs are:

- command-line arguments and paths read from stdin;
- directory names, filenames, metadata, and concurrent filesystem changes;
- destinations selected for error logs;
- consumers of generated terminal, CSV, JSON, and XML output.

Risk is relatively low when Diskogram runs as an ordinary user against trusted
directories. Risk increases when it scans an attacker-writable tree, runs with
elevated privileges, consumes untrusted stdin, or has its output opened in a
terminal or spreadsheet application.

No direct command injection, format-string vulnerability, or conventional
stack/heap buffer overflow was identified. The issues below primarily affect
availability, output safety, traversal boundaries, and result integrity.

## Security Findings

### ✅ FIXED — Medium: Terminal Escape-Sequence Injection

Paths and filesystem error messages are printed directly to the terminal.
Attacker-controlled names can contain ANSI or OSC control sequences capable of
altering terminal state, forging displayed output, creating deceptive links,
or interacting with terminal features such as clipboard controls.

Affected areas:

- scan progress and titles in `main.c`;
- titles and last-error output in `display.c`;
- optional stderr error logging in `histogram.c`.

Recommended remediation:

- add a terminal-quoting function that escapes control and non-printable bytes
  as visible sequences such as `\x1b`;
- use the quoted representation for every externally supplied path or error
  message written to an interactive terminal.

### ✅ FIXED — Medium: CSV Formula Injection

Batch CSV output places a path in the first column. CSV quoting protects the
CSV structure, but it does not stop spreadsheet software from interpreting a
field beginning with `=`, `+`, `-`, or `@` as a formula.

For example, a relative input path of `=1+1` is emitted as:

```csv
=1+1,2026-07-23,1,1,1 B
```

Affected area: `export_csv_batch_item()` in `export.c`.

Recommended remediation:

- provide spreadsheet-safe CSV output that prefixes dangerous fields with an
  apostrophe or another documented neutralizing character;
- apply this after determining the logical field value and before CSV quoting;
- document a raw mode if preserving the exact path is also required.

### ✅ FIXED — Medium: Quadratic CPU Consumption During Aggregation

`histogram_add_file()` linearly scans every existing bucket for each file. An
attacker who can control file timestamps can give files distinct normalized
times and force approximately `O(files * buckets)`, or `O(files^2)`, work.

This can make scans of hostile trees consume excessive CPU, especially with
hourly buckets and timestamps spread over a large range.

Affected area: the bucket lookup loop in `histogram.c`.

Recommended remediation:

- aggregate into a hash table or balanced tree keyed by normalized `time_t`;
- convert the entries to the existing array and sort once during finalization;
- optionally impose file, bucket, duration, or resource limits for automated
  deployments.

### Low to Medium: Path-Based Traversal Race

The POSIX scanner classifies an entry with `lstat()` and later opens the same
pathname with `opendir()`. If an attacker can modify the scanned tree
concurrently, the checked directory can be replaced before it is opened.

Possible consequences include:

- traversal outside the intended directory;
- bypass of the one-filesystem assumption;
- blocking on an unexpected mount or provider;
- recursive cycles and stack exhaustion.

The program reads metadata rather than file contents, which limits
confidentiality impact, but the race is important for privileged or automated
scans of attacker-writable trees. The Windows check-and-recurse flow has a
similar pathname race around reparse-point checks.

Recommended remediation on POSIX:

- use descriptor-relative traversal with `openat()`, `fdopendir()`, and
  `fstatat()`;
- use `O_NOFOLLOW` and `AT_SYMLINK_NOFOLLOW` where available;
- validate the device and inode of the opened object;
- track visited `(st_dev, st_ino)` directory pairs to prevent cycles.

### ⚠️ PARTIALLY FIXED — Low: Invalid XML and Encoding Output

`print_xml_escaped()` escapes XML markup characters but emits other bytes
unchanged. XML 1.0 prohibits several control characters, including byte
`0x01`. A path containing such a byte produces malformed XML.

On POSIX systems, filenames may also contain byte sequences that are not valid
UTF-8. Emitting those bytes while declaring UTF-8 can invalidate XML and JSON
documents.

Affected areas: string escaping in `export.c`.

Recommended remediation:

- validate UTF-8 before emitting structured output;
- reject, replace, or encode invalid byte sequences;
- reject or encode characters prohibited by XML 1.0;
- add parser-based tests containing control characters and unusual filenames.

### ✅ FIXED — Low: Unchecked POSIX Path Truncation

The POSIX scanner constructs child paths with `snprintf()` but does not check
whether the result was truncated. This is not a direct buffer overflow:
`snprintf()` bounds the write. The danger is that a truncated pathname is then
treated as valid, potentially inspecting the wrong object or producing
misleading results.

The Windows scanner already checks the return value.

Affected area: `scan_directory_posix()` in `scan.c`.

Recommended remediation:

- check for a negative return value or a return value greater than or equal to
  the destination size;
- record a clear “path too long” error and skip that entry;
- prefer descriptor-relative traversal, which also avoids repeatedly building
  full paths.

### Low: Error-Log Symlink and Truncation Risk in Privileged Use

`--error-log` opens the selected filename with `fopen(path, "w")`. This follows
symlinks and truncates an existing file. That is ordinary behavior for an
interactive CLI whose user selects the destination, but it can become an
arbitrary-file truncation primitive if a privileged wrapper lets an untrusted
user control the log path.

Affected area: error-log setup in `main.c`.

Recommended remediation for privileged deployments:

- do not expose the log destination to less-privileged users;
- use `open()` with suitable flags such as `O_NOFOLLOW`, `O_CREAT`, and
  optionally `O_EXCL`;
- verify the opened descriptor with `fstat()` before passing it to `fdopen()`.

### ⚠️ PARTIALLY FIXED — Low: Allocation and Counter Failure Handling

Histogram capacity doubles without checking for `size_t` multiplication
overflow. Clang's static analyzer reports a theoretical zero-sized allocation
path after capacity overflow. Reaching it requires an impractically large
number of buckets, but the allocation should still use checked arithmetic.

Related robustness problems:

- `strdup()` is not checked for failure in structured batch mode;
- histogram growth failure prints an error but allows a successful exit with
  incomplete results;
- `total_bytes`, per-bucket byte totals, and counters can wrap silently;
- recursive traversal and the total scan size have no resource limits.

Recommended remediation:

- check `capacity > SIZE_MAX / 2` and allocation-size multiplication before
  calling `realloc()`;
- check every allocation result before storing or dereferencing it;
- propagate allocation failures to `main()` and return a nonzero status;
- use checked addition for byte and count totals;
- consider iterative traversal and configurable resource limits.

## Input-Validation and Correctness Issues

These issues should be fixed, but they are not currently classified as
security vulnerabilities on their own.

### ✅ FIXED — `atoi()` Accepts Partially Numeric Input

`--last 12x days` is silently accepted as 12 days, while overflow behavior is
not reliably diagnosable.

Replace `atoi()` with `strtol()` or `strtoimax()` and validate:

- that at least one digit was consumed;
- that the entire argument was consumed;
- `errno != ERANGE`;
- that the value is positive and fits the destination type;
- that it is within a reasonable operational maximum.

### Calendar Arithmetic Is Not Inherently Incorrect

The previous version of this report claimed that subtracting from `struct tm`
necessarily mishandles leap years and varying month lengths. That claim was
misleading: `mktime()` is specifically designed to normalize out-of-range
calendar fields.

The code should still define and test its intended behavior around:

- daylight-saving transitions;
- month-end normalization;
- extreme filter values;
- failures or range limits in `mktime()`.

### Hour and Day Buckets Use UTC-Like Boundaries

Hour and day normalization divides the Unix timestamp by fixed seconds, while
the result is formatted in local time. In non-UTC time zones, particularly
around daylight-saving transitions, displayed buckets may not align with local
hour or midnight boundaries. This affects result accuracy rather than security.

### ✅ FIXED — Batch JSON Can Contain a Trailing Comma

Empty histograms are skipped by `export_json_array_item()`, but `main.c`
decides whether an item is last using the unfiltered array index. If the final
stored histogram is empty, the preceding emitted item retains a trailing comma
and the JSON is invalid.

Track emitted items and place commas between actual output items, or filter
empty histograms before serialization.

### Empty Results Can Hide Errors

Some display/export paths return early when there are no buckets. A completely
failed or filtered scan can therefore suppress useful scan metadata and error
details. Structured formats should still emit a valid empty result with its
error count and scan metadata.

### ✅ FIXED — Stdin Lines Can Be Truncated

`fgets()` reads at most `MAX_PATH_LEN - 1` bytes. An overlong input line is
processed as multiple paths rather than rejected as one overlong path.
Additionally, CRLF input retains the trailing carriage return.

Detect missing newlines, discard the remainder of an overlong line, report an
error, and strip both `\n` and `\r`.

## Verification Performed

The review included:

- manual inspection of all C source files;
- compilation with AddressSanitizer and UndefinedBehaviorSanitizer;
- Clang static analysis;
- stricter compiler warnings for formats, conversions, shadowing, prototypes,
  and undefined macros;
- targeted tests for malformed numeric input, batch JSON, CSV formulas, and
  XML control characters.

No sanitizer-detected memory corruption occurred in the exercised paths.
Static analysis identified the theoretical histogram allocation-size overflow
described above.

## Recommended Remediation Order

1. Neutralize terminal control sequences and spreadsheet formulas.
2. Replace linear bucket lookup to prevent quadratic CPU use.
3. Check POSIX path construction and all allocation results.
4. Make structured output valid for empty, unusual, and hostile inputs.
5. Replace pathname-based recursion with descriptor-relative traversal.
6. Harden privileged error-log creation if privileged use is supported.
7. Add automated regression tests and sanitizer/static-analysis CI jobs.
