#!/usr/bin/env python3
"""Schematic diagrams of the energy-deposition processes for the 59.5 keV
gamma in 1 bar Ar and 1 bar Xe (for the calibration-feasibility discussion).

Drawn with matplotlib from the simulation geometry + measured fractions.
Run: analyze later -- python3 draw_process_schematics.py --out DIR
"""

import argparse
import pathlib

import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Rectangle, FancyArrowPatch, Polygon
from matplotlib import font_manager

# CJK-capable font on this host (otherwise Chinese labels render as boxes)
font_manager.fontManager.addfont("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc")

plt.rcParams.update({
    "font.family": "Noto Sans CJK SC",
    "axes.unicode_minus": False,
    "font.size": 13,
    "axes.titlesize": 15,
    "axes.labelsize": 13,
    "xtick.labelsize": 12,
    "ytick.labelsize": 12,
    "legend.fontsize": 12,
})

CAGE = dict(x0=0, x1=140, z0=0, z1=60)   # field cage (13 cm)
OUTER_TOP = 85                            # outer gas above the cathode
SRC = (70, 62)                            # source 1 mm above the cathode


def draw_cage(ax, title):
    ax.add_patch(Rectangle((0, 0), 140, 60, fill=False, lw=1.5, ec="0.3"))
    ax.plot([0, 140], [60, 60], color="0.3", lw=1.5)
    ax.plot(SRC[0], SRC[1], marker="*", ms=20, color="crimson", zorder=5)
    ax.text(SRC[0], SRC[1] + 4, "Am-241 59.5 keV", color="crimson",
            ha="center", fontsize=11)
    ax.text(70, -7, "读出芯片平面", ha="center", fontsize=11, color="0.25")
    for x in range(10, 140, 20):
        ax.add_patch(Polygon([[x - 7, -3], [x + 7, -3], [x + 5, 1], [x - 5, 1]],
                             closed=True, fc="#7fb3e0", ec="k", lw=0.5))
    ax.text(0, 63, "场笼外气体", fontsize=10, color="0.4", va="bottom")
    ax.text(0, 30, "13 cm 场笼", fontsize=11, color="0.4", va="center",
            rotation=90)
    ax.set_xlim(-10, 150)
    ax.set_ylim(-15, OUTER_TOP)
    ax.set_aspect("equal")
    ax.axis("off")
    ax.set_title(title, fontsize=13, pad=8)


def arrow(ax, p0, p1, kind="electron", color="tab:blue", lw=2.0, rad=0.0):
    style = dict(arrowstyle="-|>", mutation_scale=16, lw=lw, color=color,
                 connectionstyle=f"arc3,rad={rad}")
    if kind == "photon":
        style.update(linestyle=(0, (4, 2)))
    elif kind == "fluor":
        style.update(linestyle=(0, (1, 1.5)))
    ax.add_patch(FancyArrowPatch(p0, p1, **style))


def photon_path(ax, p0, p1, color="purple", lw=1.8, rad=0.0):
    arrow(ax, p0, p1, kind="photon", color=color, lw=lw, rad=rad)


def label(ax, x, y, text, size=11, color="0.1", weight="normal", ha="left"):
    ax.text(x, y, text, fontsize=size, color=color, ha=ha, weight=weight)


# ------------------------------------------------------------------- Ar ----
def ar_panels():
    fig, axes = plt.subplots(1, 3, figsize=(17, 6.6))

    # 1) PE, electron spans several chips
    ax = axes[0]
    draw_cage(ax, "① 光电吸收（69%）：光电子跨芯片分摊能量")
    photon_path(ax, SRC, (65, 35))
    arrow(ax, (65, 35), (20, 10), kind="electron", color="tab:blue", lw=2.2)
    arrow(ax, (20, 10), (5, 4), kind="electron", color="tab:blue", lw=2.2)
    label(ax, 66, 42, "光电子 ~56 keV\n射程 ~几厘米", size=11, color="tab:blue")
    label(ax, 8, 18, "能量被多片芯片分摊\n单芯片全能计数 ≈ 0", size=11, weight="bold")
    ax.text(70, -13, "→ 不能做逐通道全能峰标定", ha="center", fontsize=12,
            color="crimson", weight="bold")

    # 2) PE, electron escapes the cage
    ax = axes[1]
    draw_cage(ax, "② 光电子飞出场笼：场笼内能量损失")
    photon_path(ax, SRC, (60, 40))
    arrow(ax, (60, 40), (45, 78), kind="electron", color="tab:blue", lw=2.2)
    label(ax, 63, 50, "光电子向上飞出场笼", size=11, color="tab:blue")
    label(ax, 5, 12, "能量沉积在场笼外气体\n芯片收不到", size=11, weight="bold")
    ax.text(70, -13, "→ 场笼内全能事件只有 ~45%", ha="center", fontsize=12,
            color="crimson", weight="bold")

    # 3) Compton, scattered photon escapes
    ax = axes[2]
    draw_cage(ax, "③ 康普顿散射（31%）：部分沉积")
    photon_path(ax, SRC, (55, 38))
    arrow(ax, (55, 38), (35, 25), kind="electron", color="tab:blue", lw=2.2)
    photon_path(ax, (55, 38), (120, 75), color="purple", lw=1.8)
    label(ax, 35, 45, "散射光子逃逸", size=11, color="purple")
    label(ax, 5, 12, "电子只带走部分能量\n形成低能连续谱", size=11, weight="bold")
    ax.text(70, -13, "→ 本底，不是主因", ha="center", fontsize=12, color="0.4")

    fig.suptitle("59.5 keV γ 在 1 bar Ar 中的能量沉积过程", fontsize=16, y=0.99)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    return fig


