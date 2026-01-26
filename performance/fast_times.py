import pandas as pd
import os


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

    # Process PRAG Data
    # Convert time: from s (measured over 96 runs) to per-query ms.
    df_prag["time_ms"] = (df_prag["time_s_m96"] / 96.0) * 1000.0

    # Aggregate AKPRAG data (mean over repeats)
    # Group by settings to get average time
    ak_agg = df_ak.groupby(["n_pow2", "k_pow2"], as_index=False)["time_ms"].mean()

    # Aggregate PRAG data (mean over repeats, though there seem to be few/none)
    prag_agg = df_prag.groupby(["n_pow2", "k_pow2"], as_index=False)["time_ms"].mean()

    # Merge the datasets on settings
    merged = pd.merge(ak_agg, prag_agg, on=["n_pow2", "k_pow2"], suffixes=("_ak", "_prag"))

    # Compute actual N and k values for display
    merged["N"] = 2 ** merged["n_pow2"]
    merged["k"] = 2 ** merged["k_pow2"]

    # Compute speedup
    # Speedup = Time_old / Time_new
    # "how many times do akprag fast than prag" -> Time_PRAG / Time_AKPRAG
    merged["speedup"] = merged["time_ms_prag"] / merged["time_ms_ak"]

    # Display results
    print(f"{'N':<10} {'k':<10} {'T_AK (ms)':<15} {'T_PRAG (ms)':<15} {'Speedup':<10}")
    print("-" * 65)

    for _, row in merged.iterrows():
        print(
            f"{int(row['N']):<10} {int(row['k']):<10} {row['time_ms_ak']:<15.2f} {row['time_ms_prag']:<15.2f} {row['speedup']:<10.2f}x"
        )

    print("-" * 65)

    # Overall statistics
    avg_speedup = merged["speedup"].mean()
    max_speedup = merged["speedup"].max()
    min_speedup = merged["speedup"].min()

    print(f"Average Speedup: {avg_speedup:.2f}x")
    print(f"Min Speedup:     {min_speedup:.2f}x")
    print(f"Max Speedup:     {max_speedup:.2f}x")


if __name__ == "__main__":
    main()
