from __future__ import annotations

import argparse
import re
from pathlib import Path

DEFAULT_OUTPUT_DIR = Path("./output")
PLOT_EXT = "svg"
ALGORITHM_PREFIX = "Algorithm:"
BIT_SIZE_PREFIX = "Bit Size:"
CORES_PREFIX = "Cores:"
CORES_NUMBER_PREFIX = "Cores Number:"
EXEC_TIMES_PREFIX = "Execution Times (s):"
LEGACY_EXEC_TIMES_PREFIX = "Time Results (seconds):"
AVERAGE_TIME_PREFIX = "Average Time:"


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
        if line.startswith(ALGORITHM_PREFIX) and current_section:
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


def parse_int_suffix(line: str, prefix: str) -> int | None:
    if not line.startswith(prefix):
        return None
    try:
        return int(line[len(prefix) :].strip())
    except ValueError:
        return None


def parse_times_suffix(line: str) -> list[float] | None:
    if not (line.startswith(EXEC_TIMES_PREFIX) or line.startswith(LEGACY_EXEC_TIMES_PREFIX)):
        return None

    match = re.search(r"\[(.*?)\]", line)
    if not match:
        return None
    return [float(token.strip()) for token in match.group(1).split(",") if token.strip()]


def parse_average_suffix(line: str) -> float | None:
    if not line.startswith(AVERAGE_TIME_PREFIX):
        return None
    match = re.search(r"([\d.]+)", line)
    if not match:
        return None
    return float(match.group(1))


def build_parsed_section(
    algorithm: str | None,
    cores: int | None,
    times: list[float],
    avg_time: float | None,
    bit_size: int | None,
):
    if algorithm and cores is not None and times and avg_time is not None:
        return {
            "algorithm": algorithm,
            "cores": cores,
            "times": times,
            "avg_time": avg_time,
            "bit_size": bit_size,
        }
    return None


def parse_section(lines: list[str]):
    algorithm = None
    cores = None
    times = []
    avg_time = None
    bit_size = None

    for line in lines:
        if line.startswith(ALGORITHM_PREFIX):
            algorithm = line[len(ALGORITHM_PREFIX) :].strip()
            continue

        bit_size_candidate = parse_int_suffix(line, BIT_SIZE_PREFIX)
        if bit_size_candidate is not None:
            bit_size = bit_size_candidate
            continue

        cores_candidate = parse_int_suffix(line, CORES_PREFIX)
        if cores_candidate is None:
            cores_candidate = parse_int_suffix(line, CORES_NUMBER_PREFIX)
        if cores_candidate is not None:
            cores = cores_candidate
            continue

        times_candidate = parse_times_suffix(line)
        if times_candidate is not None:
            times = times_candidate
            continue

        avg_candidate = parse_average_suffix(line)
        if avg_candidate is not None:
            avg_time = avg_candidate

    return build_parsed_section(algorithm, cores, times, avg_time, bit_size)


def build_plot_title(bit_size: int | None, plot_avg: bool) -> str:
    target_bits = bit_size if bit_size is not None else "Unknown"
    if plot_avg:
        return f"Average Time Analysis for Prime Generation Methods (Target Bit Size: {target_bits})"
    return f"Execution Time Analysis for Prime Generation Methods (Target Bit Size: {target_bits})"


def plot_series(ax, result: dict[str, object], idx: int, colors: list[str], markers: list[str], plot_avg: bool):
    algorithm = str(result["algorithm"])
    cores = int(result["cores"])
    times = list(result["times"])
    avg_time = float(result["avg_time"])
    x_values = list(range(1, len(times) + 1))
    color = colors[idx % len(colors)]
    marker = markers[idx % len(markers)]
    label = f"{algorithm} (cores: {cores})" if cores > 1 else algorithm

    y_values = [avg_time] * len(x_values) if plot_avg else times
    linestyle = "dashed" if plot_avg else "solid"
    ax.plot(x_values, y_values, marker=marker, linestyle=linestyle, color=color, label=label)


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
    ax.set_title(build_plot_title(bit_size, plot_avg))

    ax.set_xlabel("Test Round")
    ax.set_ylabel("Time (seconds)")
    ax.grid(True)
    ax.xaxis.set_major_locator(MaxNLocator(integer=True))

    markers = ["o", "s", "d", "^", "v", "x"]
    colors = plt.rcParams["axes.prop_cycle"].by_key()["color"]

    for idx, result in enumerate(results):
        plot_series(ax, result, idx, colors, markers, plot_avg)

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
