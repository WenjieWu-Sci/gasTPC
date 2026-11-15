#!/usr/bin/env python3
"""Drive gasTPC pressure scans for the alpha-array cross-check.

One run per (pressure, source height).  A per-run JSON is generated from
config/study_base.json next to this script; outputs land in
<outdir>/<study>_p<P>_z<Z>/.  Pure stdlib.

Usage:
    python3 run_scan.py --study A [--jobs 16] [--events 1000] [--out DIR]
"""

import argparse
import concurrent.futures as cf
import json
import pathlib
import subprocess
import sys

GASTPC = "/home/wuwenjie/gasTPC/build/gasTPC"
HERE = pathlib.Path(__file__).resolve().parent
BASE_CFG = HERE / "config" / "study_base.json"

PLANE_Z_MM = -300.0  # cylinder bottom face = readout plane (60 cm length)

STUDIES = {
    # XY-only: horizontal fan beam, theta fixed at 90 deg.
    "A": dict(mode="cone", heights=[120],
              pressures=[round(0.2 * i, 2) for i in range(1, 16)],
              extra={"cone_theta_min_deg": 90.0, "cone_theta_max_deg": 90.0}),
    # +Z consideration: full downward fan.
    "B": dict(mode="cone", heights=[120, 240],
              pressures=[round(0.2 + 0.1 * i, 2) for i in range(9)],
              extra={"cone_theta_min_deg": 90.0, "cone_theta_max_deg": 180.0}),
    # Slit: area source + geometric transmission through the tilted hole.
    "C": dict(mode="slit", heights=[120, 240],
              pressures=[round(0.2 + 0.1 * i, 2) for i in range(9)],
              extra={"slit_tilt_deg": 30.0, "slit_hole_diameter_mm": 0.5,
                     "slit_thickness_mm": 1.0, "slit_tilt_azimuth_deg": 0.0,
                     "source_disk_diameter_mm": 10.0}),
}


def one_run(study, p, z_mm, events, outroot, cos_theta=False):
    tag = f"{study}_p{p:g}_z{z_mm:g}"
    rundir = outroot / tag
    rundir.mkdir(parents=True, exist_ok=True)

    cfg = json.loads(BASE_CFG.read_text())
    cfg.update(STUDIES[study]["extra"])
    cfg["source_mode"] = STUDIES[study]["mode"]
    if cos_theta:
        cfg["cone_theta_cosine"] = True
    cfg["gas_pressure_bar"] = p
    cfg["gun_pos_z_cm"] = (PLANE_Z_MM + z_mm) / 10.0
    if STUDIES[study]["mode"] == "slit":
        # slit top opening 1 mm below the source disk
        cfg["slit_center_mm"] = f"0,0,{PLANE_Z_MM + z_mm - 1.0:g}"
    cfg["random_seed"] = 100000 + int(p * 1000) * 10 + int(z_mm)
    cfg["output_file"] = str(rundir / "run")
    (rundir / "config.json").write_text(json.dumps(cfg, indent=4))

    mac = rundir / "run.mac"
    mac.write_text(f"/run/verbose 0\n/event/verbose 0\n/run/beamOn {events}\n")

    log = rundir / "run.log"
    with log.open("w") as lf:
        rc = subprocess.run([GASTPC, "--config", str(rundir / "config.json"),
                             str(mac)], cwd=rundir, stdout=lf,
                            stderr=subprocess.STDOUT).returncode
    ok = rc == 0 and (rundir / "run_steps.csv").exists()
    return tag, ok


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--study", required=True, choices=list(STUDIES))
    ap.add_argument("--jobs", type=int, default=16)
    ap.add_argument("--events", type=int, default=1000)
    ap.add_argument("--out", type=pathlib.Path,
                    default=pathlib.Path("/home/wuwenjie/Data/gasTPC-crosscheck"))
    ap.add_argument("--cos-theta", action="store_true",
                    help="sample cos(theta) uniformly (solid-angle) instead of theta")
    args = ap.parse_args()

    spec = STUDIES[args.study]
    name = f"study-{args.study}" + ("-costheta" if args.cos_theta else "")
    outroot = args.out / name
    jobs = [(args.study, p, z, args.events, outroot, args.cos_theta)
            for p in spec["pressures"] for z in spec["heights"]]

    print(f"study {args.study}: {len(jobs)} runs -> {outroot}")
    failed = []
    with cf.ThreadPoolExecutor(max_workers=args.jobs) as ex:
        for tag, ok in ex.map(lambda a: one_run(*a), jobs):
            print(("OK  " if ok else "FAIL") + f" {tag}", flush=True)
            if not ok:
                failed.append(tag)
    if failed:
        print(f"FAILED: {failed}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