# ------------------------------------------------------------------- Xe ----
def xe_panels():
    fig, axes = plt.subplots(1, 3, figsize=(17, 6.6))

    # 1) PE + K-alpha reabsorbed in the same channel
    ax = axes[0]
    draw_cage(ax, "① 光电吸收（98.6%）+ Kα 同通道再吸收")
    photon_path(ax, SRC, (62, 30))
    arrow(ax, (62, 30), (58, 18), kind="electron", color="tab:blue", lw=2.2)
    photon_path(ax, (62, 30), (66, 18), color="green", lw=1.8)
    ax.add_patch(Rectangle((53, -3), 14, 4, fill=False, ec="crimson", lw=1.6))
    label(ax, 68, 36, "Kα 荧光 ~30 keV\n在同一芯片内再吸收", size=11, color="green")
    label(ax, 5, 8, "光电子 + Kα + L 壳产物\n全部留在同一芯片", size=11, weight="bold")
    ax.text(70, -13, "→ 单芯片 59.5 keV 全能峰（标定锚点）", ha="center",
            fontsize=12, color="green", weight="bold")

    # 2) K-alpha escapes the cage
    ax = axes[1]
    draw_cage(ax, "② Kα 荧光飞出场笼：逃逸峰 ~30 keV")
    photon_path(ax, SRC, (58, 32))
    arrow(ax, (58, 32), (54, 20), kind="electron", color="tab:blue", lw=2.2)
    photon_path(ax, (58, 32), (75, 80), color="green", lw=1.8)
    label(ax, 62, 46, "Kα 在 K 边（34.6 keV）以下\n转换长度 > 13 cm 场笼", size=11, color="green")
    label(ax, 5, 10, "实测只剩 ~30 keV\n（59.5 − 29.7）", size=11, weight="bold")
    ax.text(70, -13, "→ 场笼内全能事件仅 ~7.6%", ha="center", fontsize=12,
            color="crimson", weight="bold")

    # 3) K-alpha lands in another channel
    ax = axes[2]
    draw_cage(ax, "③ Kα 落到其他芯片：能量在芯片间分配")
    photon_path(ax, SRC, (58, 32))
    arrow(ax, (58, 32), (50, 20), kind="electron", color="tab:blue", lw=2.2)
    photon_path(ax, (58, 32), (95, 15), color="green", lw=1.8)
    ax.add_patch(Rectangle((88, -3), 14, 4, fill=False, ec="crimson", lw=1.6))
    label(ax, 66, 42, "Kα 荧光 ~30 keV\n在另一芯片内转化", size=11, color="green")
    label(ax, 5, 8, "电子与荧光各归各芯\n没有单芯片全能事件", size=11, weight="bold")
    ax.text(70, -13, "→ 逐通道谱出现逃逸结构，需建模", ha="center", fontsize=12,
            color="crimson", weight="bold")

    fig.suptitle("59.5 keV γ 在 1 bar Xe 中的能量沉积过程", fontsize=16, y=0.99)
    fig.tight_layout(rect=[0, 0, 1, 0.95])
    return fig


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", type=pathlib.Path, required=True)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    ar_panels().savefig(args.out / "process_Ar.png", dpi=160)
    xe_panels().savefig(args.out / "process_Xe.png", dpi=160)
    print("wrote", args.out / "process_Ar.png", "and process_Xe.png")


if __name__ == "__main__":
    main()
