from __future__ import annotations

import argparse
import math
from pathlib import Path

DEFAULT_OUTPUT_DIR = Path("./output")
PLOT_EXT = "svg"


def prompt_results_path() -> str:
    prompt = "Enter sieve results filepath or stem (e.g. output/psieve_09190513.txt or psieve_09190513): "
    value = input(prompt).strip()
    if not value:
        raise ValueError("No input provided for sieve results filepath.")
    return value


def resolve_results_path(results_input: str | Path) -> Path:
    """
    Resolve a sieve benchmark results file from either:
    - an explicit filepath, or
    - a stem such as 'psieve_09095529'.
    """
    input_path = Path(results_input)

    candidates = []
    if input_path.exists():
        candidates.append(input_path)

    if input_path.suffix == ".txt":
        candidates.append(DEFAULT_OUTPUT_DIR / input_path.name)
    else:
        candidates.append(input_path.with_suffix(".txt"))
        candidates.append(DEFAULT_OUTPUT_DIR / f"{input_path.name}.txt")

    for candidate in candidates:
        if candidate.exists():
            return candidate.resolve()

    raise FileNotFoundError(
        f"Could not resolve sieve results file from input '{results_input}'."
    )


def parse_results(results_input: str | Path):
    """
    Parse sieve benchmark output in this format:
      Test Limits: [base^exp, base^exp, ...]
      Test Results:
      Algorithm: [time1, time2, ...]
    """
    results_path = resolve_results_path(results_input)
    data: dict[str, list[int]] = {}
    limit_pairs: list[tuple[int, int]] = []

    with results_path.open("r", encoding="utf-8") as file:
        lines = [ln.strip() for ln in file.readlines() if ln.strip()]

    if not lines:
        return results_path, limit_pairs, data

    limits_line = next((ln for ln in lines if ln.startswith("Test Limits:")), None)
    if limits_line:
        rhs = limits_line.split(":", 1)[1].strip()
        parts = [p.strip() for p in rhs.strip("[]").split(",") if p.strip()]
        for part in parts:
            if "^" not in part:
                continue
            b_str, e_str = [s.strip() for s in part.split("^", 1)]
            try:
                limit_pairs.append((int(b_str), int(e_str)))
            except ValueError:
                continue

    for line in lines:
        if line.startswith(("Test Limits:", "Test Results:")):
            continue
        if ": " not in line:
            continue
        algorithm, times = line.split(": ", 1)
        if not (times.startswith("[") and times.endswith("]")):
            continue
        tokens = [token.strip() for token in times.strip("[]").split(",") if token.strip()]
        try:
            data[algorithm] = [int(token) for token in tokens]
        except ValueError:
            continue

    return results_path, limit_pairs, data


def plot_sieve_results(results_input: str | Path, save: bool = False, show: bool = True):
    try:
        from matplotlib import pyplot as plt
    except ModuleNotFoundError:
        print(
            "Missing Python dependency: matplotlib\n"
            "Install plotting requirements with:\n"
            "  python3 -m pip install -r py_tools/requirements.txt"
        )
        raise SystemExit(2)

    results_path, limit_pairs, data = parse_results(results_input)
    if not limit_pairs or not data:
        raise ValueError(f"No sieve benchmark rows parsed from '{results_path}'.")

    x_values = [base**exp for (base, exp) in limit_pairs]
    x_labels = [f"${base}^{{{exp}}}$" for (base, exp) in limit_pairs]

    min_len = min([len(times) for times in data.values()] + [len(x_values)])
    if min_len == 0:
        raise ValueError(f"No usable datapoints in '{results_path}'.")

    x_values = x_values[:min_len]
    x_labels = x_labels[:min_len]
    data = {name: values[:min_len] for name, values in data.items()}

    normalized_data: dict[str, list[float]] = {}
    for algorithm, times in data.items():
        normalized_data[algorithm] = [(time * 1000.0) / n for time, n in zip(times, x_values)]

    fig, ax = plt.subplots(figsize=(8, 6))
    ax.set_title("Time-per-input analysis over incrementing limits")
    ax.set_xlabel("N")
    ax.set_ylabel("us/n * 1000")

    x_ticks = [math.log10(v) for v in x_values]
    all_y = []

    for algorithm, times in normalized_data.items():
        all_y.append(times)
        ax.plot(x_ticks, times, label=algorithm, marker="o")

    max_y = max(max(y_values) for y_values in all_y)
    ax.set_yticks(list(range(0, int(math.ceil(max_y)) + 1, 1)))
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_labels)
    ax.grid(True)
    ax.legend()
    plt.tight_layout()

    if save:
        save_plot_figure(fig, results_path.parent, results_path.stem)
    if show:
        plt.show()
    else:
        plt.close(fig)


def save_plot_figure(fig, output_dir: Path, filename_stem: str):
    output_dir.mkdir(parents=True, exist_ok=True)
    filepath = output_dir / f"{filename_stem}.{PLOT_EXT}"
    fig.savefig(filepath, format=PLOT_EXT)
    print(f"Figure saved as {filepath}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Plot prime sieve benchmark results.")
    parser.add_argument("results", nargs="?", help="Results filepath or stem (e.g. psieve_09095529)")
    parser.add_argument("--save", action="store_true", help="Save the plot as SVG")
    parser.add_argument("--show", dest="show", action="store_true", help="Display the plot window")
    parser.add_argument("--no-show", dest="show", action="store_false", help="Do not display the plot window")
    parser.set_defaults(show=True)
    args = parser.parse_args()

    try:
        results_input = args.results if args.results else prompt_results_path()
    except ValueError as exc:
        print(str(exc))
        raise SystemExit(1)

    plot_sieve_results(results_input, save=args.save, show=args.show)
