package izprime

/*
#cgo CFLAGS: -I${SRCDIR}/../../include
#cgo LDFLAGS: -lizprime
#include <stdlib.h>
#include <izprime_ffi.h>
*/
import "C"

import (
	"fmt"
	"unsafe"
)

type SieveKind int32

const (
	SOE    SieveKind = 0
	SSOE   SieveKind = 1
	SOS    SieveKind = 2
	SSOS   SieveKind = 3
	SOEU   SieveKind = 4
	SOA    SieveKind = 5
	SIZ    SieveKind = 6
	SIZM   SieveKind = 7
	SIZMVY SieveKind = 8
)

type Error struct {
	Status  int
	Message string
}

func (e *Error) Error() string {
	return fmt.Sprintf("iZprime error [%d]: %s", e.Status, e.Message)
}

func Version() string {
	return C.GoString(C.izp_ffi_version())
}

func Sieve(kind SieveKind, limit uint64) ([]uint64, error) {
	var out C.IZP_U64_BUFFER
	status := int(C.izp_ffi_sieve_u64(C.IZP_SIEVE_KIND(kind), C.uint64_t(limit), &out))
	if status != 0 {
		return nil, statusError(status)
	}
	defer C.izp_ffi_free_u64_buffer(&out)

	if out.len == 0 || out.data == nil {
		return []uint64{}, nil
	}

	vals := unsafe.Slice((*uint64)(unsafe.Pointer(out.data)), int(out.len))
	result := make([]uint64, len(vals))
	copy(result, vals)
	return result, nil
}

func CountRange(start string, rangeSize uint64, rounds int, cores int) (uint64, error) {
	cStart := C.CString(start)
	defer C.free(unsafe.Pointer(cStart))

	var out C.uint64_t
	status := int(C.izp_ffi_count_range(cStart, C.uint64_t(rangeSize), C.int(rounds), C.int(cores), &out))
	if status != 0 {
		return 0, statusError(status)
	}
	return uint64(out), nil
}

func NextPrime(baseExpr string, forward bool) (string, error) {
	cBase := C.CString(baseExpr)
	defer C.free(unsafe.Pointer(cBase))

	var out *C.char
	dir := C.int(0)
	if forward {
		dir = 1
	}

	status := int(C.izp_ffi_next_prime(cBase, dir, &out))
	if status != 0 {
		return "", statusError(status)
	}
	defer C.izp_ffi_free_string(&out)

	return C.GoString(out), nil
}

func statusError(status int) error {
	msg := C.GoString(C.izp_ffi_last_error())
	if msg == "" {
		msg = C.GoString(C.izp_ffi_status_message(C.int(status)))
	}
	return &Error{Status: status, Message: msg}
}
