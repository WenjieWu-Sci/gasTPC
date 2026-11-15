#!/usr/bin/env python3
"""Cut analysis and report comparison for the alpha-array cross-check.

Model-A (XY projection) attribution: every gas energy deposit is assigned to
the readout cell whose centre is nearest in XY (within CELL_ACCEPT_MM); the
deposit then counts towards that cell's collected charge.  Cells are the
49-channel strip of the report figure (25 sensitive + 24 veto).

Usage:
    analyze.py track-length <dir-with-track_p*_steps.csv>
    analyze.py study <dir-with-A_p*_z*> --study A [--zcut] [--thr 1160]
"""

import argparse
import glob
import pathlib
import sys

import numpy as np
import pandas as pd

# ---------------------------------------------------------------- layout ----
# Pointy-top hexagons, horizontal pitch 11.5 mm, row pitch 9.96 mm,
# alternate rows offset by half a pitch (matches the report's 49-cell strip;
# channel 21 at the origin).
PITCH = 11.5
ROW_DY = 9.96
CELL_ACCEPT_MM = 7.5          # nearest-centre acceptance (covers 1.5 mm gaps)

ROWS = [
    # (row j, y, x of first cell, n cells, first channel id)
    ( 2,  2 * ROW_DY,   0.0,        9,  1),   # ch 1-9   veto
    ( 1,      ROW_DY,  -0.5*11.5,  10, 10),   # ch 10-19 (10,19 veto)
    ( 0,        0.0,  -11.5,       11, 20),   # ch 20-30 (20,30 veto)
    (-1,     -ROW_DY,  -0.5*11.5,  10, 31),   # ch 31-40 (31,40 veto)
    (-2, -2 * ROW_DY,   0.0,        9, 41),   # ch 41-49 veto
]
SENSITIVE = set(range(11, 19)) | set(range(21, 30)) | set(range(32, 40))


def build_layout(path=None):
    """Cell table with columns chip, x, y, role.

    With a path: read the real 98-chip layout CSV
    (channel,position,x_mm,y_mm,role).  Otherwise: the report's 49-cell
    strip (for cross-check reproduction).
    """
    if path:
        df = pd.read_csv(path, comment="#")
        df = df.rename(columns={"channel": "chip", "x_mm": "x", "y_mm": "y"})
        return df[["chip", "x", "y", "role"]]
    cells = []  # (chipID, x, y, role)
    for j, y, x0, n, ch0 in ROWS:
        for i in range(n):
            ch = ch0 + i
            cells.append((ch, x0 + i * 11.5, y,
                          "sensitive" if ch in SENSITIVE else "veto"))
    return pd.DataFrame(cells, columns=["chip", "x", "y", "role"])


def assign_cells(df, layout, accept_mm=CELL_ACCEPT_MM):
    """Return per-(eventID, chip) summed edep_keV (nearest-centre, XY)."""
    pts = df[["x_mm", "y_mm"]].to_numpy()
    cx = layout["x"].to_numpy()
    cy = layout["y"].to_numpy()
    # chunked nearest-centre search
    chip_idx = np.full(len(df), -1)
    for s in range(0, len(df), 200_000):
        p = pts[s:s + 200_000]
        d2 = ((p[:, None, 0] - cx[None, :]) ** 2
              + (p[:, None, 1] - cy[None, :]) ** 2)
        i = d2.argmin(axis=1)
        chip_idx[s:s + 200_000] = np.where(d2[np.arange(len(p)), i]
                                           <= accept_mm ** 2, i, -1)
    out = df.assign(chip=layout["chip"].to_numpy()[np.clip(chip_idx, 0, None)])
    out = out[chip_idx >= 0]
    return out.groupby(["eventID", "chip"])["edep_keV"].sum()


