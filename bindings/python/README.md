# izprime (Python wrapper)

Python wrapper over `izprime_ffi` via `ctypes`.

## Prerequisites

- Built/installed `libizprime` shared library.
- If not in system loader path, set:

```bash
export IZPRIME_LIB=/absolute/path/to/libizprime.so
# macOS: libizprime.dylib
```

## Quick demo

```bash
PYTHONPATH=bindings/python python3 bindings/python/examples/demo.py
```
