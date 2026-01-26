import matplotlib.pyplot as plt
import seaborn as sns
import pandas as pd
import os

# Set the style for a scientific paper
sns.set_theme(style="whitegrid", context="paper")

# Configure figure size for a single column in a two-column paper.
# Standard column width is roughly 3.3 to 3.5 inches.
# Height is set to half of the width as requested.
plt.rcParams.update({
    "figure.figsize": (3.4, 1.7),
    "font.family": "serif",
    "font.serif": ["Times New Roman"],
    "font.size": 8,
    "axes.labelsize": 8,
    "legend.fontsize": 8,
    "xtick.labelsize": 8,
    "ytick.labelsize": 8,
})

def main():
    # Determine the directory of this script
    script_dir = os.path.dirname(os.path.abspath(__file__))

    # Define file paths
    recall_path = os.path.join(script_dir, "recall.csv")
    rel_scores_path = os.path.join(script_dir, "rel_scores.csv")

    # Load data
    try:
        df_recall = pd.read_csv(recall_path)
        df_rel = pd.read_csv(rel_scores_path)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return

    # Create a figure with two subplots (horizontal), right one is wider (1:2 ratio)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(3.4, 1.7), gridspec_kw={'width_ratios': [1, 2]})

    # Plot 1: Recall
    # sns.lineplot automatically aggregates over the different 'query_id's
    # and plots the mean (solid line) and a confidence interval (shaded area).
    sns.lineplot(
        data=df_recall,
        x="k",
        y="recall",
        ax=ax1,
        marker="+",  # Mark all points
        errorbar="sd", # Show Standard Deviation as the shaded area
        color="#1f77b4",
        linewidth=1,
        markeredgecolor="black",
        markerfacecolor="black",
    )
    ax1.set_ylabel("Recall")
    ax1.set_ylim(top=1.01) # Max recall is 1.0
    ax1.set_xlabel("$k'$")

    # Plot 2: Relevance Scores
    sns.lineplot(
        data=df_rel,
        x="k",
        y="rel_score",
        ax=ax2,
        marker="+",  # Mark all points
        errorbar="sd", # Show Standard Deviation as the shaded area
        color="#ff7f0e",
        linewidth=1,
        markeredgecolor="black",
        markerfacecolor="black",
    )
    ax2.set_ylabel("Relevance Score")
    ax2.set_xlabel("$k'$")

    # Adjust layout to remove wasted space
    plt.tight_layout(pad=0)

    # Save the figure
    output_filename = os.path.join(script_dir, "accuracy_figure.pdf")
    plt.savefig(output_filename, dpi=300)
    print(f"Plot saved to: {output_filename}")

    # Also save a png for quick preview if needed
    plt.savefig(os.path.join(script_dir, "accuracy_figure.png"), dpi=300)

if __name__ == "__main__":
    main()
