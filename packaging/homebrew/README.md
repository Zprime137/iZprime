# Homebrew Packaging

This directory contains a Homebrew formula template.

## Automated update from release tags

The repository workflow `.github/workflows/update-homebrew-formula.yml` updates
the tap formula automatically whenever a release is published.

Required repository secret in `Zprime137/iZprime`:

- `HOMEBREW_TAP_TOKEN`: PAT with write access to `Zprime137/homebrew-izprime`.

## Generate formula for a release

1. Compute SHA256 of the release asset:

```bash
curl -L https://github.com/Zprime137/iZprime/archive/refs/tags/v<version>.tar.gz -o /tmp/izprime.tar.gz
shasum -a 256 /tmp/izprime.tar.gz
```

2. Replace placeholders in `izprime.rb.in`:

- `@VERSION@` -> release version (for example `1.0.0`)
- `@SHA256@` -> computed hash

3. Publish generated `izprime.rb` in your Homebrew tap repository.
