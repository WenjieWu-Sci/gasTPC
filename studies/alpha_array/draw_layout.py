#!/usr/bin/env python3
"""Draw the real 98-chip layout with large text and both source markers.

Usage: draw_layout.py [--layout config/array_98ch.csv] [--out array_98ch_labeled.png]
Reads the corrected layout CSV (row 1 at top), marks the edge source
(midpoint of ADC5-IN4 / ADC5-IN3) and the central source (sensitive centroid).
"""

import argparse
import pathlib

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import RegularPolygon
from matplotlib import font_manager

font_manager.fontManager.addfont("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")

plt.rcParams.update({
    "font.family": "Noto Sans CJK SC",
    "axes.unicode_minus": False,
    "font.size": 13,
    "axes.titlesize": 16,
    "axes.labelsize": 14,
    "xtick.labelsize": 13,
    "ytick.labelsize": 13,
    "legend.fontsize": 12,
})


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--layout", type=pathlib.Path,
                    default=pathlib.Path(__file__).resolve().parent / "config" / "array_98ch.csv")
    ap.add_argument("--out", type=pathlib.Path,
                    default=pathlib.Path(__file__).resolve().parent / "config" / "array_98ch_labeled.png")
    args = ap.parse_args()

    import pandas as pd
    df = pd.read_csv(args.layout, comment="#")
    df = df.rename(columns={"channel": "chip", "x_mm": "x", "y_mm": "y"})

    pos = {r.chip: (r.x, r.y) for r in df.itertuples()}
    edge = tuple((np.array(pos["ADC5-IN4"]) + np.array(pos["ADC5-IN3"])) / 2.0)
    central = (0.0, 0.0)  # sensitive centroid (layout is centred on it)

    fig, ax = plt.subplots(figsize=(12, 9))
    for r in df.itertuples():
        color = "#7fb3e0" if r.role == "sensitive" else "#e08888"
        ax.add_patch(RegularPolygon((r.x, r.y), 6, radius=6.0,
                                    facecolor=color, edgecolor="k", lw=0.5))
        ax.text(r.x, r.y, r.chip.replace("ADC", "").replace("-IN", "/"),
                ha="center", va="center", fontsize=8)

    # source markers with callouts placed outside the array
    ax.plot(*edge, marker="*", color="green", markersize=24, zorder=5)
    ax.annotate("偏心源：ADC5-IN4 / ADC5-IN3 中点",
                xy=edge, xytext=(-72, 66), fontsize=13, color="green",
                arrowprops=dict(arrowstyle="->", color="green", lw=1.5))
    ax.plot(*central, marker="*", color="crimson", markersize=24, zorder=5)
    ax.annotate("中心源：灵敏区质心",
                xy=central, xytext=(18, -68), fontsize=13, color="crimson",
                arrowprops=dict(arrowstyle="->", color="crimson", lw=1.5))

    ax.set_aspect("equal"); ax.autoscale_view()
    ax.set_xlabel("x (mm)"); ax.set_ylabel("y (mm)")
    ax.set_title("98-chip layout: 49 sensitive (blue) + 49 veto (red), 12 mm pitch")
    fig.tight_layout()
    fig.savefig(args.out, dpi=150)
    print("wrote", args.out)


if __name__ == "__main__":
    main()