# ------------------------------------------------------- reference values ----
# Digitised from the report PNGs (no data tables available); tolerance ±2-3 pp.
REPORT = {
    "A": dict(p=[0.2, 0.4, 0.6, 0.8, 1.0, 1.2, 1.4, 1.6, 1.8, 2.0, 2.2, 2.4,
                 2.6, 2.8, 3.0],
              veto=[100, 100, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
              sat=[0, 0, 78, 77, 99, 100, 100, 100, 100, 100, 100, 100, 100,
                   100, 100],
              acc=[0, 0, 22, 23, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0]),
    "B12": dict(p=[0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0],
                acc=[0, 5, 20.5, 37, 18, 10, 8, 0, 1.5]),
    "B24": dict(p=[0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0],
                acc=[4, 8.5, 20.5, 37, 18, 10.5, 8.5, 0, 1.5]),
    "C12": dict(p=[0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0],
                acc=[0, 0, 16, 17, 9, 4, 0, 0, 0]),
    "C24": dict(p=[0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0],
                acc=[0, 0, 16, 17, 9, 4, 0, 0, 0]),
}

PLANE_Z_MM = -300.0
GAS_R_MM = 350.0
GAS_TOP_MM = 300.0


# ------------------------------------------------------------- analysis ------
def analyse_run(rundir, layout, thr, use_zcut, accept_mm=CELL_ACCEPT_MM):
    rundir = pathlib.Path(rundir)
    # geometry from the run's own config (fallback: original 60 cm cylinder)
    plane_z, gas_r, gas_top = PLANE_Z_MM, GAS_R_MM, GAS_TOP_MM
    cfgfile = rundir / "config.json"
    if cfgfile.exists():
        import json
        cfg = json.loads(cfgfile.read_text())
        length_mm = cfg.get("tpc_length_cm", 60.0) * 10.0
        plane_z = -length_mm / 2.0
        gas_top = plane_z + length_mm
        gas_r = cfg.get("tpc_radius_cm", 35.0) * 10.0

    steps = pd.read_csv(rundir / "run_steps.csv")
    events = pd.read_csv(rundir / "run_events.csv")

    # out-of-gas exclusions (side/top boundary reached; a source placed at the
    # top face does not count as exiting)
    excl = ((events["rmax_mm"] >= gas_r - 2)
            | ((events["zmax_mm"] >= gas_top - 2)
               & (events["z0_mm"] < gas_top - 2)))
    events = events[~excl]
    steps = steps[steps["eventID"].isin(events["eventID"])]

    cellsum = assign_cells(steps, layout, accept_mm)  # (eventID, chip) -> keV
    ev = events.set_index("eventID")
    n = len(ev)

    veto = np.zeros(len(ev), bool)
    sens_max = pd.Series(0.0, index=ev.index)
    role = layout.set_index("chip")["role"]
    for (eid, chip), kev in cellsum.items():
        if role[chip] == "veto":
            veto[ev.index.get_loc(eid)] |= kev > 0
        else:
            if kev > sens_max.get(eid, 0.0):
                sens_max[eid] = kev
    sat = (sens_max > thr).to_numpy()
    trunc = (ev["zmin_mm"] <= plane_z + 2.0).to_numpy() if use_zcut else np.zeros(n, bool)

    res = dict(n=n, excluded=int(excl.sum()),
               veto_only=int((veto & ~sat & ~trunc).sum()),
               sat_only=int((sat & ~veto & ~trunc).sum()),
               plane_only=int((trunc & ~veto & ~sat).sum()),
               multiple=int(((veto.astype(int) + sat + trunc) >= 2).sum()),
               accepted=int((~veto & ~sat & ~trunc).sum()))
    return res


def cmd_study(args):
    layout = build_layout(args.layout)
    rows = []
    for d in sorted(pathlib.Path(args.dir).glob(f"{args.study}_p*_z*")):
        p = float(d.name.split("_p")[1].split("_z")[0])
        z = float(d.name.split("_z")[1])
        r = analyse_run(d, layout, args.thr, args.zcut,
                        accept_mm=7.0 if args.layout else CELL_ACCEPT_MM)
        rows.append(dict(study=args.study, p=p, z=z, **r))
    df = pd.DataFrame(rows).sort_values(["z", "p"])
    for c in ["veto_only", "sat_only", "plane_only", "multiple", "accepted"]:
        df[c + "_pct"] = 100.0 * df[c] / df["n"]

    pd.set_option("display.width", 160)
    print(df[["p", "z", "n", "excluded", "veto_only_pct", "sat_only_pct",
              "plane_only_pct", "multiple_pct", "accepted_pct"]].to_string(
              index=False, float_format="%.1f"))

    # report comparison on 'accepted'
    for z in sorted(df["z"].unique()):
        sub = df[df["z"] == z]
        refkey = "A" if args.study == "A" else f"{args.study}{int(round(z / 10))}"
        ref = REPORT.get(refkey)
        print(f"\n--- z={z:g} mm: sim accepted% vs report ({refkey}) ---")
        sim_map = dict(zip(sub["p"], sub["accepted_pct"]))
        if ref:
            for pr, ra in zip(ref["p"], ref["acc"]):
                sv = sim_map.get(pr, float("nan"))
                print(f"  P={pr:4.2f}  sim={sv:6.2f}%   report={ra:6.2f}%   "
                      f"diff={sv - ra:+7.2f} pp")
    outcsv = pathlib.Path(args.dir) / f"analysis_{args.study}.csv"
    df.to_csv(outcsv, index=False)
    print(f"\nwrote {outcsv}")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
        fig, ax = plt.subplots(figsize=(7, 4.5))
        for z in sorted(df["z"].unique()):
            sub = df[df["z"] == z]
            ax.plot(sub["p"], sub["accepted_pct"], "o-", label=f"sim z={z:g}")
            ref = REPORT.get("A" if args.study == "A" else f"{args.study}{int(round(z / 10))}")
            if ref:
                ax.plot(ref["p"], ref["acc"], "s--", alpha=0.6,
                        label=f"report z={z:g}")
        ax.set_xlabel("Ar pressure (bar)")
        ax.set_ylabel("accepted (%)")
        ax.set_title(f"Study {args.study}: gasTPC vs report (digitised ±2-3 pp)")
        ax.legend()
        ax.grid(alpha=0.3)
        fig.tight_layout()
        png = pathlib.Path(args.dir) / f"compare_{args.study}.png"
        fig.savefig(png, dpi=140)
        print(f"wrote {png}")
    except Exception as e:
        print(f"plot skipped: {e}", file=sys.stderr)


def cmd_track_length(args):
    import re
    print(f"{'P_bar':>7} {'sim_L_mm':>10} {'sem':>8}")
    for f in sorted(glob.glob(str(pathlib.Path(args.dir) / "track_p*_steps.csv"))):
        p = float(re.search(r"track_p([\d.]+)_steps", f).group(1))
        df = pd.read_csv(f)
        a = df[df.trackID == 1]
        L = a.groupby("eventID").apply(
            lambda d: np.sqrt((d.x_mm ** 2 + d.y_mm ** 2
                               + (d.z_mm + 290) ** 2)).max())
        print(f"{p:7.1f} {L.mean():10.3f} {L.sem():8.3f}")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    sub = ap.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("study")
    s.add_argument("dir")
    s.add_argument("--study", required=True, choices=["A", "B", "C"])
    s.add_argument("--zcut", action="store_true")
    s.add_argument("--thr", type=float, default=1160.0)
    s.add_argument("--layout", help="external chip layout CSV (98-chip real board)")
    t = sub.add_parser("track-length")
    t.add_argument("dir")
    args = ap.parse_args()
    (cmd_study if args.cmd == "study" else cmd_track_length)(args)


if __name__ == "__main__":
    main()
