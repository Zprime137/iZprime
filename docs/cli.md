# izprime CLI

The CLI binary is `izprime` and exposes task-oriented subcommands over the library API.

## Build

```bash
make cli
./build/bin/izprime help
```

## Numeric input syntax

Commands that accept numbers support:

- decimal integers: `1000000`
- grouped decimals: `1,000,000`
- arithmetic expressions with `+ - * /`
- powers: `10^6`
- scientific shorthand: `1e6`, `10e100`
- parenthesized expressions: `(10^6 + 5) * 7`

## Commands

## `stream_primes`

Streams primes in an inclusive range using `SiZ_stream`.

```bash
izprime stream_primes --range "[LOWER, UPPER]" [--print | --stream-to FILE] [--print-gaps] [--mr-rounds N]
```

Examples:

```bash
izprime stream_primes --range "[0, 10^5]" --print
izprime stream_primes --range "[0, 10^5]" --print-gaps
izprime stream_primes --range "[1,000,000, 1,001,000]" --stream-to output/range.txt
```

Alias: `sieve`.

## `count_primes`

Counts primes in an inclusive range using `SiZ_count`.

```bash
izprime count_primes --range "[LOWER, UPPER]" [--cores N|max] [--mr-rounds N]
```

Examples:

```bash
izprime count_primes --range "[0, 10^9]" --cores 8
izprime count_primes --range "10^100 + 10^9, 10^100 + 10^9 + 10^9" --cores max
```

Alias: `count`.

Notes:

- `--cores` accepts an integer value (`>= 1`) or the literal `max`.
- `--cores-number` is still accepted as a backward-compatible alias.
- On platforms without `fork`, multi-process mode is unavailable and execution falls back to single-process mode.

## `next_prime`

Finds the next prime after `n` using `iZ_next_prime`.

```bash
izprime next_prime --n VALUE
```

Example:

```bash
izprime next_prime --n "10^12 + 39"
```

## `prev_prime`

Finds the previous prime before `n` using `iZ_next_prime` in reverse mode.

```bash
izprime prev_prime --n VALUE
```

Example:

```bash
izprime prev_prime --n "10^12 + 39"
```

Alias: `prev`.

## `is_prime`

Checks primality of `n` using `test_primality`.

```bash
izprime is_prime --n VALUE [--rounds N]
```

Example:

```bash
izprime is_prime --n "10^61 + 1" --rounds 40
```

## Existing maintenance commands

```bash
izprime test [--limit N]
izprime benchmark [--limit N] [--repeat N] [--algo NAME|all] [--save-results FILE]
izprime doctor
```

Use `izprime help <command>` for command-specific help.
