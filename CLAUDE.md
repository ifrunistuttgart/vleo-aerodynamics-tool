# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

**AeroSat** — a C++20 toolbox that computes aerodynamic force and torque on VLEO satellites
by combining GPU rasterization-based surface visibility ("shading") with gas–surface
interaction (GSI) physics. Optional MEX bindings expose the same pipeline to MATLAB.

The MATLAB panel-method implementation from the CEAS Space Journal paper lives on the
`legacy-matlab-panel-method` branch, not here.

## Build and run

Dependencies come from **pixi** (conda-forge). On Windows, VS 2022 Build Tools must still be
installed manually (conda-forge can find `cl.exe` but cannot redistribute it).

```powershell
pixi install                                  # one-time
pixi run build                                # configure + build core + examples
pixi run run-example compute_force_and_torque # or: import_and_visualize
pixi run build-matlab                         # opt-in MEX bindings (requires MATLAB)
```

Two presets, sharing one build tree at `out/build/dev`: `dev` and `dev-matlab`, which
differs only in `BUILD_MATLAB_BINDINGS`. `[activation.env]` in [pixi.toml](pixi.toml) clears
`CMAKE_GENERATOR*`/`CMAKE_ARGS` for every task — conda-forge's compiler activation exports
VS-generator defaults that would override Ninja.

Four non-obvious build facts:

- **`dev` is `RelWithDebInfo`, i.e. already optimized** (`/O2 /Ob1 /DNDEBUG` plus `-ZI` for
  symbols) — it is not a slow build. A real `Debug` build is deliberately not offered:
  conda-forge's Windows C++ packages are Release-only (`/MD`), and a `/MDd` build corrupts
  memory across the DLL boundary into VTK/assimp/spdlog. Do not add one.
