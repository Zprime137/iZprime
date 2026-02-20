# Contributing to iZprime

Thanks for contributing. This project is prepared for research-grade publication, so we hold changes to a strict quality bar.

## Scope

Contributions are welcome for:

- correctness fixes,
- performance improvements,
- documentation quality,
- tests/benchmarks and tooling,
- new algorithms that align with the iZprime design.

## Development Setup

Required dependencies:

- C compiler (GCC/Clang),
- `make`,
- GMP,
- OpenSSL.

Optional:

- `pkg-config`,
- Doxygen,
- Python packages in `py_tools/requirements.txt` for plots.

## Code Style

- Keep modules focused and composable.
- Prefer explicit ownership and cleanup paths.
- Keep public API declarations in `include/`, implementations in `src/`.
- Document new public structs/functions with Doxygen comments.
- Avoid introducing hidden global state.

## Validation Requirements

Before opening a pull request:

```bash
make clean
make test-unit
make test-integration
make -- test-all --verbose
```

If you touched performance-sensitive code, also run:

```bash
make benchmark-p_sieve save-results
make benchmark-p_gen save-results
make benchmark-SiZ_count save-results
```

If you changed API/docs:

```bash
make userManual
```

## Benchmarking Expectations

- Include the exact command(s) used.
- Include hardware/OS/compiler context.
- Share result files from `output/` when relevant.
- Prefer reporting both absolute timing and relative speedup/regression.

## Pull Request Checklist

1. Explain the problem and the technical approach.
2. List behavioral changes and compatibility impact.
3. Link tests that validate the change.
4. Add or update benchmark evidence for performance claims.
5. Update docs (`README.md`, `docs/*.md`, Doxygen comments) when needed.

## Algorithm Contributions

For new sieve/generation techniques:

- map design to existing toolkit abstractions (`IZM`, `VX_SEG`, bitmaps, arrays),
- provide pseudocode-level explanation in `docs/pseudocode.pdf` source pipeline,
- add integration tests versus trusted baselines,
- add benchmark hooks so users can compare against existing models.

## Review Standard

PRs are reviewed for:

- correctness and edge-case behavior,
- memory/process safety,
- API clarity and consistency,
- test coverage,
- reproducibility of benchmark claims.
