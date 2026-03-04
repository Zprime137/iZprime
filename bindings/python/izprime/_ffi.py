"""Low-level ctypes bindings for izprime_ffi.h."""

from __future__ import annotations

import ctypes
import ctypes.util
import os
import platform
from pathlib import Path


class IzpU64Buffer(ctypes.Structure):
    _fields_ = [
        ("data", ctypes.POINTER(ctypes.c_uint64)),
        ("len", ctypes.c_size_t),
    ]


# Backward-compatibility alias for older wrapper imports.
IZP_U64_BUFFER = IzpU64Buffer


def _library_candidates() -> list[str]:
    env_path = os.environ.get("IZPRIME_LIB")
    candidates: list[str] = []
    if env_path:
        candidates.append(env_path)

    repo_root = Path(__file__).resolve().parents[3]
    build_lib = repo_root / "build" / "lib"

    system = platform.system()
    if system == "Windows":
        candidates.extend(
            [
                str(build_lib / "libizprime.dll"),
                str(build_lib / "izprime.dll"),
            ]
        )
    else:
        ext = "dylib" if system == "Darwin" else "so"
        candidates.append(str(build_lib / f"libizprime.{ext}"))
        for p in sorted(build_lib.glob(f"libizprime*.{ext}*")):
            candidates.append(str(p))

    libname = ctypes.util.find_library("izprime")
    if libname:
        candidates.append(libname)

    return candidates


def load_library() -> ctypes.CDLL:
    errors: list[str] = []
    for candidate in _library_candidates():
        try:
            lib = ctypes.CDLL(candidate)
            _configure_signatures(lib)
            return lib
        except OSError as exc:
            errors.append(f"{candidate}: {exc}")

    err = "\n".join(errors) if errors else "no candidates"
    raise OSError(
        "Unable to load libizprime. Build/install the shared library or set IZPRIME_LIB."
        f"\nTried:\n{err}"
    )


def _configure_signatures(lib: ctypes.CDLL) -> None:
    lib.izp_ffi_version.restype = ctypes.c_char_p
    lib.izp_ffi_version.argtypes = []

    lib.izp_ffi_status_message.restype = ctypes.c_char_p
    lib.izp_ffi_status_message.argtypes = [ctypes.c_int]

    lib.izp_ffi_last_error.restype = ctypes.c_char_p
    lib.izp_ffi_last_error.argtypes = []

    lib.izp_ffi_clear_error.restype = None
    lib.izp_ffi_clear_error.argtypes = []

    lib.izp_ffi_sieve_u64.restype = ctypes.c_int
    lib.izp_ffi_sieve_u64.argtypes = [ctypes.c_int, ctypes.c_uint64, ctypes.POINTER(IzpU64Buffer)]

    lib.izp_ffi_count_range.restype = ctypes.c_int
    lib.izp_ffi_count_range.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint64,
        ctypes.c_int,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_uint64),
    ]

    lib.izp_ffi_stream_range.restype = ctypes.c_int
    lib.izp_ffi_stream_range.argtypes = [
        ctypes.c_char_p,
        ctypes.c_uint64,
        ctypes.c_int,
        ctypes.c_char_p,
        ctypes.c_int,
        ctypes.POINTER(ctypes.c_uint64),
    ]

    lib.izp_ffi_next_prime.restype = ctypes.c_int
    lib.izp_ffi_next_prime.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]

    lib.izp_ffi_random_prime_vx.restype = ctypes.c_int
    lib.izp_ffi_random_prime_vx.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]

    lib.izp_ffi_random_prime_vy.restype = ctypes.c_int
    lib.izp_ffi_random_prime_vy.argtypes = [ctypes.c_int, ctypes.c_int, ctypes.POINTER(ctypes.c_char_p)]

    lib.izp_ffi_free_u64_buffer.restype = None
    lib.izp_ffi_free_u64_buffer.argtypes = [ctypes.POINTER(IzpU64Buffer)]

    lib.izp_ffi_free_string.restype = None
    lib.izp_ffi_free_string.argtypes = [ctypes.POINTER(ctypes.c_char_p)]
