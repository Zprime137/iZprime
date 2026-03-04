use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int};

#[repr(i32)]
#[derive(Clone, Copy)]
pub enum SieveKind {
    Soe = 0,
    Ssoe = 1,
    Sos = 2,
    SsoS = 3,
    Soeu = 4,
    Soa = 5,
    Siz = 6,
    Sizm = 7,
    SizmVy = 8,
}

#[repr(C)]
struct U64Buffer {
    data: *mut u64,
    len: usize,
}

#[derive(Debug)]
pub struct IzprimeError {
    pub status: i32,
    pub message: String,
}

impl std::fmt::Display for IzprimeError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "iZprime error [{}]: {}", self.status, self.message)
    }
}

impl std::error::Error for IzprimeError {}

#[link(name = "izprime")]
extern "C" {
    fn izp_ffi_version() -> *const c_char;
    fn izp_ffi_status_message(code: c_int) -> *const c_char;
    fn izp_ffi_last_error() -> *const c_char;

    fn izp_ffi_sieve_u64(kind: c_int, limit: u64, out: *mut U64Buffer) -> c_int;
    fn izp_ffi_count_range(start: *const c_char, range: u64, mr_rounds: c_int, cores: c_int, out_count: *mut u64) -> c_int;
    fn izp_ffi_next_prime(base_expr: *const c_char, forward: c_int, out_prime_base10: *mut *mut c_char) -> c_int;

    fn izp_ffi_free_u64_buffer(out: *mut U64Buffer);
    fn izp_ffi_free_string(out: *mut *mut c_char);
}

pub fn version() -> String {
    unsafe { cstr_to_string(izp_ffi_version()) }
}

pub fn sieve(kind: SieveKind, limit: u64) -> Result<Vec<u64>, IzprimeError> {
    let mut out = U64Buffer {
        data: std::ptr::null_mut(),
        len: 0,
    };

    let status = unsafe { izp_ffi_sieve_u64(kind as c_int, limit, &mut out) };
    if status != 0 {
        return Err(last_error(status));
    }

    let result = unsafe {
        let vals = if out.data.is_null() || out.len == 0 {
            vec![]
        } else {
            std::slice::from_raw_parts(out.data, out.len).to_vec()
        };
        izp_ffi_free_u64_buffer(&mut out);
        vals
    };

    Ok(result)
}

pub fn count_range(start: &str, range: u64, mr_rounds: i32, cores: i32) -> Result<u64, IzprimeError> {
    let c_start = CString::new(start).map_err(|_| IzprimeError {
        status: 1,
        message: "start expression contains interior NUL".to_string(),
    })?;

    let mut out_count = 0_u64;
    let status = unsafe {
        izp_ffi_count_range(
            c_start.as_ptr(),
            range,
            mr_rounds as c_int,
            cores as c_int,
            &mut out_count,
        )
    };

    if status != 0 {
        return Err(last_error(status));
    }

    Ok(out_count)
}

pub fn next_prime(base_expr: &str, forward: bool) -> Result<String, IzprimeError> {
    let c_base = CString::new(base_expr).map_err(|_| IzprimeError {
        status: 1,
        message: "base expression contains interior NUL".to_string(),
    })?;

    let mut out: *mut c_char = std::ptr::null_mut();
    let direction = if forward { 1 } else { 0 };

    let status = unsafe { izp_ffi_next_prime(c_base.as_ptr(), direction, &mut out) };
    if status != 0 {
        return Err(last_error(status));
    }

    let s = unsafe {
        let value = cstr_to_string(out as *const c_char);
        izp_ffi_free_string(&mut out);
        value
    };

    Ok(s)
}

fn last_error(status: i32) -> IzprimeError {
    unsafe {
        let raw = izp_ffi_last_error();
        let msg = if raw.is_null() || (*raw) == 0 {
            cstr_to_string(izp_ffi_status_message(status as c_int))
        } else {
            cstr_to_string(raw)
        };
        IzprimeError { status, message: msg }
    }
}

unsafe fn cstr_to_string(ptr: *const c_char) -> String {
    if ptr.is_null() {
        return String::new();
    }
    CStr::from_ptr(ptr).to_string_lossy().into_owned()
}
