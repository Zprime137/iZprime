#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path


def prompt_results_path() -> str:
    prompt = "Enter benchmark results filepath or stem (e.g. output/psieve_09190513.txt or psieve_09190513): "
    value = input(prompt).strip()
    if not value:
        raise ValueError("No input provided for results filepath.")
    return value


def resolve_results_path(results_input: str | Path) -> Path:
    input_path = Path(results_input)
    candidates = []

    if input_path.exists():
        candidates.append(input_path)

    if input_path.suffix == ".txt":
        candidates.append(Path("output") / input_path.name)
    else:
        candidates.append(input_path.with_suffix(".txt"))
        candidates.append(Path("output") / f"{input_path.name}.txt")

    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()

    raise FileNotFoundError(f"Could not find results file for '{results_input}'.")


def detect_results_type(results_path: Path) -> str:
    """
    Return one of: 'p_sieve', 'p_gen'.
    """
    with results_path.open("r", encoding="utf-8") as handle:
        probe = [line.strip() for _, line in zip(range(40), handle)]

    if any(line.startswith("Test Limits:") for line in probe):
        return "p_sieve"
    if any(line.startswith("Algorithm:") for line in probe):
        return "p_gen"

    filename = results_path.name.lower()
    if filename.startswith("psieve_"):
        return "p_sieve"
    if filename.startswith("p_gen_"):
        return "p_gen"

    raise ValueError(
        f"Unable to detect results type for '{results_path}'. "
        "Expected sieve results with 'Test Limits:' or prime-gen results with 'Algorithm:'."
    )


def main():
    parser = argparse.ArgumentParser(
        description="Plot benchmark results by auto-detecting result type."
    )
    parser.add_argument("results", nargs="?", help="Results filepath or stem")
    parser.add_argument("--save", action="store_true", help="Save generated plot(s) as SVG")
    parser.add_argument("--avg", action="store_true", help="For p_gen only: plot averages")
    parser.add_argument("--show", dest="show", action="store_true", help="Display plot window")
    parser.add_argument("--no-show", dest="show", action="store_false", help="Do not display plot window")
    parser.set_defaults(show=True)
    args = parser.parse_args()

    try:
        results_input = args.results if args.results else prompt_results_path()
    except ValueError as exc:
        print(str(exc))
        raise SystemExit(1)

    results_path = resolve_results_path(results_input)
    results_type = detect_results_type(results_path)

    try:
        from plot_p_gen_results import plot_prime_gen_results
        from plot_p_sieve_results import plot_sieve_results
    except ModuleNotFoundError as exc:
        missing = exc.name if hasattr(exc, "name") else "dependency"
        print(
            f"Missing Python dependency: {missing}\n"
            "Install plotting requirements with:\n"
            "  python3 -m pip install -r py_tools/requirements.txt"
        )
        raise SystemExit(2)

    if results_type == "p_sieve":
        plot_sieve_results(results_path, save=args.save, show=args.show)
        return

    if results_type == "p_gen":
        plot_prime_gen_results(
            results_path, plot_avg=args.avg, save=args.save, show=args.show
        )
        return

    raise RuntimeError(f"Unsupported results type '{results_type}'.")


if __name__ == "__main__":
    main()
