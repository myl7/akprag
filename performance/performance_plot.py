import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import os

# Set the style for a scientific paper
sns.set_theme(style="whitegrid", context="paper")

# Configure figure size for a single column in a two-column paper.
plt.rcParams.update(
    {
        "figure.figsize": (3.4, 1.7),
        "font.family": "serif",
        "font.serif": ["Times New Roman"],
        "font.size": 8,
        "axes.labelsize": 8,
        "legend.fontsize": 8,
        "xtick.labelsize": 8,
        "ytick.labelsize": 8,
    }
)


def main():
    # Determine the directory of this script
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Define file paths
    akprag_path = os.path.join(script_dir, "akprag.csv")
    prag_path = os.path.join(script_dir, "prag.csv")

    # Load data
    try:
        df_ak = pd.read_csv(akprag_path)
        df_prag = pd.read_csv(prag_path)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return

    # Process AKPRAG Data
    # First 49 rows (7 variations of N * 7 repeats)
    df_ak_1 = df_ak.iloc[:49].copy()
    df_ak_1["Method"] = "$p^2$RAG"

    # Remaining rows (vary k)
    df_ak_2 = df_ak.iloc[49:].copy()
    df_ak_2["Method"] = "$p^2$RAG"

    # Process PRAG Data
    # Convert time: from s (measured over 96 runs? or batch 96?) to per-query ms.
    # Instruction: "divide it by 96"
    df_prag["time_ms"] = (df_prag["time_s_m96"] / 96.0) * 1000.0

    # Split PRAG Data
    # Part 1: First 7 rows (vary N)
    df_prag_1 = df_prag.iloc[:7].copy()
    df_prag_1["Method"] = "PRAG"

    # Part 2: Remaining rows (vary k)
    df_prag_2 = df_prag.iloc[7 + 4 :].copy()
    df_prag_2["Method"] = "PRAG"

    # Compute actual X values for valid log plotting
    df_ak_1["N"] = 2 ** df_ak_1["n_pow2"]
    df_prag_1["N"] = 2 ** df_prag_1["n_pow2"]

    df_ak_2["k'"] = 2 ** df_ak_2["k_pow2"]
    df_prag_2["k'"] = 2 ** df_prag_2["k_pow2"]

    # Combine datasets
    data_1 = pd.concat([df_ak_1, df_prag_1], ignore_index=True)
    data_2 = pd.concat([df_ak_2, df_prag_2], ignore_index=True)

    # Create figure with two subplots
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(3.4, 2))

    # Plot 1: Vary N
    sns.lineplot(
        data=df_ak_1,
        x="N",
        y="time_ms",
        label="$p^2$RAG",
        ax=ax1,
        marker="+",
        color="tab:blue",
        markeredgecolor="black",
        markerfacecolor="black",
        # errorbar="sd",
        linewidth=1,
    )
    sns.lineplot(
        data=df_prag_1,
        x="N",
        y="time_ms",
        label="PRAG",
        ax=ax1,
        marker="x",
        markersize=4,
        color="tab:orange",
        markeredgecolor="black",
        markerfacecolor="black",
        # errorbar="sd",
        linewidth=1,
    )

    ax1.set_xscale("log", base=2)
    ax1.set_yscale("log")
    ax1.set_xlabel("$N$")
    ax1.set_ylabel("Server Time (ms)")

    # Clean up legend in subplot
    if ax1.get_legend():
        ax1.get_legend().remove()

    # Plot 2: Vary k'
    sns.lineplot(
        data=df_ak_2,
        x="k'",
        y="time_ms",
        label="$p^2$RAG",
        ax=ax2,
        marker="+",
        color="tab:blue",
        markeredgecolor="black",
        markerfacecolor="black",
        # errorbar="sd",
        linewidth=1,
    )
    sns.lineplot(
        data=df_prag_2,
        x="k'",
        y="time_ms",
        label="PRAG",
        ax=ax2,
        marker="x",
        markersize=4,
        color="tab:orange",
        markeredgecolor="black",
        markerfacecolor="black",
        # errorbar="sd",
        linewidth=1,
    )

    ax2.set_xscale("log", base=2)
    ax2.set_yscale("log")
    ax2.set_xlabel("$k'$")
    ax2.set_ylabel("")

    if ax2.get_legend():
        ax2.get_legend().remove()

    # Global Legend
    handles, labels = ax1.get_legend_handles_labels()

    # Place legend at the bottom
    fig.legend(
        handles, labels, loc="lower center", bbox_to_anchor=(0.5, 0.0), ncol=2, frameon=False
    )

    # Adjust layout to make room for legend at bottom
    plt.tight_layout(pad=0, rect=[0, 0.1, 1, 1])

    # Save the figure
    output_filename = os.path.join(script_dir, "performance_figure.pdf")
    plt.savefig(output_filename, dpi=300)
    print(f"Plot saved to: {output_filename}")

    # Also save png
    plt.savefig(os.path.join(script_dir, "performance_figure.png"), dpi=300)


if __name__ == "__main__":
    main()
