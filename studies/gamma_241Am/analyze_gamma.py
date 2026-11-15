#!/usr/bin/env python3
"""Am-241 59.5 keV gamma: per-channel calibration analysis + 3D event displays.

Usage: analyze_gamma.py RUNDIR [--layout config/array_98ch.csv] [--out DIR]

The 13 cm field cage is z in [-65, +65] mm; everything above is outer gas.
Per-channel spectra use in-cage deposits only (the chips can only collect
charge from the cage).  Whole-vessel sums include the outer gas.
"""

import argparse
import json
import pathlib

import numpy as np
import pandas as pd

FULL_E = (55.0, 62.0)   # full-energy window around 59.5 keV


def load_layout(path):
    df = pd.read_csv(path, comment="#")
    return df.rename(columns={"channel": "chip", "x_mm": "x", "y_mm": "y"})


def assign(steps, layout, accept=7.0):
    """Nearest-centre XY attribution to chips (in-cage steps only)."""
    if not len(steps):
        return steps.assign(chip=pd.Series(dtype=str))
    cx, cy = layout.x.to_numpy(), layout.y.to_numpy()
    pts = steps[["x_mm", "y_mm"]].to_numpy()
    chip_idx = np.full(len(steps), -1)
    for s in range(0, len(steps), 200_000):
        p = pts[s:s + 200_000]
        d2 = ((p[:, None, 0] - cx[None, :]) ** 2
              + (p[:, None, 1] - cy[None, :]) ** 2)
        i = d2.argmin(axis=1)
        chip_idx[s:s + 200_000] = np.where(d2[np.arange(len(p)), i] <= accept ** 2, i, -1)
    out = steps.copy()
    out["chip"] = layout.chip.to_numpy()[np.clip(chip_idx, 0, None)]
    return out[chip_idx >= 0]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rundir", type=pathlib.Path)
    ap.add_argument("--layout", required=True, type=pathlib.Path)
    ap.add_argument("--out", type=pathlib.Path)
    args = ap.parse_args()

    out = args.out or args.rundir / "analysis"
    out.mkdir(parents=True, exist_ok=True)
    layout = load_layout(args.layout)

    # geometry from the run config: readout plane at the cylinder bottom,
    # 13 cm field cage above it, source 1 mm above the cathode.
    cfg = {}
    if (args.rundir / "config.json").exists():
        cfg = json.loads((args.rundir / "config.json").read_text())
    length_mm = cfg.get("tpc_length_cm", 60.0) * 10.0
    readout_z = -length_mm / 2.0
    cage_top = readout_z + 130.0
    src = np.array([cfg.get("gun_pos_x_cm", -2.59) * 10.0,
                    cfg.get("gun_pos_y_cm", 4.486) * 10.0,
                    cfg.get("gun_pos_z_cm", -16.9) * 10.0])

    st = pd.read_csv(args.rundir / "run_steps.csv")
    ev = pd.read_csv(args.rundir / "run_events.csv").set_index("eventID")
    gas = args.rundir.name

    st["in_cage"] = st.z_mm.between(readout_z, cage_top)
    cage = st[st.in_cage]
    ev["cage_edep"] = cage.groupby("eventID").edep_keV.sum().reindex(ev.index, fill_value=0.0)
    first_proc = st[st.trackID > 1].sort_values(["eventID", "trackID"]).groupby("eventID").first().creator
    ev["proc"] = first_proc

    interacting = ev[ev.n_hits > 0]
    summary = []
    summary.append(f"gas: {gas}   primaries: {len(ev)}")
    summary.append(f"interacting in vessel: {len(interacting)} ({100*len(interacting)/len(ev):.2f}%)")
    summary.append(f"full-energy (>55 keV) whole vessel: {(interacting.edep_total_keV>55).sum()}"
                   f" ({100*(interacting.edep_total_keV>55).mean():.1f}% of interacting)")
    summary.append(f"full-energy in cage: {(interacting.cage_edep>55).sum()}"
                   f" ({100*(interacting.cage_edep>55).mean():.1f}%)")
    pe = interacting[interacting.proc == 1]
    summary.append(f"first-interaction photoelectric fraction: {100*len(pe)/len(interacting):.1f}%")

    # ---- per-channel spectra -------------------------------------------------
    cs = assign(cage, layout)
    cellsum = cs.groupby(["eventID", "chip"]).edep_keV.sum().reset_index()
    spec = cellsum.groupby("chip").apply(
        lambda d: pd.Series({
            "n_with_deposit": len(d),
            "n_full_energy_channel": int(((d.edep_keV >= FULL_E[0]) & (d.edep_keV <= FULL_E[1])).sum()),
            "mean_edep_keV": d.edep_keV.mean(),
            "median_edep_keV": d.edep_keV.median(),
        }), include_groups=False).reset_index()
    spec = layout.merge(spec, on="chip", how="left").fillna(
        {"n_with_deposit": 0, "n_full_energy_channel": 0})
    spec.to_csv(out / "channel_summary.csv", index=False)
    cellsum.to_csv(out / "channel_event_edep.csv", index=False)

    print("\n".join(summary))
    (out / "summary.txt").write_text("\n".join(summary) + "\n")

    # ---- figures -------------------------------------------------------------
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import RegularPolygon

    # total spectra: vessel vs cage
    fig, ax = plt.subplots(figsize=(7.5, 4.6))
    bins = np.arange(0, 66, 0.5)
    ax.hist(interacting.edep_total_keV, bins=bins, histtype="step", label="whole vessel", lw=1.4)
    ax.hist(interacting.cage_edep, bins=bins, histtype="step", label="in cage (chips see this)", lw=1.4)
    ax.set_yscale("log"); ax.set_xlabel("edep (keV)"); ax.set_ylabel("events / 0.5 keV")
    ax.axvline(59.5, color="r", ls="--", lw=0.8)
    ax.set_title(f"{gas} @1 bar, {len(ev)} primaries: total vs in-cage energy")
    ax.legend(); ax.grid(alpha=0.3)
    fig.tight_layout(); fig.savefig(out / "spectra_total.png", dpi=140); plt.close(fig)

    # first-conversion XY
    first = st.groupby("eventID", sort=False).first()  # first recorded step per event
    fig, ax = plt.subplots(figsize=(7.5, 6))
    h = ax.hist2d(first.x_mm, first.y_mm, bins=(np.arange(-85, 90, 5), np.arange(-70, 75, 5)), cmap="viridis")
    ax.set_aspect("equal"); fig.colorbar(h[3], ax=ax, label="first conversions")
    ax.plot(src[0], src[1], marker="*", color="red", markersize=16, label="source")
    ax.set_xlabel("x (mm)"); ax.set_ylabel("y (mm)"); ax.legend()
    ax.set_title(f"{gas}: first-conversion positions")
    fig.tight_layout(); fig.savefig(out / "conversion_xy.png", dpi=140); plt.close(fig)

    # per-channel full-energy count map
    fig, ax = plt.subplots(figsize=(8, 6.5))
    for _, r in layout.iterrows():
        n = int(spec.loc[spec.chip == r.chip, "n_full_energy_channel"].iloc[0])
        color = plt.cm.viridis(min(n / max(1, spec.n_full_energy_channel.max()), 1.0))
        ax.add_patch(RegularPolygon((r.x, r.y), 6, radius=6.0, facecolor=color, edgecolor="k", lw=0.3))
        if n:
            ax.text(r.x, r.y, str(n), ha="center", va="center", fontsize=5)
    ax.set_aspect("equal"); ax.autoscale_view()
    ax.set_title(f"{gas}: full-energy (55–62 keV) counts per channel")
    ax.set_xlabel("x (mm)"); ax.set_ylabel("y (mm)")
    fig.tight_layout(); fig.savefig(out / "channel_fullenergy_map.png", dpi=140); plt.close(fig)

    # example per-channel spectra: nearest to source, farthest, and highest count
    dist = np.hypot(layout.x - src[0], layout.y - src[1])
    near = layout.chip.iloc[np.argmin(dist)]
    far = layout.chip.iloc[np.argmax(dist)]
    best = spec.loc[spec.n_full_energy_channel.idxmax(), "chip"] if spec.n_full_energy_channel.max() > 0 else near
    fig, axes = plt.subplots(1, 3, figsize=(13, 4), sharey=True)
    for ax, ch, tag in zip(axes, (near, best, far), ("nearest to source", "most full-energy", "farthest")):
        d = cellsum[cellsum.chip == ch].edep_keV
        ax.hist(d, bins=bins, histtype="step", lw=1.3)
        ax.set_yscale("log"); ax.set_xlabel("channel edep (keV)")
        ax.set_title(f"{ch} ({tag})\nN={len(d)}, full={int(((d>=55)&(d<=62)).sum())}", fontsize=9)
        ax.grid(alpha=0.3)
    axes[0].set_ylabel("events / 0.5 keV")
    fig.suptitle(f"{gas}: per-channel spectra")
    fig.tight_layout(); fig.savefig(out / "spectra_channels.png", dpi=140); plt.close(fig)

    # ---- 3D event displays ---------------------------------------------------
    def show_event(eid, fname):
        d = st[st.eventID == eid]
        fig = plt.figure(figsize=(8, 6.5))
        ax = fig.add_subplot(111, projection="3d")
        sc = ax.scatter(d.x_mm, d.y_mm, d.z_mm, c=d.edep_keV, s=6,
                        cmap="inferno", vmin=0)
        fig.colorbar(sc, ax=ax, label="step edep (keV)", shrink=0.7)
        # cage outline
        theta = np.linspace(0, 2 * np.pi, 60)
        for z in (readout_z, cage_top):
            ax.plot(100 * np.cos(theta), 100 * np.sin(theta), np.full_like(theta, z),
                    color="gray", lw=0.7)
        ax.plot(src[0], src[1], src[2], marker="*", color="red", markersize=14)
        ax.set_xlabel("x (mm)"); ax.set_ylabel("y (mm)"); ax.set_zlabel("z (mm)")
        e = ev.loc[eid]
        ax.set_title(f"{gas} event {eid}: proc={int(e.proc) if pd.notna(e.proc) else '-'}, "
                     f"vessel={e.edep_total_keV:.1f} keV, cage={e.cage_edep:.1f} keV")
        ax.set_xlim(-110, 110); ax.set_ylim(-110, 110)
        ax.set_zlim(readout_z, max(cage_top + 80, d.z_mm.max() + 20))
        fig.tight_layout(); fig.savefig(out / fname, dpi=140); plt.close(fig)

    # representative events
    cand = []
    full_pe = interacting[(interacting.proc == 1) & (interacting.edep_total_keV > 58)]
    if len(full_pe):
        cand.append((full_pe.index[0], "event3d_full_pe.png"))
    esc = interacting[(interacting.proc == 1) & (interacting.edep_total_keV > 0)
                      & (interacting.edep_total_keV < 55) & (interacting.zmax_mm > cage_top + 20)]
    if len(esc):
        cand.append((esc.index[0], "event3d_escape.png"))
    ka = interacting[(interacting.edep_total_keV > 26) & (interacting.edep_total_keV < 34)]
    if len(ka):
        cand.append((ka.index[0], "event3d_kalpha_escape.png"))
    co = interacting[interacting.proc == 2]
    if len(co):
        cand.append((co.index[0], "event3d_compton.png"))
    for eid, fname in cand:
        show_event(eid, fname)

    print(f"\noutputs in {out}: summary.txt, channel_summary.csv, "
          f"channel_event_edep.csv, spectra_total.png, conversion_xy.png, "
          f"channel_fullenergy_map.png, spectra_channels.png, "
          + ", ".join(f for _, f in cand))


if __name__ == "__main__":
    main()
