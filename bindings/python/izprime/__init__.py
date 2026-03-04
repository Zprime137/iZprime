"""Python wrapper for iZprime FFI."""

from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum
import ctypes

from ._ffi import IZP_U64_BUFFER, load_library


class Status(IntEnum):
    OK = 0
    INVALID_ARG = 1
    PARSE = 2
    ALLOC = 3
    NOT_FOUND = 4
    OPERATION = 5


class SieveKind(IntEnum):
    SOE = 0
    SSOE = 1
    SOS = 2
    SSOS = 3
    SOEU = 4
    SOA = 5
    SIZ = 6
    SIZM = 7
    SIZM_VY = 8


@dataclass
class IzprimeError(Exception):
    status: int
    message: str

    def __str__(self) -> str:
        return f"iZprime error [{self.status}]: {self.message}"


class Izprime:
    def __init__(self) -> None:
        self._lib = load_library()

    @property
    def version(self) -> str:
        return self._lib.izp_ffi_version().decode("utf-8")

    def status_message(self, status: int) -> str:
        return self._lib.izp_ffi_status_message(int(status)).decode("utf-8")

    def last_error(self) -> str:
        raw = self._lib.izp_ffi_last_error()
        return raw.decode("utf-8") if raw else ""

    def sieve(self, kind: SieveKind, limit: int) -> list[int]:
        out = IZP_U64_BUFFER()
        status = self._lib.izp_ffi_sieve_u64(int(kind), int(limit), ctypes.byref(out))
        self._raise_if_error(status)
        try:
            if out.len == 0 or not out.data:
                return []
            return [out.data[i] for i in range(out.len)]
        finally:
            self._lib.izp_ffi_free_u64_buffer(ctypes.byref(out))

    def count_range(self, start: str, range_size: int, mr_rounds: int = 30, cores: int = 1) -> int:
        out_count = ctypes.c_uint64(0)
        status = self._lib.izp_ffi_count_range(
            start.encode("utf-8"),
            int(range_size),
            int(mr_rounds),
            int(cores),
            ctypes.byref(out_count),
        )
        self._raise_if_error(status)
        return int(out_count.value)

    def stream_range(
        self,
        start: str,
        range_size: int,
        filepath: str,
        mr_rounds: int = 30,
        stream_gaps: bool = False,
    ) -> int:
        out_count = ctypes.c_uint64(0)
        status = self._lib.izp_ffi_stream_range(
            start.encode("utf-8"),
            int(range_size),
            int(mr_rounds),
            filepath.encode("utf-8"),
            1 if stream_gaps else 0,
            ctypes.byref(out_count),
        )
        self._raise_if_error(status)
        return int(out_count.value)

    def next_prime(self, base_expr: str, forward: bool = True) -> int:
        out = ctypes.c_char_p()
        status = self._lib.izp_ffi_next_prime(base_expr.encode("utf-8"), 1 if forward else 0, ctypes.byref(out))
        self._raise_if_error(status)
        try:
            return int(out.value.decode("utf-8"))
        finally:
            self._lib.izp_ffi_free_string(ctypes.byref(out))

    def random_prime_vx(self, bit_size: int, cores: int = 1) -> int:
        return self._random_prime(self._lib.izp_ffi_random_prime_vx, bit_size, cores)

    def random_prime_vy(self, bit_size: int, cores: int = 1) -> int:
        return self._random_prime(self._lib.izp_ffi_random_prime_vy, bit_size, cores)

    def _random_prime(self, fn, bit_size: int, cores: int) -> int:
        out = ctypes.c_char_p()
        status = fn(int(bit_size), int(cores), ctypes.byref(out))
        self._raise_if_error(status)
        try:
            return int(out.value.decode("utf-8"))
        finally:
            self._lib.izp_ffi_free_string(ctypes.byref(out))

    def _raise_if_error(self, status: int) -> None:
        if status != Status.OK:
            msg = self.last_error() or self.status_message(status)
            raise IzprimeError(status=status, message=msg)
