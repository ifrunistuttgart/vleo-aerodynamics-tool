# AeroSat Toolbox
Modular C++ toolbox for aerodynamic analysis of VLEO satellites, combining computer-graphics-based surface visibility (shading) with gas-surface interaction (GSI) models.

The toolbox is designed for research workflows where many configurations must be evaluated efficiently (attitude changes, flow directions, atmospheric conditions, and model parameters).

## Scientific Goal
The main goal is to initialize geometry/model data once and then compute aerodynamic force and torque quickly for many parameter combinations.

Typical target applications:
- Comparative studies of GSI models
- Sensitivity analyses for atmosphere/surface parameters
- Fast generation of aerodynamic loads for simulation pipelines (for example Sadycos coupling)
- Method development for GPU-based shading and force/torque acceleration

## Researcher-Oriented Use Cases
- Implement and benchmark new aerodynamic/GSI models
- Implement new shading algorithms (CPU or GPU)
- Implement alternative aggregation methods for total force/torque
- Load custom satellite geometries (target path includes custom formats and `.urdf` workflows)
- Run sweeps over orientation, flow vector, and environment parameters
- Integrate aerodynamic load computation into external simulation frameworks


## Architecture Overview

The project is organised as a small modular C++ library (root: `aero_sat/`) with separate responsibilities so individual parts can be swapped or extended with minimal friction.

- `aero_sat/core/` — core interfaces, common data types and small utilities (examples: calculator interfaces, shared value types, configuration structs).
- `aero_sat/aero_load_calculator/` — load/force/torque aggregation and calculator implementations that glue shading + GSI models into a final aerodynamic result.
- `aero_sat/gsi/` — gas–surface interaction model implementations (Sentman, Schuette scaffolding and any future models).
- `aero_sat/satellite/` — mesh, geometry loaders and satellite abstraction types (rotatable meshes)
- `aero_sat/shading_pipeline/` — occlusion/shading implementations and backends (binary-shader utilities and an OpenGL backend under `opengl/`).
- `aero_sat/visualization/` — optional visualization helpers (VTK wrappers and small viewers used by examples).
- `main/` — small native example executables showing how to wire the library together (`main/main.cpp`).
- `matlab/` — MATLAB MEX gateway and example/test scripts for the MATLAB bindings.
- `test/` — unit and integration tests (GoogleTest).

Design notes:
- Interface-first and strategy-style composition: shading, GSI and aggregation are decoupled so you can replace a shading backend or a GSI model independently.
- Shading backends are intentionally separated from the load calculation; shading can be implemented on CPU or GPU and substituted via the pipeline interfaces.


## Logging
`spdlog` is used for logging across the project.

## Naming Conventions
### General
The naming style is inspired by Python PEP 8 while following C++ interface conventions.
- Class names: PascalCase, for example `Satellite`
- Interfaces: PascalCase with `I` prefix, for example `ISatellite`
- Member variables: snake_case
- Functions: snake_case, for example `calculate_drag`
- Variables: snake_case, for example `drag_coefficient`
- Constants: UPPER_SNAKE_CASE, for example `PI`
- folders and files: snake_case

### Units in identifiers
- Use `__` to separate variable name and unit suffix
- Use `_per_` for fractional units, for example `__m_per_s`
- Put exponents directly after unit symbols, for example `__m2`

### Velocity defintion
The velocity of the incoming stream of molecules is defined in the body coordinate system of the satellite an will be referenced as `v_rel_B__m_per_s`. The following sketch illustrates that definition.

![velocity_definition.png](velocity_definition.png)
## Build and Dependencies

- Language standard: C++20
- Build system: CMake (project provides `CMakePresets.json` with commonly used presets)
- Dependency management: vcpkg (manifest mode). The repository contains a `vcpkg.json` and `vcpkg-configuration.json` to record and reproduce the dependency set.

Primary vcpkg manifest dependencies (see `vcpkg.json`):

- `assimp` — mesh import (model loading)
- `glew` / `glfw3` — OpenGL helper libraries used by the shading/visualization backends
- `glm` — header-only math for graphics-friendly vector/matrix types
- `gtest` — unit testing
- `spdlog` — logging
- `vtk` — optional visualization and some transitive numerical/IO dependencies (note: VTK pulls `eigen3` and other helper libs transitively)

Notes and tips:
- Before configuring, ensure your vcpkg installation is available and (when using the presets) that the presets point to the correct vcpkg toolchain file. The presets in `CMakePresets.json` set the toolchain for you when they are used.
- Typical workflow (PowerShell, from project root):

```powershell
# configure via preset (example)
cmake --preset x64-debug

# build
cmake --build out/build/x64-debug --config Debug -- -j 8
```

- Enable MATLAB bindings when you have MATLAB installed by configuring with the CMake option `-DBUILD_MATLAB_BINDINGS=ON` (the top-level `CMakeLists.txt` exposes this option).
- Enable tests with the usual CTest/CMake testing options (tests live in `test/` and use GoogleTest).
- An IDE solution file is generated by the presets/build (for example `AeroSat.slnx` appears under the configured build directory when using the Visual Studio generator).

## examples

The repository contains small example entry points for both native C++ usage and MATLAB integration. Build the project first (see "building").

### matlab

The MATLAB integration is implemented as a MEX gateway. To build the MATLAB bindings:

1. Ensure MATLAB is installed and the CMake `find_package(Matlab)` call can locate it (the examples were validated with MATLAB R2026a).
2. Configure with the MATLAB option and build:

```powershell
cmake --preset x64-debug -- -DBUILD_MATLAB_BINDINGS=ON
cmake --build out/build/x64-debug --config Debug -- -j 8
```

3. After a successful build the MEX file will be placed in `matlab/bin`. From MATLAB you can add that folder to the path and run the example/test scripts included in the `matlab/` folder:

```matlab
addpath(fullfile(pwd, 'matlab', 'bin'))
run('test_sentman.m')  % example test script included in the repo
show_mesh();           % simple visualization helper
```

Refer to the `matlab/` folder for MATLAB example scripts (`Sentman.m`, `HybridAeroLoadCalculator.m`, `RotatableMeshSatellite.m`, and `test_sentman.m`).

### c++

The native C++ example executable is located in the `main/` subproject. After building, run the example executable produced by CMake. Example (PowerShell):

```powershell
# build (if not already built)
cmake --preset x64-debug
cmake --build out/build/x64-debug --config Debug -- -j 8

# run the example executable (adjust path/preset name if you used a different preset)
& "${PWD}\out\build\x64-debug\main\main.exe"
```

The `main` executable demonstrates how the library components are tied together; inspect `main/main.cpp` for a minimal usage example.

## testing

Unit tests are included under the `test/` folder and use GoogleTest.
