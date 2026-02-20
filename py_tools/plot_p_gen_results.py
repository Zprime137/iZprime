from __future__ import annotations

import argparse
import re
from pathlib import Path

DEFAULT_OUTPUT_DIR = Path("./output")
PLOT_EXT = "svg"


def prompt_results_path() -> str:
    prompt = "Enter prime-generation results filepath or stem (e.g. output/p_gen_09190513.txt or p_gen_09190513): "
    value = input(prompt).strip()
    if not value:
        raise ValueError("No input provided for prime-generation results filepath.")
    return value


def resolve_results_path(results_input: str | Path) -> Path:
    """
    Resolve a prime-generation benchmark results file from either:
    - an explicit filepath, or
    - a stem such as 'p_gen_09095529'.
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
        f"Could not resolve prime-generation results file from input '{results_input}'."
    )


def read_results(results_input: str | Path):
    """
    Read prime-generation benchmark results.

    Supported section style:
      Algorithm: <name>
      Bit Size: <bits>
      Cores: <cores>
      ...
      Execution Times (s): [t1, t2, ...]
      Average Time: <value> seconds
    """
    results_path = resolve_results_path(results_input)
    with results_path.open("r", encoding="utf-8") as file:
        lines = [line.strip() for line in file.readlines()]

    sections = []
    current_section = []
    for line in lines:
        if line.startswith("Algorithm:") and current_section:
            sections.append(current_section)
            current_section = [line]
            continue

        if line:
            current_section.append(line)

    if current_section:
        sections.append(current_section)

    parsed = []
    bit_size_hint = None
    for section in sections:
        row = parse_section(section)
        if row is not None:
            parsed.append(row)
            if bit_size_hint is None and row["bit_size"] is not None:
                bit_size_hint = row["bit_size"]

    return results_path, bit_size_hint, parsed


def parse_section(lines: list[str]):
    algorithm = None
    cores = None
    times = []
    avg_time = None
    bit_size = None

    for line in lines:
        if line.startswith("Algorithm:"):
            algorithm = line[len("Algorithm:") :].strip()
        elif line.startswith("Bit Size:"):
            try:
                bit_size = int(line[len("Bit Size:") :].strip())
            except ValueError:
                bit_size = None
        elif line.startswith("Cores:"):
            try:
                cores = int(line[len("Cores:") :].strip())
            except ValueError:
                cores = None
        elif line.startswith("Cores Number:"):
            try:
                cores = int(line[len("Cores Number:") :].strip())
            except ValueError:
                cores = None
        elif line.startswith("Execution Times (s):") or line.startswith("Time Results (seconds):"):
            match = re.search(r"\[(.*?)\]", line)
            if match:
                times = [float(token.strip()) for token in match.group(1).split(",") if token.strip()]
        elif line.startswith("Average Time:"):
            match = re.search(r"([\d.]+)", line)
            if match:
                avg_time = float(match.group(1))

    if algorithm and cores is not None and times and avg_time is not None:
        return {
            "algorithm": algorithm,
            "cores": cores,
            "times": times,
            "avg_time": avg_time,
            "bit_size": bit_size,
        }
    return None


def plot_prime_gen_results(
    results_input: str | Path, plot_avg: bool = False, save: bool = False, show: bool = True
):
    try:
        from matplotlib import pyplot as plt
        from matplotlib.ticker import MaxNLocator
    except ModuleNotFoundError:
        print(
            "Missing Python dependency: matplotlib\n"
            "Install plotting requirements with:\n"
            "  python3 -m pip install -r py_tools/requirements.txt"
        )
        raise SystemExit(2)

    results_path, bit_size, results = read_results(results_input)
    if not results:
        raise ValueError(f"No prime-generation benchmark rows parsed from '{results_path}'.")

    fig, ax = plt.subplots(figsize=(10, 6))
    target_bits = bit_size if bit_size is not None else "Unknown"
    if plot_avg:
        ax.set_title(
            f"Average Time Analysis for Prime Generation Methods (Target Bit Size: {target_bits})"
        )
    else:
        ax.set_title(
            f"Execution Time Analysis for Prime Generation Methods (Target Bit Size: {target_bits})"
        )

    ax.set_xlabel("Test Round")
    ax.set_ylabel("Time (seconds)")
    ax.grid(True)
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))

    markers = ["o", "s", "d", "^", "v", "x"]
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]

    for idx, result in enumerate(results):
        algorithm = result["algorithm"]
        cores = result["cores"]
        times = result["times"]
        avg_time = result["avg_time"]
        x_values = list(range(1, len(times) + 1))
        color = colors[idx % len(colors)]
        marker = markers[idx % len(markers)]
        label = f"{algorithm} (cores: {cores})" if cores > 1 else algorithm

        if plot_avg:
            ax.plot(
                x_values,
                [avg_time] * len(x_values),
                marker=marker,
                linestyle="dashed",
                color=color,
                label=label,
            )
        else:
            ax.plot(
                x_values,
                times,
                marker=marker,
                linestyle="solid",
                color=color,
                label=label,
            )

    ax.legend()
    plt.tight_layout()

    if save:
        suffix = "_avg" if plot_avg else ""
        save_plot_figure(fig, results_path.parent, f"{results_path.stem}{suffix}")
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
    parser = argparse.ArgumentParser(description="Plot prime-generation benchmark results.")
    parser.add_argument("results", nargs="?", help="Results filepath or stem (e.g. p_gen_09095529)")
    parser.add_argument("--avg", action="store_true", help="Plot average time lines instead of per-run values")
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

    plot_prime_gen_results(results_input, plot_avg=args.avg, save=args.save, show=args.show)
