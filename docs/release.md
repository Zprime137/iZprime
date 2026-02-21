# Releasing iZprime

This document defines the release workflow for `v1.x` and later.

## Versioning

- Use SemVer tags: `vMAJOR.MINOR.PATCH` (example: `v1.0.0`).
- `VERSION` in `Makefile` should match the release tag.
- `SOVERSION` defaults to the major version and should change only on ABI breaks.

## Pre-release Gate

Run this checklist from a clean working tree:

```bash
make clean
make test-unit
make test-integration
make benchmark-p_sieve save-results
make benchmark-p_gen save-results
make benchmark-SiZ_count save-results
make userManual
make dist VERSION=1.0.0
```

Confirm:

- generated `docs/userManual.pdf` opens correctly,
- benchmark outputs are saved in `output/`,
- `dist/` contains tarball + checksum,
- release tarball does not include transient artifacts (`build/`, `logs/`, `output/`, `.git/`).

Windows readiness (recommended for each release):

- run CI builds on `windows-latest` (MSYS2/MinGW toolchain),
- run at least `make doctor`, `make test-unit`, and `make test-integration`,
- confirm fallback behavior for multi-process APIs on non-fork platforms.

Native package metadata is maintained under `packaging/`:

- `packaging/debian/` (Debian/Ubuntu),
- `packaging/rpm/izprime.spec` (Fedora/RHEL),
- `packaging/homebrew/` (Homebrew formula template),
- `packaging/windows/` (portable + winget templates).

## Changelog and Release Notes

`ChangeLog` should summarize user-visible changes.

Suggested generation workflow:

```bash
LAST_TAG=$(git describe --tags --abbrev=0 2>/dev/null || echo "")
if [ -n "$LAST_TAG" ]; then
  git log --pretty=format:"- %s (%h)" "$LAST_TAG"..HEAD
else
  git log --pretty=format:"- %s (%h)"
fi
```

Use the output as a draft, then curate into sections:

- Added
- Changed
- Fixed
- Performance
- Documentation

Release notes in GitHub/GitLab should include:

- highlights,
- migration notes (if any),
- validation commands used,
- benchmark artifacts summary.

## Tag and Publish

```bash
git tag -a v1.0.0 -m "iZprime v1.0.0"
git push origin v1.0.0
```

Attach `dist/izprime-<version>.tar.gz` and checksum file to the release.

## Post-release

- bump `VERSION` to next development version (for example `1.0.1-dev`),
- update docs if commands/artifacts changed,
- track follow-up fixes as issues/milestone items.
