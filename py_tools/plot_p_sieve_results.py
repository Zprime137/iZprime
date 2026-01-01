import math
import os

from matplotlib import pyplot as plt

output_dir = './output/'  # default output directory
plot_ext = 'svg'  # default plot extension


def parse_results(filename: str):
    """
    parse results format:
    Test Limits: [base^exp, base^exp, ...]
    Test Results:
    Sieve Model1: [time1, time2, ...]
    Sieve Model2: [time1, time2, ...]

    Extract the test limits and the results for each sieve model,
    returning (limit_pairs, results), where:

    - limit_pairs: list[tuple[int, int]] of (base, exp)
    - results: dict[str, list[int]] mapping model name -> times
    """
    data = {}
    limit_pairs: list[tuple[int, int]] = []

    with open(f"./output/{filename}.txt", 'r') as file:
        lines = [ln.strip() for ln in file.readlines() if ln.strip()]

    if not lines:
        return limit_pairs, data

    # Headers can span multiple lines, and result lines are of the form:
    # "Algorithm: [t1, t2, ...]". We'll parse headers first.
    header_lines = []
    result_lines = []
    for ln in lines:
        # Always treat known metadata keys as headers, even though they can look
        # like "Key: [a, b, c]".
        if ln.startswith(('Test Limits:', 'Test Range:', 'Tests Count:', 'Test Results:')):
            header_lines.append(ln)
            continue

        if ': ' in ln:
            _, rhs = ln.split(': ', 1)
            if rhs.startswith('[') and rhs.endswith(']'):
                result_lines.append(ln)
                continue

        header_lines.append(ln)

    limits_line = next(
        (ln for ln in header_lines if ln.startswith('Test Limits:')), None)
    range_line = next(
        (ln for ln in header_lines if ln.startswith('Test Range:')), None)

    # Preferred format (supports mixed bases):
    # Test Limits: [10^9, 2^32, ...]
    if limits_line is not None:
        rhs = limits_line.split(':', 1)[1].strip()
        parts = [p.strip() for p in rhs.strip('[]').split(',') if p.strip()]
        for part in parts:
            if '^' not in part:
                continue
            b_str, e_str = [s.strip() for s in part.split('^', 1)]
            try:
                limit_pairs.append((int(b_str), int(e_str)))
            except ValueError:
                continue

    # Fallback format (single base, consecutive exponents):
    # Test Range: 10^4:10^9
    elif range_line is not None:
        range_part = range_line.split(':', 1)[1].strip()
        left, right = range_part.split(':', 1)
        b0, e0 = left.split('^', 1)
        b1, e1 = right.split('^', 1)

        base0 = int(b0)
        base1 = int(b1)
        min_exp = int(e0)
        max_exp = int(e1)

        # If bases mismatch, we can't infer intermediate mixed bases.
        # In that case, we only emit the endpoints.
        if base0 != base1:
            limit_pairs = [(base0, min_exp), (base1, max_exp)]
        else:
            limit_pairs = [(base0, exp) for exp in range(min_exp, max_exp + 1)]

    # Read the results lines: "Algorithm: [t1, t2, ...]"
    for line in result_lines:
        if ': ' not in line:
            continue
        algorithm, times = line.split(': ', 1)
        if not times.startswith('[') or not times.endswith(']'):
            continue
        times_list = times.strip('[]')
        if times_list == '':
            data[algorithm] = []
        else:
            tokens = [t.strip() for t in times_list.split(',') if t.strip()]
            try:
                data[algorithm] = [int(t) for t in tokens]
            except ValueError as e:
                raise ValueError(
                    f"Non-integer time value in line: '{line}'") from e

    return limit_pairs, data


# Function to plot the time performance of sieve benchmarking results
def plot_sieve_results(filename: str, save: bool = False):
    # use parsed results
    limit_pairs, data = parse_results(filename)
    if not limit_pairs or not data:
        raise ValueError(f"No results parsed for '{filename}'")

    x_values = [b ** e for (b, e) in limit_pairs]
    x_labels = [f'${b}^{{{e}}}$' for (b, e) in limit_pairs]

    # Keep normalization safe if any model has fewer points.
    min_len = min([len(times) for times in data.values()] + [len(x_values)])
    if min_len == 0:
        raise ValueError(f"No usable datapoints for '{filename}'")

    x_values = x_values[:min_len]
    x_labels = x_labels[:min_len]
    data = {k: v[:min_len] for k, v in data.items()}

    # Normalize the data
    normalized_data = {}
    for algorithm, times in data.items():
        normalized_times = [(time * 1000) / n for time,
                            n in zip(times, x_values)]
        normalized_data[algorithm] = normalized_times

    # Plotting the normalized results
    fig, ax = plt.subplots(figsize=(8, 6))

    title = "Time-per-input analysis over incrementing limits"
    ax.set_title(title)
    ax.set_xlabel(f'N')
    ax.set_ylabel('μs/n * 1000')

    x_ticks = [math.log10(v) for v in x_values]
    all_y = []

    for _, (algorithm, times) in enumerate(normalized_data.items()):
        label = algorithm
        y = times
        all_y.append(y)
        ax.plot(x_ticks, y, label=label, marker='o')

    # Set y ticks
    max_y = max([max(y) for y in all_y])
    ax.set_yticks(list(range(0, int(math.ceil(max_y)) + 1, 1)))

    # Set x ticks
    ax.set_xticks(x_ticks)
    ax.set_xticklabels(x_labels)

    ax.grid(True)
    plt.legend()
    plt.show()

    if save:
        save_plot_figure(fig, dir=output_dir, filename=filename)


# Function to save the plot figure
def save_plot_figure(fig, dir, filename):
    """save plot in svg format in the default output_dir"""
    filepath = os.path.join(dir, f"{filename}.{plot_ext}")
    fig.savefig(filepath, format=plot_ext)
    print(f"Figure saved as {filepath}")


if __name__ == '__main__':
    plot_sieve_results("psieve_01203349", save=True)
