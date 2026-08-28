> **For the original MATLAB/panel-method implementation** described in
> Geyer et al., *"Aerodynamic attitude control of very-low-earth-orbit
> satellites: simulative analysis and insights into nonlinear system
> properties,"* CEAS Space Journal (2025).
> [https://doi.org/10.1007/s12567-025-00684-x](https://doi.org/10.1007/s12567-025-00684-x)
> please visit: [the `legacy-matlab-panel-method` branch](https://github.com/ifrunistuttgart/vleo-aerodynamics-tool/tree/legacy-matlab-panel-method)
> (see tag [`paper-v1.0`](../../releases/tag/paper-v1.0)). Development has
> since moved to a faster, GPU-accelerated toolbox

# AeroSat Toolbox

Modular C++ toolbox for aerodynamic analysis of VLEO satellites, combining
computer-graphics-based surface visibility (shading) with gas-surface interaction (GSI)
models.

Built for research workflows where many configurations must be evaluated efficiently —
attitude changes, flow directions, atmospheric conditions, model parameters.

## Scientific Goal

Initialize geometry/model data once, then compute aerodynamic force and torque quickly
for many parameter combinations.

Typical target applications:
- Comparative studies of GSI models
- Sensitivity analyses for atmosphere/surface parameters
- Fast generation of aerodynamic loads for simulation pipelines (e.g. Sadycos coupling)
- Method development for GPU-based shading and force/torque acceleration

Researcher-oriented use cases:
- Implement and benchmark new aerodynamic/GSI models
- Implement new shading algorithms (CPU or GPU)
- Implement alternative aggregation methods for total force/torque
- Load custom satellite geometries (including `.urdf` workflows)
- Run sweeps over orientation, flow vector, and environment parameters
- Integrate aerodynamic load computation into external simulation frameworks

## Building

### **Prerequisites**
- dependency manager, either of following:
  - [pixi](https://pixi.sh) — the only tool you install manually. It provisions
    everything else (CMake, Ninja, compiler, VTK, assimp, GLEW, GLFW, glm, spdlog) into a
    project-local environment.
  - [vcpkg](https://vcpkg.io/en/) - needed if you don't want to use pixi 
- **Windows only:** Visual Studio 2022 Build Tools (C++ workload). conda-forge can
  locate an installed compiler but can't redistribute `cl.exe` itself, so this one step
  stays manual.
- **Optional:** MATLAB R2024a or later — only needed for the MATLAB bindings.

### **Build and run**
### **Pixi**
```powershell
pixi install                                  # one-time: resolves and downloads all dependencies
pixi run build                                # configure + build the core toolbox and all examples
pixi run run-example import_and_visualize     # run an example (see examples/)
```

Two things worth knowing:
- The build type is `RelWithDebInfo`, not `Debug`. conda-forge's Windows packages are
  Release-only (`/MD`); a `Debug` build (`/MDd`) corrupts memory across the DLL boundary
  between your code and theirs.
- Always launch built executables via `pixi run run-example <name>` (or from a `pixi
  shell`), not by running the `.exe` directly — its DLLs live inside the pixi
  environment, not on your normal `PATH`.

Linux and macOS are supported by the same `pixi.toml`/`pixi.lock`, but only the Windows
path has been fully verified so far.

If you prefer VS Code's CMake Tools UI over the `pixi run` tasks, the same setup is
available as CMake presets: `pixi-debug`, `pixi-release`, `pixi-debug-matlab`.

**MATLAB bindings**

Off by default — building the core toolbox never requires MATLAB.

```powershell
pixi run build-matlab
```

This builds `matlab/bin/MexGateway.mexw64` together with every DLL it needs, copied in
automatically — no `PATH` changes required. Then, in MATLAB:

```matlab
addpath('<repo_root>\matlab')
addpath('<repo_root>\matlab\bin')
test_sentman
```

The MATLAB classes live in the `vat` package (`matlab/+vat/`), so they are addressed as
`vat.Sentman`, `vat.AeroConditions`, and so on. Only `matlab/` itself goes on the path —
never `matlab/+vat/`; MATLAB finds a package through its parent folder.

```matlab
gsi_model = vat.Sentman(1);
geometry = vat.RotatableMeshGeometry('my_satellite.obj');
```

Three viewers are available, each opening a window that blocks until it is closed:

```matlab
vat.show_meshes(geometry)                       % which mesh_id is which part
vat.show_hinges(geometry, hinges)               % check hinge points and axes
vat.show_shading(geometry, visibility, v_rel)   % triangles coloured by shading
```

`vat.show_meshes` labels every mesh `[mesh_id] name`, taking the name from the model
file — that id is what `turn_mesh_around_axis` expects. `vat.show_hinges` takes a struct
array whose fields are exactly that method's arguments, so a hinge can be checked before
it is used:

```matlab
h(1).mesh_id = 0; h(1).origin = [0.5 0 0]; h(1).axis = [0 1 0];
vat.show_hinges(geometry, h)
geometry.turn_mesh_around_axis(h(1).mesh_id, deg2rad(20), h(1).origin, h(1).axis)
```

Use `import vat.*` at the top of a script if you would rather drop the prefix.

See `matlab/+vat/` for the classes, and `matlab/examples/` plus `matlab/test_sentman.m`
for scripts using them.

`MexGateway` only logs warnings/errors by default (per-call INFO logging goes through
the MATLAB Engine API and is slow). Set `MEX_GATEWAY_LOG_LEVEL=INFO` in the
environment before starting MATLAB to see per-call logs — no rebuild needed.

### **vcpkg**
Vcpkg is used in manifest mode. The repository contains a `vcpkg.json` and `vcpkg-configuration.json` to record and reproduce the dependency set.

**Notes and tips:**
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

### Tests

`test/` contains GoogleTest-based tests, but they aren't wired into the CMake build yet.

## Architecture Overview

A small modular C++ library (root: `aero_sat/`) with separate responsibilities so
individual parts can be swapped or extended with minimal friction.

- `aero_sat/core/` — core interfaces, common data types, small utilities.
- `aero_sat/aero_load_calculator/` — load/force/torque aggregation, gluing shading + GSI
  models into a final aerodynamic result.
- `aero_sat/gsi/` — gas–surface interaction models (Sentman, Schuette scaffolding, and
  any future models).
- `aero_sat/geometry/` — mesh/geometry loaders and geometry abstractions (static and
  rotatable meshes).
- `aero_sat/shading_pipeline/` — occlusion/shading implementations, including an OpenGL
  backend under `opengl/`.
- `aero_sat/visualization/` — optional VTK-based viewers used by the examples. Three
  views: `ShowShading` (triangles coloured by visibility, plus the wind
  vector), `ShowMeshes` (each mesh in its own colour with a naming legend), and
  `ShowHinges` (hinge points and rotation axes drawn over a translucent model).
- `examples/` — small standalone example programs (one per subfolder) demonstrating
  toolbox usage, e.g. `examples/import_and_visualize/`.
- `matlab/` — MATLAB MEX gateway plus the `vat` package (`matlab/+vat/`) and
  example/test scripts.
- `test/` — unit and integration tests (GoogleTest).

Design notes:
- Interface-first, strategy-style composition: shading, GSI, and aggregation are
  decoupled so any one of them can be replaced independently.
- Shading backends are intentionally separate from load calculation; shading can run on
  CPU or GPU and be substituted via the pipeline interfaces.

## Conventions

**Terminology**: the geometric vocabulary is strictly hierarchical, and the toolbox
uses these three words and no synonyms for them:

| Term | Meaning |
| --- | --- |
| `triangle` | The smallest unit: a single triangular face. |
| `mesh` | A group of triangles that moves as one rigid body (e.g. one solar panel). A mesh is the unit of rotation; its index is the `mesh_id` passed to `turn_mesh_around_axis`. |
| `geometry` | The complete model, made up of one or more meshes. |

The word *surface* is reserved for its aerodynamic meaning only — surface temperature,
gas–surface interaction — and never refers to a mesh or a triangle.

**Separating meshes in a model file**: export each mesh as an **object** (`o wing_left`
in an `.obj`). One object becomes one mesh, and that is the unit `turn_mesh_around_axis`
rotates. An `.stl` carries no structure at all and becomes a single mesh.

Do **not** use `.obj` groups (`g wing_left`) to separate parts. Groups are a second,
parallel way of dividing an `.obj`, and they are not merged with objects: a file whose
five parts are groups, each holding the six faces of a box, loads as thirty meshes -- one
per face -- so every `mesh_id` addresses a fragment and rotating one tears a single face
off a part. The importer detects this and warns, but it cannot repair it; fix the export.
`vat.show_meshes` shows how a file actually ended up divided.

**Logging**: `spdlog` is used across the project.

**Naming** (PEP 8-inspired, adapted for C++):
- Classes: PascalCase (`StaticMeshGeometry`)
- Interfaces: PascalCase with `I` prefix (`IGeometryShadingData`)
- Functions, variables, member variables: snake_case (`calculate_drag`, `drag_coefficient`)
- Constants: UPPER_SNAKE_CASE (`PI`)
- Folders and files: snake_case

**Namespaces**: everything public lives in `vat` (`vat::Sentman`, `vat::AeroConditions`).
One nested namespace holds internals that are not part of the public API:
- `vat::gl` — the thin OpenGL wrapper layer under `aero_sat/shading_pipeline/opengl/`
  (`vat::gl::Shader`, `vat::gl::VertexBuffer`, ...). Those names are generic enough that
  they would otherwise collide with any other renderer linked into the same program.

The embedded GLSL sources live in an anonymous namespace at the top of the `.cpp` that
uses them (`binary_shader.cpp`, `cop_shader.cpp`), which keeps each backend's shaders
private to that file. Each backend owns its own copy, so they can diverge freely.

The MATLAB interface mirrors this with a `vat` package folder (`matlab/+vat/`).

**Units in identifiers**:
- `__` separates a variable name from its unit suffix
- `_per_` for fractional units, e.g. `__m_per_s`
- Exponents go directly after the unit symbol, e.g. `__m2`

**Velocity definition**: the incoming molecular stream velocity is defined in the
satellite's body frame and referenced as `v_rel_B__m_per_s`:

![velocity_definition.png](velocity_definition.png)

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).