- **The `CMP0141` block in [CMakeLists.txt](CMakeLists.txt#L3-L7) is load-bearing, not stale.**
  Under `CMP0141 NEW` CMake drops `/Zi` from `CMAKE_CXX_FLAGS_RELWITHDEBINFO` (verified: it is
  just `/O2 /Ob1 /DNDEBUG`) and takes debug-info format from `MSVC_DEBUG_INFORMATION_FORMAT`
  instead. That block is the only reason `-ZI` reaches the compiler. Delete it and you silently
  lose every `.pdb`. It works fine under Ninja; only its German comment is worth changing.
- **Never run built `.exe`s directly.** Their DLLs live inside the pixi env, not on `PATH`.
  Go through `pixi run run-example <name>` or a `pixi shell`.
- **`matlab/bin/` is self-contained by design**, and is a staging directory owned entirely by
  [cmake/stage_matlab_bin.cmake](cmake/stage_matlab_bin.cmake). `MexGateway` links into the
  build tree and is copied here with every DLL it resolves. It must **not** be linked directly
  into `matlab/bin`: `file(GET_RUNTIME_DEPENDENCIES)` searches a module's own directory first,
  so the deps would resolve to the already-staged copies and never refresh. The script asserts
  against that. A running MATLAB keeps these files mapped, so a rebuild needs `clear mex`.

### Tests

GoogleTest suites under [test/](test/) are wired via `gtest_discover_tests`. `BUILD_TESTING`
is ON in the `base` preset and both test presets set `execution.noTestsAction=error`, so an
empty discovery fails instead of reporting success.

`pixi run test` builds and runs the suite: **38 tests, 35 passing.** The three failures are
real and pre-existing, not build breakage — see the work queue.

```powershell
pixi run test                                         # the normal path
ctest --preset dev -R <regex>                         # subset by test name
out/build/dev/test/gsi/gsi_test --gtest_filter=Foo.*  # single test, from a pixi shell
```

Test binaries: `gsi_test`, `satellite_test`, `shading_pipeline_test`, `calculator_test`. The
shading and calculator suites need a working OpenGL context (GLFW hidden window).

## Architecture

Strategy-style composition around four interfaces. The concrete pieces are interchangeable;
nothing downstream knows which implementation it got.

```
ISatelliteShadingData   geometry + per-triangle areas/normals/centroids/model matrices
        │                 └── StaticMeshSatellite → RotatableMeshSatellite
        ├──────────────► IShadingPipeline ──► IShadingAlgorithm (BinaryShader | CoPShader)
        │                 └── ShadingPipeline (owns the GLFW/OpenGL context)
        └──────────────► IAeroLoadCalculator ◄── IGSIModel (Sentman)
                          └── HybridForceTorqueCalculator
```

`HybridForceTorqueCalculator::calc_aero_torque_force` is the whole story in one loop
([hybrid_aero_load_calculator.cpp](aero_sat/aero_load_calculator/hybrid_aero_load_calculator.cpp)):
shade once for the flow direction, then for each triangle multiply the GSI model's
per-element force/torque by that triangle's visibility factor and accumulate. Back-facing
triangles (`dot(normal, v_rel) <= 0`) bypass the shading result and get visibility 1.0 — the
GSI model itself is responsible for returning ~zero for them.

`ISatelliteManipulator` is a separate interface for articulating parts (solar arrays):
`turn_surface_around_axis` writes a per-mesh model matrix; it does **not** touch vertices.

### Geometry data layout (easy to get wrong)

`StaticMeshSatellite` loads via assimp (`aiProcess_Triangulate | aiProcess_JoinIdenticalVertices`)
and then **de-indexes**: `m_vertices` holds 9 floats per triangle (3 explicit vertices), and
`get_triangle_ids()` is *not* an index buffer — it is a per-vertex `uint32` attribute, the same
value repeated 3×, used as a render-target color. IDs are **1-based**, so `0` in the ID
framebuffer means background. Normals, centroids (3 floats each) and areas (1 float) are
per-triangle, computed at load time from the raw vertices.

`RotatableMeshSatellite` overrides the getters to apply the model matrices. The transformed
arrays and the bounding radius are computed together and cached; `turn_surface_around_axis()`
marks them outdated. Positions go through `transform_positions()` (w = 1) and normals through
`transform_directions()` (linear block only) — mixing the two is what bug row 1 was.
`get_model_space_vertices()` returns the untransformed geometry for consumers that apply the
model matrices themselves, which is what the shading pipeline uploads.

### Shading algorithms

Both live under [aero_sat/shading_pipeline/](aero_sat/shading_pipeline/), share the
[opengl/](aero_sat/shading_pipeline/opengl/) wrappers (VAO/VBO/IBO/FBO/Shader/ComputeShader/VisibilityReducer),
and embed their GLSL as `inline constexpr const char*` raw string literals in
`shaders/*_shader.h` — there are no runtime shader files to ship.

- **Binary**: renders triangle IDs under an orthographic projection along the flow direction;
  a triangle is visible (1.0) if any pixel carries its ID.
- **CoP**: renders triangle depth, then re-renders each triangle's *centroid* as a point with a
  small depth bias; visible if that point survives. Less area-quantization bias than Binary at
  the same resolution.

Both return 0.0/1.0 per triangle today, though the interface allows fractional values.
`num_pixel` is the square render resolution — the accuracy/speed knob.

After rendering, both shaders hand the ID texture to `VisibilityReducer`
([opengl/visibility_reducer.h](aero_sat/shading_pipeline/opengl/visibility_reducer.h)), a compute
shader that marks each ID it finds in an SSBO of one flag per triangle, so only `N+1` uints are
read back instead of the whole `num_pixel²` image.

One driver-portability decision is load-bearing there: the ID image is read with `texelFetch` on
a `usampler2D`, **not** `imageLoad` on a `uimage2D`, because `imageLoad` from an integer image
returns zeros on some AMD and Intel drivers. Keep that. (Verified working on Intel Iris Xe.)

### CMake target layout

One target per `aero_sat/` subdirectory, each with an `AeroSat::` alias and its own
`target_include_directories` — so headers are included flat (`#include "sentman.h"`), never by
relative path. `core` is INTERFACE-only. `shading_pipeline`'s subdirectories (`opengl/`,
`binary_shader/`, `cop_shader/`) define no targets; they `target_sources(...)` into the parent.

