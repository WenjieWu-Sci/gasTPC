#!/usr/bin/env python3
"""Forward response: edep -> electrons -> per-channel charge/voltage spectra.

Model (per user constraints):
- n_e = edep_eV / W with Fano-factor fluctuation (Xe: W = 21.9 eV, F = 0.17)
- no electron attachment
- transverse diffusion at 200 V/cm: sigma_T = 1.3 mm/sqrt(cm) * sqrt(drift_cm)
  (nominal value for 1 bar Xe; drift = z_of_step - readout_z)
- gain = 124 mV/fC for every channel (nominal), no measured electronics noise

Usage: forward_response.py RUNDIR [--out DIR]
"""

import argparse
import json
import pathlib

import numpy as np
import pandas as pd

W_EV = 21.9
FANO = 0.17
DIFF_MM_SQRT_CM = 1.3
GAIN_MV_PER_FC = 124.0
Q_E_FC = 1.602176634e-4          # electron charge in fC


def load_layout(path):
    df = pd.read_csv(path, comment="#")
    return df.rename(columns={"channel": "chip", "x_mm": "x", "y_mm": "y"})


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("rundir", type=pathlib.Path)
    ap.add_argument("--layout", required=True, type=pathlib.Path)
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--diff-mm", type=float, default=DIFF_MM_SQRT_CM,
                    help="transverse diffusion coefficient in mm/sqrt(cm)")
    args = ap.parse_args()

    out = args.out or args.rundir / "forward"
    out.mkdir(parents=True, exist_ok=True)
    layout = load_layout(args.layout)

    cfg = json.loads((args.rundir / "config.json").read_text())
    readout_z = -cfg.get("tpc_length_cm", 60.0) * 10.0 / 2.0
    cage_top = readout_z + 130.0

    st = pd.read_csv(args.rundir / "run_steps.csv")
    st = st[st.z_mm.between(readout_z, cage_top)].copy()
    rng = np.random.default_rng(20261116)

    # ---- electrons with Fano fluctuation ----
    mean_e = st.edep_keV.to_numpy() * 1000.0 / W_EV
    n_e = np.maximum(0, np.round(mean_e + rng.normal(0.0, 1.0, len(mean_e))
                                 * np.sqrt(FANO * np.maximum(mean_e, 0)))).astype(np.int64)
    st["n_e"] = n_e
    st = st[st.n_e > 0]

    # ---- transverse diffusion ----
    drift_cm = (st.z_mm.to_numpy() - readout_z) / 10.0
    sigma = args.diff_mm * np.sqrt(np.maximum(drift_cm, 0.0))
    st["x_d"] = st.x_mm + rng.normal(0.0, 1.0, len(st)) * sigma
    st["y_d"] = st.y_mm + rng.normal(0.0, 1.0, len(st)) * sigma

    # ---- chip attribution (nearest centre, 7 mm) ----
    cx, cy = layout.x.to_numpy(), layout.y.to_numpy()
    pts = st[["x_d", "y_d"]].to_numpy()
    idx = np.full(len(st), -1)
    for s in range(0, len(st), 200_000):
        p = pts[s:s + 200_000]
        d2 = ((p[:, None, 0] - cx[None, :]) ** 2 + (p[:, None, 1] - cy[None, :]) ** 2)
        i = d2.argmin(axis=1)
        idx[s:s + 200_000] = np.where(d2[np.arange(len(p)), i] <= 49.0, i, -1)
    st["chip"] = layout.chip.to_numpy()[np.clip(idx, 0, None)]
    st = st[idx >= 0]

    cell = st.groupby(["eventID", "chip"]).agg(n_e=("n_e", "sum"),
                                               edep=("edep_keV", "sum")).reset_index()
    cell["v_mV"] = cell.n_e * Q_E_FC * GAIN_MV_PER_FC
    cell["keV_eq"] = cell.n_e * W_EV / 1000.0
    cell.to_csv(out / "channel_event_charge.csv", index=False)

    # ---- summary: per-channel full-energy and sharing ----
    full = cell[(cell.keV_eq >= 55) & (cell.keV_eq <= 62)]
    per_ch = cell.groupby("chip").agg(n_events=("eventID", "count"),
                                      n_e_total=("n_e", "sum")).reset_index()
    fe = full.groupby("chip").size().rename("n_full").reset_index()
    per_ch = per_ch.merge(fe, on="chip", how="left").fillna({"n_full": 0})
    per_ch = layout.merge(per_ch, on="chip", how="left").fillna(
        {"n_events": 0, "n_e_total": 0, "n_full": 0})
    per_ch.to_csv(out / "channel_summary_forward.csv", index=False)

    nev = st.eventID.nunique()
    split = cell.groupby("eventID").chip.nunique()
    print(f"events with cage deposit: {nev}")
    print(f"charge sharing >1 channel: {(split > 1).mean() * 100:.1f}%")
    print(f"full-energy(55-62 keV-eq) total: {len(full)}")
    print(f"per-channel full-energy: median {per_ch.n_full.median():.0f}, "
          f"min {per_ch.n_full.min():.0f}, max {per_ch.n_full.max():.0f}")

    # ---- figures ----
    import matplotlib
    matplotlib.use("Agg")
    import matplotlib.pyplot as plt
    from matplotlib.patches import RegularPolygon

    plt.rcParams.update({"font.size": 13, "axes.titlesize": 14,
                         "axes.labelsize": 13, "xtick.labelsize": 12,
                         "ytick.labelsize": 12, "legend.fontsize": 12})

    # total spectra: edep vs charge-equivalent
    ev = pd.read_csv(args.rundir / "run_events.csv").set_index("eventID")
    ev["cage_edep"] = ev.index.map(st.groupby("eventID").edep_keV.sum()).fillna(0.0)
    ev["cage_keV_eq"] = ev.index.map(cell.groupby("eventID").keV_eq.sum()).fillna(0.0)
    inter = ev[ev.cage_edep > 0]
    fig, ax = plt.subplots(figsize=(7.5, 4.8))
    bins = np.arange(0, 66, 0.5)
    ax.hist(inter.cage_edep, bins=bins, histtype="step", label="edep in cage", lw=1.4)
    ax.hist(inter.cage_keV_eq, bins=bins, histtype="step", label="charge (Fano + diffusion)", lw=1.4)
    ax.set_yscale("log"); ax.set_xlabel("energy (keV or keV-equiv.)")
    ax.legend(); ax.grid(alpha=0.3)
    ax.set_title(f"{args.rundir.name}: edep vs collected charge")
    fig.tight_layout(); fig.savefig(out / "spectra_forward_total.png", dpi=140); plt.close(fig)

    # full-energy map
    fig, ax = plt.subplots(figsize=(9.5, 7.5))
    vmax = max(1, per_ch.n_full.max())
    for _, r in per_ch.iterrows():
        frac = r.n_full / vmax
        ax.add_patch(RegularPolygon((r.x, r.y), 6, radius=6.0,
                                    facecolor=plt.cm.viridis(frac), edgecolor="k", lw=0.3))
        if r.n_full:
            ax.text(r.x, r.y, str(int(r.n_full)), ha="center", va="center",
                    fontsize=8.5, color="white" if frac < 0.55 else "black")
    ax.set_aspect("equal"); ax.autoscale_view()
    ax.set_title(f"{args.rundir.name}: full-energy counts (forward model)")
    ax.set_xlabel("x (mm)"); ax.set_ylabel("y (mm)")
    fig.tight_layout(); fig.savefig(out / "channel_fullenergy_forward.png", dpi=140); plt.close(fig)

    # example per-channel spectra in mV
    src = np.array([cfg.get("gun_pos_x_cm", 0.0) * 10, cfg.get("gun_pos_y_cm", 0.0) * 10])
    dist = np.hypot(layout.x - src[0], layout.y - src[1])
    near = layout.chip.iloc[np.argmin(dist)]
    best = per_ch.loc[per_ch.n_full.idxmax(), "chip"]
    far = layout.chip.iloc[np.argmax(dist)]
    fig, axes = plt.subplots(1, 3, figsize=(13, 4.2), sharey=True)
    for ax, ch, tag in zip(axes, (near, best, far), ("nearest", "most full", "farthest")):
        d = cell[cell.chip == ch].v_mV
        ax.hist(d, bins=np.arange(0, 70, 0.5), histtype="step", lw=1.3)
        ax.set_yscale("log"); ax.set_xlabel("channel signal (mV)")
        ax.set_title(f"{ch} ({tag})\nN={len(d)}, full={int(per_ch.loc[per_ch.chip == ch, 'n_full'].iloc[0])}",
                     fontsize=11)
        ax.grid(alpha=0.3)
    fig.suptitle(f"{args.rundir.name}: per-channel spectra (124 mV/fC)")
    fig.tight_layout(); fig.savefig(out / "spectra_forward_channels.png", dpi=140); plt.close(fig)
    print(f"outputs in {out}")


if __name__ == "__main__":
    main()
