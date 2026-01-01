import math
import os

from matplotlib import pyplot as plt

# defaults plot values
output_dir = './output/'
plot_ext = 'svg'


# Function to read results from a file
def read_results(filename: str):
    data = {}
    with open(f"./output/{filename}.txt", 'r') as file:
        lines = file.readlines()

        # Read the test range from the first line
        test_range_line = lines[0].strip().split()

        # Extract the range part (e.g., "10^3:10^9")
        range_part = test_range_line[2].split(':')

        # Parse the exponents from the format "10^3" and "10^9"
        min_exp = int(range_part[0].split('^')[1])
        max_exp = int(range_part[1].split('^')[1])

        # Read the results
        for line in lines[1:]:
            algorithm, times = line.strip().split(': ')
            times = list(map(int, times.strip('[]').split(', ')))
            data[algorithm] = times

    return data, 10, list(range(min_exp, max_exp + 1))


# Function to plot the time performance of sieve benchmarking results
def plot_sieve_results(filename: str, save: bool = False):
    data, base, exponents = read_results(filename)
    x_values = [base**exp for exp in exponents]

    # Normalize the data
    normalized_data = {}
    for algorithm, times in data.items():
        normalized_times = [(time * 1000) / n for time,
                            n in zip(times, x_values)]
        normalized_data[algorithm] = normalized_times

    # Plotting the normalized results
    fig, ax = plt.subplots(figsize=(8, 6))

    title = "Time-Performance Analysis:\nNormalized execution time over incrementing powers of 10 as limits"
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
    ax.set_xticklabels([f'$10^{{{exp}}}$' for exp in exponents])

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
    plot_sieve_results("sieve_results_20250907113347", save=True)
