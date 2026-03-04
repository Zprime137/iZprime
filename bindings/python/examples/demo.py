from izprime import Izprime, SieveKind


def main() -> None:
    izp = Izprime()
    print(f"iZprime version: {izp.version}")

    primes = izp.sieve(SieveKind.SIZM, 1000)
    print(f"SiZm primes <= 1000: count={len(primes)} last={primes[-1]}")

    count = izp.count_range("10^6", 1000, cores=1)
    print(f"count in [10^6, 10^6+999]: {count}")

    nxt = izp.next_prime("10^20 + 12345", forward=True)
    print(f"next prime: {nxt}")


if __name__ == "__main__":
    main()