## MATLAB bindings

Single `MexGateway` MEX entry point with string-dispatched commands —
`MexGateway("Class.method", ...)` in [matlab/mex_gateway.cpp](matlab/mex_gateway.cpp). C++
objects live in per-class `unordered_map<int, unique_ptr<T>>` members with monotonically
increasing integer handles; each `.m` `handle` class stores its `handle_` (int32, `-1` when
unconstructed) and calls `"Class.delete"` from its destructor. All arguments are type- and
size-checked by the `validate_*` helpers before use, and exceptions are logged then rethrown to
MATLAB.

Adding a MATLAB-visible method means editing three places: the `if (cls == ...)` block in
`mex_gateway.cpp`, the wrapper `.m` classdef, and (usually) its `arguments` block.

Logging routes spdlog through a custom sink that `feval`s `fprintf` in the MATLAB engine
([custom_spdlog_sink.h](matlab/custom_spdlog_sink.h), [matlab_logger.h](matlab/matlab_logger.h)).
It defaults to DEBUG and is changed at runtime with `setLogLevel("debug"|"info"|"warn"|"error")`.
Per-call logging is slow — turn it down when benchmarking. (The README's claim that
`MEX_GATEWAY_LOG_LEVEL` controls this is stale; no such env var is read.)

MATLAB usage:

```matlab
addpath('<repo_root>\matlab'); addpath('<repo_root>\matlab\bin')
sat  = RotatableMeshSatellite('geometries/shuttlecock_15360.obj');
pipe = ShadingPipeline(sat, 1, 800);   % 0 = Binary, 1 = CoP; 800 = num_pixel
calc = HybridAeroLoadCalculator(sat, pipe, Sentman(1));
[F, T] = calc.calc_aero_load([7800 0 0], 300.0, ...
    AeroConditions(1.2482e-11, 934.0, 16*1.6605390689252e-27, 0.9));
```

Scripts and `.obj` geometries for experiments live in [matlab/examples/](matlab/examples/).

## Conventions

Naming is PEP-8-inspired: `PascalCase` classes, `I`-prefixed interfaces, `snake_case`
functions/variables/files/folders, `UPPER_SNAKE_CASE` constants, `m_` member prefix in C++.

Units are encoded in identifiers: `__` separates name from unit (`angle__rad`), `_per_` for
fractions (`__m_per_s`), exponent directly after the symbol (`__m2`).

`v_rel_B__m_per_s` is the relative flow vector **in the satellite body frame**
(see [velocity_definition.png](velocity_definition.png)). The operative sign convention is
fixed by Sentman's `erfc(-s * cos_delta)` with `cos_delta = dot(v_rel_hat, n)`: **a triangle is
windward (loaded) when `dot(n, v_rel) > 0`**, and leeward triangles fall off to ~zero force.
The shading camera (placed at `+v_rel_hat * bounding_sphere_radius`, looking at the origin) and
the calculator's back-face branch both follow that same convention, so the pipeline is
self-consistent — but the README's phrase "incoming molecular stream velocity" reads as the
opposite sign. See row 12 of the work queue.

