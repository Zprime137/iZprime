# Homebrew Packaging

This directory contains a Homebrew formula template.

## Generate formula for a release

1. Compute SHA256 of the release asset:

```bash
shasum -a 256 dist/izprime-<version>.tar.gz
```

2. Replace placeholders in `izprime.rb.in`:

- `@VERSION@` -> release version (for example `1.0.0`)
- `@SHA256@` -> computed hash

3. Publish generated `izprime.rb` in your Homebrew tap repository.
