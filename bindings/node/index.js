const path = require("node:path");
const ffi = require("ffi-napi");
const ref = require("ref-napi");
const Struct = require("ref-struct-di")(ref);

const U64Buffer = Struct({
  data: ref.refType(ref.types.uint64),
  len: ref.types.size_t,
});

const candidates = [];
if (process.env.IZPRIME_LIB) {
  candidates.push(process.env.IZPRIME_LIB);
}

const root = path.resolve(__dirname, "..", "..");
if (process.platform === "darwin") {
  candidates.push(path.join(root, "build", "lib", "libizprime.dylib"));
} else if (process.platform === "win32") {
  candidates.push(path.join(root, "build", "lib", "libizprime.dll"));
} else {
  candidates.push(path.join(root, "build", "lib", "libizprime.so"));
}

let lib = null;
let loadErr = null;
for (const cand of candidates) {
  try {
    lib = ffi.Library(cand, {
      izp_ffi_version: ["string", []],
      izp_ffi_last_error: ["string", []],
      izp_ffi_sieve_u64: ["int", ["int", "uint64", ref.refType(U64Buffer)]],
      izp_ffi_free_u64_buffer: ["void", [ref.refType(U64Buffer)]],
      izp_ffi_count_range: [
        "int",
        ["string", "uint64", "int", "int", ref.refType("uint64")],
      ],
    });
    break;
  } catch (err) {
    loadErr = err;
  }
}

if (!lib) {
  throw new Error(`Failed to load libizprime (${loadErr})`);
}

function version() {
  return lib.izp_ffi_version();
}

function countRange(start, rangeSize, rounds = 30, cores = 1) {
  const out = ref.alloc("uint64");
  const status = lib.izp_ffi_count_range(start, rangeSize, rounds, cores, out);
  if (status !== 0) {
    throw new Error(lib.izp_ffi_last_error());
  }
  return Number(out.deref());
}

module.exports = {
  version,
  countRange,
};
