# Native Package Plan

This document tracks distro-native packaging targets for iZprime.

## Targets

- macOS: Homebrew tap formula
- Debian/Ubuntu: `.deb` packages
- Fedora/RHEL: `.rpm` package
- Windows: MSYS2 build + portable ZIP + winget manifest

## Metadata Locations

- Homebrew template: `packaging/homebrew/izprime.rb.in`
- Debian metadata: `packaging/debian/`
- RPM spec: `packaging/rpm/izprime.spec`
- Windows packaging templates: `packaging/windows/`

## Package Split (Linux)

- `izprime`: CLI binary
- `libizprime1`: runtime shared library
- `libizprime-dev`: headers, static lib, pkg-config file

## Release Flow

1. Tag release and publish source artifact + checksum.
2. Build platform-native packages from the release tarball.
3. Run post-install smoke checks:
   - `izprime doctor`
   - one CLI functional command (for example `izprime count_primes --range "[0, 100000]"`)
4. Publish packages to distribution channel (tap/PPA/COPR/winget).

## CI Gates

- Linux/macOS/Windows build and test matrix in `.github/workflows/ci.yml`.
- Minimum required checks per platform:
  - `make doctor`
  - `make cli`
  - `make test-unit`
  - `make test-integration`
