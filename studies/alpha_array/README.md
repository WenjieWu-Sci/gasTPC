# Alpha-array cross-check (studies/alpha_array)

Independent gasTPC reproduction of the 2026-11-09 feasibility report
"256 阵列测量 Ar 中 α 粒子可行性研究_考虑狭缝" (WeChat PPTX, no data tables;
reference curves digitised from figures, tolerance ±2–3 pp).

## What is simulated

- Am-241 alpha, 5.486 MeV, in Ar at 0.2–3.0 bar (ideal-gas density scaling).
- Geometry: gas volume only (built-in cylinder, 35 cm radius, 60 cm long,
  bottom face = readout plane).  No silicon volumes: chip charge assignment is
  done offline by XY projection onto the 49-cell hex strip (model A —
  25 sensitive + 24 veto cells, pitch 11.5 mm, channel 21 at the origin).
- Studies:
  - **A** (XY only): source 120 mm above plane, theta = 90°, phi in ±10.23°.
    Cuts: no deposit in any veto cell column; per-cell ≤ 1160 keV.
  - **B** (+Z): theta in (90°, 180°) — solid-angle uniform
    (`cone_theta_cosine: true`), phi ±10.23°, source at 120/240 mm.
    Adds cut: gas deposit z_min must not touch the readout plane.
  - **C** (slit): 10 mm disk source, directions accepted only if they
    geometrically thread the tilted hole (1 mm plate, 0.5 mm hole,
    30° to horizontal, azimuth +X).  Per-cell threshold 1052 keV.

## Run

On a host with Geant4 (>= 11.4) and the built `gasTPC` binary:

```bash
python3 run_scan.py --study A --jobs 24 --events 1000   # B, C likewise
python3 analyze.py study /home/wuwenjie/Data/gasTPC-crosscheck/study-A --study A
python3 analyze.py study .../study-B --study B --zcut
python3 analyze.py study .../study-C --study C --zcut --thr 1052
python3 analyze.py track-length /home/wuwenjie/Data/gasTPC-crosscheck/track-length
```

Outputs per run: `run_steps.csv` (per-step edep), `run_events.csv`
(per-event aggregates + primary vertex/direction), `run.log`, `config.json`.

## Cross-check findings (1000 events/point)

| Report claim | gasTPC | Verdict |
| --- | --- | --- |
| Track length 45.54 mm @1 bar, ∝1/P | 43.07 mm @1 bar, ∝1/P exact | scaling exact; absolute 5.4% shorter (physics-list level) |
| A: optimum 0.6–0.8 bar, peak ~22% | 21.3% @0.6, 19.6% @0.8 | reproduced (−0.7/−3.4 pp) |
| B: optimum 0.5 bar, peak ~37% | 33.7–34.3% @0.5 with cosθ sampling | optimum reproduced; magnitude within ~3–5 pp |
| C: optimum 0.4–0.5 bar, ~16–17% | 17.1% @0.5 ✓, but 32% @0.4 (2× report) | optimum reproduced; 0.4-bar point sensitive to exact slit position/orientation |

Assumptions/offsets to keep in mind: solid-angle (cosθ-uniform) sampling
matches the report much better than θ-uniform — the report's convention is
unspecified; the slit hole azimuth/offset relative to the array is our
inference; cell pitch/gap from the figure (11.5/1.5 mm); report curves were
digitised from PNG (±2–3 pp).
