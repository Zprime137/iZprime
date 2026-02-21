# Windows Packaging (MSYS2 + winget)

## Current support model

- Build target: MSYS2 UCRT64 toolchain
- Runtime: portable `izprime` package published as a GitHub release asset
- Installer channel: winget portable manifest (recommended first step)

## Portable artifact contents

- `izprime.exe`
- `libizprime-*.dll` (if built shared)
- required MinGW runtime DLLs (if needed)
- `LICENSE`
- `README` snippet with quick command examples

## winget publishing

1. Generate release artifact from CI.
2. Compute SHA256 for the published ZIP.
3. Fill manifest templates in this directory.
4. Submit to winget-pkgs repository.

Native MSI installer can be added later once packaging requirements stabilize.
