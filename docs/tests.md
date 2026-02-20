# Tests

This document describes test targets, execution commands, and current pass snapshots.

## 1. Test Targets

From repository root:

```bash
make test-all
make test-unit
make test-integration
```

Verbose output:

```bash
make test-all verbose
make test-unit verbose
make test-integration verbose
```

GNU make passthrough form (dash flags):

```bash
make -- test-all --verbose
make -- test-unit --verbose
make -- test-integration --verbose
```

## 2. What Each Target Runs

- `test-unit`: bitmap/int-array/iZm/vx-seg module-level tests.
- `test-integration`: sieve hash integrity, range APIs, and prime-generation integration checks.
- `test-all`: unit + integration suites through the shared test runner.

## 3. Current Snapshot Results

Snapshot command set:

```bash
make test-unit
make test-integration
make test-all
```

Observed exit codes and summaries:

| Target                  | Exit code | Summary                                 |
| ----------------------- | --------: | --------------------------------------- |
| `make test-unit`        |         0 | 7/7 module groups passed (100.0%)       |
| `make test-integration` |         0 | 6/6 integration groups passed (100.0%)  |
| `make test-all`         |         0 | full test runner completed successfully |

Expected success markers in output:

- Unit suite: `[SUCCESS] ALL MODULE TESTS PASSED!`
- Integration suite: `[SUCCESS] ALL INTEGRATION TESTS PASSED!`

## 4. Show Results In Terminal

Quick pass/fail runs:

```bash
make test-unit
make test-integration
```

Detailed run (prints per-test information and hashes):

```bash
make test-integration verbose
```

Capture logs for review:

```bash
make test-unit > /tmp/test_unit.log 2>&1
make test-integration > /tmp/test_integration.log 2>&1
```

Tail the summaries:

```bash
tail -n 40 /tmp/test_unit.log
tail -n 50 /tmp/test_integration.log
```

## 5. Notes

- The integrity suite intentionally skips `SiZm_vy` in hash equality checks because it emits unordered prime lists.
- The test runner exits non-zero when any group fails; `make` surfaces that as a target failure.
