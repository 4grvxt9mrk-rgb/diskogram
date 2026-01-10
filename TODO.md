# TODO

## Future Feature Ideas

### Error Logging
Add optional parameter to log ALL errors to a logfile (not just the last error).

**Rationale**: Currently we only track `error_count` and `last_error`. When scanning large directory trees with many permission errors, it would be useful to see all errors that occurred, not just the most recent one.

**Implementation considerations**:
- Add `--error-log <filename>` or `--log-errors <filename>` flag
- Could be a simple text file with timestamp + error message
- Or structured format (JSON lines, CSV) for easier parsing
- Need to decide: append to existing log or overwrite?
- Performance: writing to file on every error vs buffering in memory
- Error handling: what if we can't write to the log file itself?

**Example usage**:
```bash
./diskogram --error-log errors.txt /path/to/scan
./diskogram --error-log errors.json --json /path/to/scan
```

**Related**: Could also add verbosity levels (`-v`, `-vv`) to control stderr output detail