Every `.cpp` that includes spdlog does `#define FMT_UNICODE 0` first (avoids MSVC's "Unicode
support requires compiling with /utf-8"). `SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG` is set
project-wide, so `SPDLOG_DEBUG` stays live even in Release.

Internal C++ math is `float`/`glm` throughout; the MATLAB layer passes `double` and converts at
the boundary.

Licensed GPL-3.0.

## Work queue (ordered by severity)

Verified 2026-08-31. P0 items produce **silently wrong numbers** — no crash, no warning.
Details for the P2–P4 rows are in [Known stale leftovers](#known-stale-leftovers).

> Rows 1, 2, 3, 7, 8 and 10 are **done** on the local branch `task/perf-and-correctness`
> (not merged to `main`, not pushed), along with a GLFW context-lifetime bug found while
> fixing them. Row 2's cache also lands row 9. See [Performance](#performance) for the
> measured result. The rows are kept below until that branch is merged.

| # | Sev | Area | Item | Where | Fix |
|---|-----|------|------|-------|-----|
| 1 | **P0** | Bug | `apply_transform` transforms **normals as points** (`vec4(n, 1.0f)`), so the model matrix's translation column is added to every normal. That column is `origin - R*origin`, which is non-zero whenever the hinge origin is off its own rotation axis → normals lose unit length *and* direction. Sentman divides only by `\|v_rel\|`, so a non-unit normal feeds a wrong `cos_delta` straight into `erfc`/`exp`. Live in `soar_rotatable.m`: 45°, hinge at x=−0.15 ⇒ ‖n‖ ∈ [0.885, 1.115], ±11.5%. | [rotatable_mesh_satellite.cpp:65](aero_sat/satellite/rotatable_mesh_satellite.cpp#L65) | Use `glm::mat3(transform) * n` for normals (renormalize), keep `w=1` for vertices/centroids. Split the shared helper. |
| 2 | **P0** | Bug | **Rotation applied twice.** `ShadingPipeline`'s ctor uploads `satellite.get_vertices()`, which for `RotatableMeshSatellite` is *already* model-transformed; `shade_satellite` then multiplies by `model_matrices[i]` again. Only wrong if a surface is turned *before* the pipeline is built — which is exactly what `soar_rotatable.m:32-36` does. Shading geometry then disagrees with the GSI geometry. | [shading_pipeline.cpp:17](aero_sat/shading_pipeline/shading_pipeline.cpp#L17) | Upload untransformed vertices (add a raw-vertex accessor), since the GPU path applies the matrices itself. |
| 3 | **P0** | Bug | Returned spans **dangle**. `get_vertices()`/`get_normals()`/`get_centroids()` each move-assign their member vector, freeing the buffer any previously returned span points into. Two calls in one expression ⇒ UB. | [rotatable_mesh_satellite.cpp:12-25](aero_sat/satellite/rotatable_mesh_satellite.cpp#L12-L25) | Transform in place into a pre-sized member, or return `const std::vector&`. Document that the previous span dies. |
| 4 | ~~P1~~ | Tests | ~~`shading_pipeline_test` does not compile.~~ **Stale — all four test targets build.** Verified 2026-09-08. | — | Done. |
| 5 | ~~P1~~ | Tests | ~~`geometries/tetraeder.obj` does not exist.~~ **Stale — the fixtures use `tetraeder_vector.h`.** Verified 2026-09-08. | — | Done. |
| 6 | ~~P1~~ | Build | ~~No preset sets `BUILD_TESTING`.~~ **Done** — ON in the `base` preset, plus `noTestsAction=error` so empty discovery fails. 38 tests run, 35 pass. | [CMakePresets.json](CMakePresets.json) | Done. |
| 7 | **P2** | Leak | `CoPShader` allocates the histogram SSBO and compiles a compute shader that is **never dispatched**, and never calls `glDeleteBuffers` → GPU buffer leak per pipeline instance. | [cop_shader.cpp:50-59](aero_sat/shading_pipeline/cop_shader/cop_shader.cpp#L50-L59) | Delete the dead histogram path in both shaders. |
| 8 | **P2** | Dead code | `BinaryShader::m_compute_shader` / `m_histogramBuffer` never assigned; `compute_shader.h` + `color_shader.h` compiled into the target but included by nothing. | see stale list | Remove; drop from `target_sources`. |
| 9 | **P2** | Perf | One force evaluation re-transforms the whole mesh 3–4× (`get_normals`, `get_centroids`, `get_bounding_sphere_radius`, each a full copy + pass). | [hybrid_aero_load_calculator.cpp:40-42](aero_sat/aero_load_calculator/hybrid_aero_load_calculator.cpp#L40-L42) | Cache per model-matrix generation; invalidate on `turn_*`. |
| 10 | **P3** | Docs | Misleading comment: the `GL_SHADER_IMAGE_ACCESS_BARRIER_BIT` block argues for a barrier the code doesn't issue, guarding a compute shader that doesn't run. | [binary_shader.cpp:123-130](aero_sat/shading_pipeline/binary_shader/binary_shader.cpp#L123-L130) | Keep only the `glReadPixels`-over-`imageLoad` rationale. |
| 11 | **P3** | Docs | README's entire vcpkg section is unrunnable (`x64-debug` preset, vcpkg toolchain, `.slnx` — none exist), plus `MEX_GATEWAY_LOG_LEVEL`, "Schuette", `.urdf`, Sadycos, and the "tests aren't wired" claim. | [README.md](README.md) | Rewrite or delete. |
| 12 | **P3** | Docs | Velocity sign wording. Code is unambiguous and self-consistent (`dot(n, v_rel) > 0` = windward), but "incoming molecular stream velocity" reads as the opposite sign. **Owner call:** confirm intent, then fix the prose (not the code). | README + [velocity_definition.png](velocity_definition.png) | One clarifying sentence. |
| 13 | **P3** | Docs | `UML_design.mdj` (894 KB) + `PackageDiagram.png` predate the current architecture; `.github/copilot-instructions.md` is one orphaned clib line. | repo root | Delete or mark historical. |
| 14 | **P4** | Hygiene | Three incompatible generations of the MATLAB API live in `matlab/examples/`; only the bare-class one works. | see stale list | Delete gens 2 & 3, or move to `examples/legacy/`. |
| 15 | **P4** | Hygiene | `test_sentman.m` loads a missing `.obj`, so the README's MATLAB quick-start fails; it is a benchmark, not a test. | [matlab/test_sentman.m:26](matlab/test_sentman.m#L26) | Point at a committed mesh; rename. |
| 16 | **P4** | Hygiene | `Assembled_ISS.stl` committed 3× byte-identical and unreferenced; `GetTestDataPath` in 3 copies with 2 signatures; `Vat.prj` + `resources/project/` still named "Vat". | test/, matlab/ | De-duplicate; rename to AeroSat. |
| 17 | **P5** | API | `AeroConditions` passed as **non-const** `&` through every interface though nothing mutates it. | [core.h](aero_sat/core/core.h) | Make it `const&`. |
| 18 | **P5** | API | `ISatelliteManipulator::turn_surface` / `turn_surfaces` return `-1` in every implementation; `//TODO meshid statt surface id` still open. | [Isatellite_manipulator.h](aero_sat/core/Isatellite_manipulator.h) | Drop from the interface, or implement. |
| 19 | **P5** | Robustness | A zero-length rotation axis makes `glm::rotate` produce an all-NaN matrix. (The `//TODO: consider normalizing axis` above it is obsolete — `glm::rotate` already normalizes.) | [rotatable_mesh_satellite.cpp:49](aero_sat/satellite/rotatable_mesh_satellite.cpp#L49) | Reject a near-zero axis; delete the stale TODO. |
| 20 | **P5** | Hygiene | `MAX_TRIANGLES` justified by a histogram buffer that no longer exists; commented-out `#*.obj` in `.gitignore` needs a why-comment. **The `CMP0141` claim that used to sit here was wrong and has been removed** — that block is load-bearing under Ninja and is the only source of `-ZI`; only its German comment needs changing. | see stale list | Cosmetic. |

### On the manipulator, specifically

`turn_surface_around_axis` itself is fine: it builds `T(+o) · R · T(−o)`, `glm::rotate`
normalizes its axis internally, and the resulting 3×3 block stays orthonormal — so **the
rotation never skews or scales the geometry**. Model matrices start as identity, one per mesh,
and `turn_surface_around_axis` overwrites rather than accumulates (each call is absolute, not
relative — worth knowing).

The damage is entirely downstream, in how the matrix is *applied* (rows 1–3):

- Normals go through the same `w=1` path as vertices, so they pick up the translation column.
  This is invisible whenever the hinge origin lies **on** the rotation axis — the usual
  solar-panel case, `origin=[0,1.5,0]` with `axis=[0,1,0]` — because then `R·o = o` and the
  column is exactly zero. It only bites for an off-axis origin, which is why it has survived.
- Because the transform is a pure rotation, `glm::mat3(transform)` is already the correct
  normal matrix; `transpose(inverse(...))` is not needed *unless* scaling is ever introduced.
  Worth a comment at the fix site so the next person doesn't add scale and silently break it.
- Unit length is preserved by the rotation itself, so after fixing row 1 a renormalize is
  belt-and-braces — but cheap, and it protects against accumulated float drift.

## Known stale leftovers

Things that had a purpose once and no longer do. Verified as of 2026-08-31 — **re-check before
acting on any of these**, and delete the entry once it is resolved. Do not treat any of them as
a description of how the system currently works, and do not copy their patterns into new code.

### Documentation that describes a system that no longer exists

- **The whole vcpkg section of [README.md](README.md#L97-L114).** It tells you to run
  `cmake --preset x64-debug` — no such preset exists; [CMakePresets.json](CMakePresets.json)
  defines only `pixi-*`. It claims "the presets set the [vcpkg] toolchain for you" — none of
  them do. It mentions an `AeroSat.slnx` from the Visual Studio generator — every preset uses
  Ninja. [vcpkg.json](vcpkg.json) still lists the dependency set, and
  [vcpkg-configuration.json](vcpkg-configuration.json) still points at the long-deprecated
  `vcpkg-ce-catalog` artifact registry. Treat pixi as the only working dependency path.
- **`MEX_GATEWAY_LOG_LEVEL`** (README, MATLAB bindings section). No such env var is read
  anywhere. Log level defaults to DEBUG in the `MexFunction` constructor and is changed with
  `setLogLevel(...)`.
- **"Schuette scaffolding"** (README architecture section). [aero_sat/gsi/](aero_sat/gsi/)
  contains only Sentman. The name survives from the old UML model (below).
- **"`.urdf` workflows" and "Sadycos coupling"** (README use cases). Aspirational; neither
  appears anywhere in the code. Geometry loading is assimp-only.
- **README: "tests aren't wired into the CMake build yet."** They *are* wired — the real
  problem is `BUILD_TESTING` plus the rot listed below.
- **[UML_design.mdj](UML_design.mdj) (894 KB) and [PackageDiagram.png](PackageDiagram.png).**
  Predate the current architecture entirely. Their classes are `Snake_Case`
  (`Binary_Shader`, `Force_Torque_Calculator`, `Shading_Context`, `Projected_Area_Calculator`,
  `Relative_Shader`) with interfaces `IGeometry`, `IShader`, `IKinematics`,
  `IProjected_Area_Calculator`, `IVisualisation` — none of which exist. Of today's class names
  only `ISatelliteManipulator` appears. Do not use these as a design reference.
- **[.github/copilot-instructions.md](.github/copilot-instructions.md).** A single orphaned line
  about "MATLAB clib blockers", left over from an abandoned `clibgen`-based binding attempt
  (see `bugtesting_temp.m` below). Irrelevant to the current hand-written MEX gateway.

### Dead code in the shading pipeline

All of this is residue from an earlier design where a GPU compute shader built a per-triangle
pixel **histogram** (which would have given fractional visibility). That was replaced by a CPU
`glReadPixels` + "any pixel seen" pass, and the scaffolding was never removed. **No
`glDispatchCompute` call exists anywhere in the repo.**

- `BinaryShader::m_compute_shader` and `m_histogramBuffer` — declared in
  [binary_shader.h](aero_sat/shading_pipeline/binary_shader/binary_shader.h#L18-L24), never
  assigned or read in the `.cpp`.
- `CoPShader` — *worse*: it actually allocates the SSBO and compiles the compute shader in
  `set_vertices` ([cop_shader.cpp:50-59](aero_sat/shading_pipeline/cop_shader/cop_shader.cpp#L50-L59)),
  then never dispatches it. The buffer is also never `glDeleteBuffers`'d in the destructor, so
  it leaks per pipeline instance.
- [binary_shader/shaders/compute_shader.h](aero_sat/shading_pipeline/binary_shader/shaders/compute_shader.h)
  and [color_shader.h](aero_sat/shading_pipeline/binary_shader/shaders/color_shader.h) — listed
  in `target_sources` but included by nothing. `color_shader.h` unpacks RGB from a packed uint,
  from before triangle IDs moved to an `R32UI` integer target; its comments are in German.
- The `MAX_TRIANGLES = (2u << 28) - 1` guard in both shaders, commented "limit histogrambuffer
  size to about 1GB" — the histogram buffer it sizes is gone, so the bound is now arbitrary.
- The barrier comment block in
  [binary_shader.cpp:123-130](aero_sat/shading_pipeline/binary_shader/binary_shader.cpp#L123-L130).
  It argues at length that `GL_SHADER_IMAGE_ACCESS_BARRIER_BIT` is required for compute-shader
  `imageLoad` visibility — the code immediately below issues only
  `glMemoryBarrier(GL_FRAMEBUFFER_BARRIER_BIT)`, and there is no compute shader to be visible
  to. Only the `glReadPixels`-over-`imageLoad` half of the comment still applies.

### Rotted tests

The two build-level entries that used to head this list — the missing `tetraeder.obj` and the
uncompilable pipeline tests — were **already fixed** and have been removed. Verified 2026-09-08:
all four targets build and `pixi run test` runs 38 tests.

Three of those 38 fail, and they are genuine, not build rot:

- **`SentmanTest.CalcForceAndTorque_TemperatureRatioMethod2` and `...3`.** `torque.z` comes out
  as `+0.00053698` against an expected `-0.00053700` — equal in magnitude, opposite in sign. That
  is a sign convention disagreement, not arithmetic drift, and it points straight at row 12.
  **Owner call:** decide whether the code or the expectation is authoritative before touching
  either; adjusting the expectation to match would destroy the signal.
- **`CoPShaderTest.ShadeTetrahedron`.** Triangle 3 reads visible along `+z` where the fixture
  expects it hidden.

Still rotted:

- **`Assembled_ISS.stl` committed three times** (byte-identical, in `test/satellite/`,
  `test/shading_pipeline/`, `test/aero_load_calculators/`) and referenced by nothing.
- **`GetTestDataPath` exists in three copies with two different signatures** — a one-arg
  `__FILE__`-capturing version duplicated in two `.cpp`s, and a two-arg version in
  [test_helpers.h](test/satellite/test_helpers.h) that only `test/satellite/` includes.

### Three generations of MATLAB API in one folder

Only generation 1 works against the current `MexGateway`. Generations 2 and 3 are untracked
local scratch files; **do not copy their call style.**

1. **Current** — bare classes `RotatableMeshSatellite`, `ShadingPipeline`, `Sentman`,
   `AeroConditions`, `HybridAeroLoadCalculator`. Used by `shuttlecock.m`, `soar_rotatable.m`,
   `test_sentman.m`.
2. **`vat.*` package** — `vat.RotatableMeshGeometry`, `vat.ShadingPipeline`, `vat.show_shading`.
   Used by `compare_iss.m`, `compare_shuttlecock.m`, `shuttlecock_torque_volume.m`. No `+vat`
   package directory exists. Matches the `IGeometry` naming in the old UML model.
3. **`AeroSat.*` clib-style** — `AeroSat.gsi.Sentman(1, 0.9)`, `AeroSat.gsi.Maxwell`,
   `AeroSat.gsi.Newton`, `AeroSat.core.AeroConditions`, `get_alpha_e()` getters, in
   `bugtesting_temp.m`. Nothing of this exists: no `Maxwell`, no `Newton`, no GSI getters, no
   two-argument `Sentman`. Residue of the abandoned `clibgen` binding attempt.

Also in this area:

- **[matlab/test_sentman.m:26](matlab/test_sentman.m#L26)** loads
  `'International Space Station.obj'`, which is not in the repo — so the README's MATLAB
  quick-start (`addpath ...; test_sentman`) fails partway through. It is a benchmark script,
  not a test, despite the name.
- **[matlab/Vat.prj](matlab/Vat.prj) and [matlab/resources/project/](matlab/resources/project/)**
  — a MATLAB project still named "Vat" (~40 opaque XML files); `Vat.prj` itself is an empty stub
  element. The toolbox is now AeroSat.
- **`compare_shuttlecock.m` loads `iss_26.obj`** and its comment says "compare shading times for
  ISS model" — a copy of `compare_iss.m` that was never adapted.
- Untracked MATLAB autosaves (`*.asv`) sit next to the scripts; `.gitignore` covers them.

### Smaller items

- `ISatelliteManipulator::turn_surface(id, angle)` and `turn_surfaces()` — every implementation
  returns `-1` ("Not implemented for static mesh"), and `RotatableMeshSatellite` overrides
  neither. Only `turn_surface_around_axis` is real, and it carries a
  `//TODO meshid statt surface id`.
- `AeroConditions` is a plain POD in [core.h](aero_sat/core/core.h) but is passed as a
  **non-const reference** through every interface (`IGSIModel`, `IAeroLoadCalculator`) even
  though nothing mutates it.
- The German comment and `CMP0141` / `EditAndContinue` block at the top of
  [CMakeLists.txt](CMakeLists.txt#L3-L7) targets MSVC debug-info formats for the Visual Studio
  generator; every preset uses Ninja with `RelWithDebInfo`.
- `.gitignore` has a commented-out `#*.obj` — `.obj` is both an MSVC object file and the mesh
  format used throughout, which is presumably why it was disabled. Worth a comment saying so.
- An empty untracked `slprj/` (Simulink codegen cache) sits at the repo root — local only, not
  in git.
- Only 2 of the 8 `.obj` files in [matlab/examples/geometries/](matlab/examples/geometries/) are
  tracked; `shuttlecock_15k.obj` also exists as a *different* file in
  [examples/geometry_files/](examples/geometry_files/) (different checksum, same name).


## Performance

Measured on Intel Iris Xe, GL 4.3, `RelWithDebInfo`, `shuttlecock_61440.obj` (61 440 triangles),
with [examples/perf_probe](examples/perf_probe/) — `--timings` for these numbers, `--fingerprint`
for the before/after result diff that guarded the changes.

State on `main` — one `calc_aero_torque_force` at P = 4000, by phase:

| phase | ms | share |
|---|---|---|
| `glReadPixels`, 64 MB device→host | 21.0 | 30 % |
| readback buffer alloc + zero-fill, then overwritten | 11.9 | 17 % |
| `get_bounding_sphere_radius()`, full 9N re-transform | 14.0 | 20 % |
| CPU "which IDs appear" scan, 16 Mpx | 7.0 | 10 % |
| `get_normals()` + `get_centroids()` | 8.3 | 12 % |
| Sentman × 61 440 | 4.7 | 7 % |
| **draw + rasterise (actual GPU work)** | **1.8** | **3 %** |

The GPU did 3 % of the work; the rest was marshalling a 64 MB image to extract 61 440 booleans.

After the two changes on `task/perf-and-correctness` (geometry cache, GPU reduction), median of
3 runs:

| | `shade()` P=1000 | `shade()` P=4000 | full P=1000 | full P=4000 |
|---|---|---|---|---|
| before | 19.11 ms | 54.67 ms | 33.40 ms | 71.47 ms |
| after | 1.81 ms | 3.55 ms | 9.03 ms | 10.86 ms |
| | **10.6×** | **15.4×** | 3.7× | 6.6× |

`num_pixel` is now nearly free — P=4000 costs 1.9× P=1000, so accuracy has largely stopped being
a trade-off. What remains is dominated by the Sentman loop (~4.7 ms), which is the next
candidate (work-queue rows 4 and 5).

Two measurement traps worth knowing: `examples/geometry_files/shuttlecock_15k.obj` holds **60**
triangles, not 15 000, so it hides all N-dependent cost; and one stray run gave a 3× outlier on
an otherwise stable figure, so take a median of several runs.
