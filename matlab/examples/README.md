# VAT MATLAB Examples

This directory contains introductory tutorials, workflow examples, and verification benchmarks demonstrating how to use the VAT (VLEO Aerodynamics Toolbox) MATLAB interface for aerodynamic analysis in Very Low Earth Orbit (VLEO).

---

## Getting Started & Prerequisites

Before running any of the examples, ensure that the MATLAB MEX binaries and dependencies are built and available:

1. **Build the MATLAB binaries** from the repository root:
   ```bash
   pixi run build-matlab
   ```
2. **Open MATLAB** and initialize the project environment by either:
   - Opening/loading the MATLAB project file `Vat.prj` in the repository root (recommended), or
   - Adding the required paths in MATLAB:
     ```matlab
     addpath('matlab');
     addpath('matlab/bin');
     ```

---

## Directory Overview

```text
matlab/examples/
├── ex01_quickstart.m             # Basic aerodynamic force and torque calculation
├── ex02_geometry_and_hinges.m    # Multi-body geometry inspection and hinge rotations
├── ex03_remeshing.m              # Mesh quality inspection, remeshing, caching, and auto pixel sizing
├── ex04_gsi_models.m             # Gas-Surface Interaction (GSI) models comparison
├── ex05_shading.m                # Shading pipelines (Binary vs. CoP) and shadowing effects
├── ex06_attitude_sweep.m         # Angle-of-attack sweeps (drag, lift, pitch torque)
├── ex07_wing_deflection.m        # Deflecting control surfaces / hinged wings
├── ex08_soar_torque_map.m        # 2D aerodynamic torque map across wind directions
├── geometries/                   # OBJ geometry files used across examples
└── benchmarks/                   # Analytical and numerical validation benchmarks
    ├── analytic_plates.m         # Two-plate shadowing benchmark vs. analytic solution
    ├── exact_solution_shadowing.m# Analytic reference helper for shadowing benchmark
    ├── smoothness.m              # Load smoothness and discretization jumps across attitudes
    ├── onesided_plate_cl_cd.m    # Flat plate lift and drag coefficients across all GSI models
    └── two_sided_plate_gsi.m     # Two-sided flat plate aerodynamic validation
```

---

## Tutorial Examples (`ex01` – `ex08`)

The numbered examples provide a step-by-step introduction to the core functionality of VAT:

### [`ex01_quickstart.m`](ex01_quickstart.m) — Quick Start
- **What it shows:** An end-to-end aerodynamic calculation in VLEO.
- **Workflow:** Sets up atmospheric conditions (`vat.AeroConditions`), loads a 3D geometry (`vat.geometry.RotatableMeshGeometry`), configures a Gas-Surface Interaction model (`vat.gsi_models.Sentman`), initializes the hardware-accelerated shading pipeline (`vat.shading.ShadingPipeline`), and computes aerodynamic force and torque using `vat.loads.HybridForceTorqueCalculator`.

### [`ex02_geometry_and_hinges.m`](ex02_geometry_and_hinges.m) — Geometry and Hinges
- **What it shows:** How to inspect sub-meshes in multi-part OBJ files and define kinematic hinges for movable surfaces (e.g. wings, drag sails, solar panels).
- **Features:** Visual inspection with `vat.visualization.show_meshes` and `vat.visualization.show_hinges`, followed by rotating mesh elements around body-relative hinge axes using `turn_mesh_around_axis`.

### [`ex03_remeshing.m`](ex03_remeshing.m) — Mesh Quality & Remeshing
- **What it shows:** How to inspect CAD mesh quality, remesh into uniform near-equilateral triangles, save and reload cached meshes, and leverage automatic shading raster sizing.
- **Features:** Demonstrates why CAD exports (which have narrow sliver triangles on rims/edges) cause aliasing near shadow edges, shows how to predict and execute remeshing while preserving sub-mesh IDs and hinges, and compares forces and runtimes between raw CAD and remeshed models.

### [`ex04_gsi_models.m`](ex04_gsi_models.m) — Gas-Surface Interaction Models
- **What it shows:** Comparison of all six available GSI models in VAT:
  - `Newton` (specular/impact)
  - `Cook`
  - `Maxwell`
  - `SchaafChambre`
  - `Storch`
  - `Sentman`
- **Features:** Compares drag and lift at an angle of attack and illustrates tuning model parameters (such as the energy accommodation coefficient $\alpha_E$) without needing to re-render shading or rebuild the calculator.

### [`ex05_shading.m`](ex05_shading.m) — Aerodynamic Shading
- **What it shows:** How self-shadowing and body-to-panel shadowing are handled in hypersonic free-molecular flow.
- **Features:** Compares the two shading algorithms:
  - **Binary (algorithm 0):** Marks a triangle visible if any raster pixel hits it.
  - **CoP / Center-of-Pressure (algorithm 1):** Marks a triangle visible if its center is exposed to the oncoming flow.
- Visualizes shadowed vs. illuminated triangles in 3D using `vat.visualization.show_shading`.

### [`ex06_attitude_sweep.m`](ex06_attitude_sweep.m) — Loads Over Attitude
- **What it shows:** Efficient attitude sweeps (angle of attack variation) computing drag, lift, and pitching torque ($T_y$).
- **Features:** Demonstrates fast GPU rasterization performance, evaluating dozens of attitudes in fractions of a second.

### [`ex07_wing_deflection.m`](ex07_wing_deflection.m) — Deflecting Wings & Aerodynamic Control
- **What it shows:** Dynamic deflection of articulated surfaces and its impact on satellite forces and pitching moments.
- **Features:** Demonstrates shielding effects where deflected surfaces enter the wake/shadow of the satellite body.

### [`ex08_soar_torque_map.m`](ex08_soar_torque_map.m) — Aerodynamic Torque Map
- **What it shows:** Generation of a complete 2D aerodynamic torque map ($T_x, T_y, T_z$) over angle of attack ($\alpha$) and sideslip angle ($\beta$).
- **Application:** Demonstrates fast multi-directional evaluation suitable for orbit and attitude propagation without pre-calculated lookup tables.

---

## Verification & Benchmarks (`benchmarks/`)

Scripts located in the [`benchmarks/`](benchmarks/) folder validate VAT against analytical solutions and investigate numerical properties:

- **[`benchmarks/analytic_plates.m`](benchmarks/analytic_plates.m)**: Compares hybrid shading results against the closed-form analytical solution (`exact_solution_shadowing.m`) for two partially overlapping flat plates across various raster resolutions and flow angles.
- **[`benchmarks/smoothness.m`](benchmarks/smoothness.m)**: Analyzes load smoothness and discretization jumps across continuous attitude variations as a function of raster pixel resolution.
- **[`benchmarks/onesided_plate_cl_cd.m`](benchmarks/onesided_plate_cl_cd.m)**: Computes lift ($C_L$) and drag ($C_D$) coefficients for a unit flat plate across all GSI models over flow angles from $0^\circ$ to $90^\circ$.
- **[`benchmarks/two_sided_plate_gsi.m`](benchmarks/two_sided_plate_gsi.m)**: Validates aerodynamic loads on a two-sided flat plate.

---