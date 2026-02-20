# Examples

This folder contains small standalone programs showing how to use the public API in `include/iZ_api.h`.

## Build

From the repo root:

```bash
make examples
```

## Run

```bash
./build/examples/sieve_primes SiZm 10000000 10
./build/examples/SiZ_range 0 1000000
./build/examples/SiZ_range 1000000000000 1000000 output/iZ_stream.txt
./build/examples/p_genrators 1024 vx
```

Tip: you can also use the Makefile helper:

```bash
make run-example EX=sieve_primes ARGS="SiZm 10000000 10"
```
