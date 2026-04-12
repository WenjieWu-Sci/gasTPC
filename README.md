# gasTPC – Geant4 Gas Time Projection Chamber Simulation

A complete C++17 Geant4 simulation of a cylindrical gas TPC supporting
**Xe**, **SF6**, and **SeF6** fill gases.

## Features

| Feature | Details |
|---------|---------|
| Geometry | Built-in C++ cylinder **or** external GDML file |
| Gas model | W-value · Fano-factor fluctuations · electron attachment |
| EM physics | `G4EmStandardPhysics_option4` (best accuracy for gas) |
| Electric field | Configurable uniform field (V/cm) on drift volume |
| Output | ROOT `TTree` (optional) or plain CSV summary |
| Configuration | Simple flat JSON files – no external JSON library |

## Project Structure

```
CMakeLists.txt
include/          – C++ header files
src/              – C++ source files (+ main.cc)
macros/           – Geant4 macro scripts
config/           – JSON configuration files
gdml/             – GDML geometry file
```

## Requirements

| Dependency | Version | Notes |
|------------|---------|-------|
| CMake      | ≥ 3.16  | |
| Geant4     | ≥ 10.7  | built with `gdml`, `ui_all`, `vis_all` |
| ROOT       | ≥ 6.x   | **optional** – enables TTree output |
| C++        | 17      | |

## Build

```bash
mkdir build && cd build
cmake .. -DGeant4_DIR=/path/to/geant4/lib/cmake/Geant4
make -j$(nproc)
```

With ROOT:
```bash
cmake .. -DGeant4_DIR=... -DROOT_DIR=/path/to/root
```

## Run

```bash
# Interactive (visualisation)
./gasTPC

# Batch – built-in geometry, 100 × 662 keV gammas in Xe
./gasTPC macros/run_builtin.mac

# Batch – GDML geometry (set geometry_mode="gdml" in config/detector.json first)
./gasTPC macros/run_gdml.mac

# Custom config
./gasTPC --config /path/to/detector.json macros/run_builtin.mac
```

## Configuration

Edit `config/detector.json` before building or running:

```json
{
    "geometry_mode"       : "builtin",
    "gdml_file"           : "gdml/simple_tpc.gdml",
    "gas"                 : "Xe",
    "tpc_radius_cm"       : 10.0,
    "tpc_length_cm"       : 20.0,
    "anode_thickness_cm"  : 0.1,
    "cathode_thickness_cm": 0.1,
    "output_file"         : "output",
    "particle"            : "gamma",
    "energy_keV"          : 662.0
}
```

`config/field.json` controls the drift field:
```json
{ "field_strength_V_per_cm": 500.0 }
```

## Gas Parameters

| Gas  | W-value (eV) | Fano | Attachment (cm⁻¹) | v_drift (cm/µs) |
|------|-------------|------|-------------------|-----------------|
| Xe   | 21.9        | 0.17 | 0.00              | 2.0             |
| SF6  | 34.0        | 0.57 | 0.05              | 0.1             |
| SeF6 | 33.0        | 0.55 | 0.03              | 0.2             |

## Output

With ROOT: `output.root` containing a `tpc` TTree with per-hit branches
`eventID`, `nHits`, `posX/Y/Z` (mm), `edep` (keV), `time` (ns),
`trackID`, `pdg`, `nElectrons`.

Without ROOT: `output_summary.csv` with aggregate run statistics.

## License

See [LICENSE](LICENSE).