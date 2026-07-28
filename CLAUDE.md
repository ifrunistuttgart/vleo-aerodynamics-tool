# Build Instructions (VS Code on Windows)

## Prerequisites

- **CMake** ≥ 3.15 (tested with 4.1)
- **Ninja** build tool
- **Visual Studio 2022** (Community or higher) with C++ desktop workload
- **vcpkg** installed at `C:\vcpkg` (or any path, set `VCPKG_ROOT` accordingly)
- **MATLAB** (only required for MATLAB bindings)

## One-time Setup

### 1. Fix the `libaec` GitLab download (rate-limit workaround)

The vcpkg registry downloads `libaec` from `gitlab.dkrz.de`, which frequently returns HTTP 429.
A GitHub-mirror overlay port is already committed at `overlay-ports/libaec/`, and the
`vcpkg-configuration.json` has `"overlay-ports": ["./overlay-ports"]` to activate it automatically.
No manual action required — this is already in the repo.

### 2. Set the VCPKG_ROOT environment variable

Add to your user or system environment:
```
VCPKG_ROOT=C:\vcpkg
```

Or set it per-session in a Developer PowerShell / cmd before opening VS Code.

## Configure & Build (VS Code CMake Tools)

Use the **CMake Tools** VS Code extension (already configured via `CMakePresets.json`):

1. Open the command palette → **CMake: Select Configure Preset** → choose `x64-debug` (or `x64-release`)
2. **CMake: Configure** — this runs `cmake --preset x64-debug` and triggers vcpkg to install all
   dependencies (first run takes ~60 min; subsequent runs use the binary cache and are fast)
3. **CMake: Build** — builds all targets including `main.exe`

> **Important:** Do **not** use a plain bash/Git Bash terminal for configure/build.
> `cl.exe` (MSVC) is only in PATH inside a Developer Command Prompt or when VS Code's
> CMake Tools manages the environment. The CMake Tools extension handles this automatically.

## Build MATLAB Bindings (`MexGateway.mexw64`)

MATLAB bindings are off by default. They are enabled via `BUILD_MATLAB_BINDINGS=ON` in
`CMakePresets.json` (already set in `windows-base`).

1. Ensure MATLAB R2024a or later is installed
2. Reconfigure if needed (**CMake: Configure**)
3. **CMake: Build Target** → select `MexGateway`
4. Output: `matlab/bin/MexGateway.mexw64` + all required DLLs copied alongside it

## Using the MATLAB Interface

In MATLAB, add both folders to the path before running any script:

```matlab
addpath('<repo_root>\matlab')
addpath('<repo_root>\matlab\bin')
```

Then run e.g.:
```matlab
test_sentman
```

## Notes

- The `overlay-ports/libaec/` directory redirects the `libaec` download from GitLab to the
  GitHub mirror (`MathisRosenhauer/libaec`) with the correct SHA512 for the GitHub tarball.
  The `vcpkg-configuration.json` registers this overlay automatically.
- Private `std::unique_ptr` members in implementation classes are not MATLAB clib blockers
  (they are not exposed in the public API).

> **The instructions above describe the current (vcpkg-based) build.** They are being
> replaced by the migration plan below. Do not delete this section until the migration
> in "Build System Migration Plan" is complete and this file has been rewritten to match.

---

# Build System Migration Plan: vcpkg → pixi + conda-forge

**Status:** Planned, not yet started. **Branch:** work should happen off a dedicated branch
(not `bugfix-gpu-panel-visibility`), e.g. `build-system-migration`.

## Why this migration

The current vcpkg-based build is slow (VTK builds from source, taking on the order of
hours) and fragile (a dependency chain reaches out to `gitlab.dkrz.de` for `libaec`, which
frequently 429s, requiring the `overlay-ports/libaec` GitHub-mirror workaround). The user's
goals for the new build system:

- Dependencies install with one simple command.
- The build works on Windows, Linux, and macOS (needed now, not just "someday").
- No juggling multiple build tools/procedures.
- No hacky workarounds like the current overlay port — clean and maintainable.
- Drastically shorter build times (no building VTK from source).

## Root cause of the current libaec/VTK slowness (researched, confirmed)

Traced directly from `out/build/x64-debug/vcpkg-manifest-install.log` and the vcpkg port
manifests at the pinned baseline commit (`vcpkg-configuration.json`'s
`256acc64012b23a13041d8705805e1f23b43a024`):

- `vcpkg.json` depends on plain `"vtk"` with no feature selection, so vcpkg builds VTK's
  **default feature set**: `cgns`, `netcdf`, `seacas`, `proj`, `sql`, `libharu`, `libtheora`.
- That default set pulls in `netcdf-c` → `hdf5` (built with its own default `szip` feature) →
  `libaec` — confirmed present in the actual install plan:
  `hdf5[core,zlib,szip,hl]`, `libaec:x64-windows@1.1.6`, `cgns[core,lfs,hdf5]`,
  `netcdf-c[core,netcdf-4,nczarr,dap]`, `seacas`.
- **None of this is used by the codebase.** [aero_sat/visualization/show_mesh.cpp](aero_sat/visualization/show_mesh.cpp)
  and [aero_sat/visualization/CMakeLists.txt](aero_sat/visualization/CMakeLists.txt) only use VTK's
  `CommonCore`, `CommonDataModel`, `FiltersSources`, `FiltersModeling`, `RenderingCore`,
  `RenderingOpenGL2`, `InteractionStyle`, `RenderingAnnotation`, `InteractionWidgets`,
  `RenderingFreeType` — plain mesh rendering, no I/O/scientific-data formats at all.
- In other words: the GitLab/libaec flakiness and most of the VTK build time are self-inflicted,
  caused by VTK's default feature set pulling in unused heavyweight dependencies. This is a
  known long-standing pain point — commit `b54de61` disabled visualization outright at one
  point specifically because "vtk takes hours to build with vcpkg."

## Decision: replace vcpkg with pixi + conda-forge

Confirmed via research (see chat history / links below) before deciding:

- **VTK is available prebuilt on conda-forge** for `win-64`, `linux-64`, `linux-aarch64`,
  `osx-64`, and `osx-arm64` (checked at prefix.dev/channels/conda-forge/packages/vtk,
  version 9.6.2 at time of research). Using the prebuilt binary means no VTK build at all,
  and its feature set is fixed by the conda-forge recipe rather than something we
  accidentally over-select — this removes the libaec problem at the root rather than working
  around it.
- The other current dependencies (`assimp`, `glew`, `glfw3`/`glfw`, `glm`, `gtest`, `spdlog`)
  are all long-standing, common conda-forge packages. **Exact package/config names still
  need to be confirmed during implementation** (e.g. `glfw` vs `glfw3`, exact CMake config
  target names) — not verified individually yet, only VTK was directly checked.
- **Docker was considered and rejected**: it virtualizes the whole OS, which buys nothing
  here — it can't show a native GUI window for the OpenGL viewer well, and it can't reach a
  MATLAB install living on the host. Pixi already gives reproducible, native, per-OS
  environments without virtualization, which is what was actually wanted.
- **MATLAB stays a manual host-installed prerequisite** (it's commercial, not distributable
  via conda-forge/pixi) — same as it already is today. This is fine because
  [matlab/CMakeLists.txt](matlab/CMakeLists.txt) builds `MexGateway` via CMake's own
  `matlab_add_mex()`, i.e. a normal shared-library CMake target linking against MATLAB's
  `MX_LIBRARY`/`MEX_LIBRARY`/`ENG_LIBRARY` — it does **not** shell out to MATLAB's own `mex`
  CLI/toolchain, so it builds fine with whatever compiler the pixi-managed CMake toolchain
  uses.
- **Caveat on Windows:** conda-forge cannot legally redistribute `cl.exe`. Its Windows
  "compilers" package only *locates/activates* a system-installed Visual Studio Build Tools
  2022 — it doesn't vendor the compiler itself. So **VS Build Tools 2022 remains a one-time
  Windows host prerequisite** regardless of this migration; pixi replaces vcpkg, not Visual
  Studio. Linux/macOS conda-forge compiler packages are self-contained (no separate system
  compiler needed there).

## Migration plan

1. **Add `pixi.toml` at repo root.**
   - Declare platforms: `win-64`, `linux-64`, `osx-64`, `osx-arm64` (add `linux-aarch64` if
     needed later).
   - Dependencies: `cmake`, `ninja`, `vtk`, `assimp`, `glew`, `glfw` (confirm exact name),
     `glm`, `gtest`, `spdlog`, plus a C++ compiler (`cxx-compiler` on Linux/macOS; on Windows,
     confirm whether the conda-forge `vs2022_win-64` activation package is needed or whether
     relying on a pre-activated Developer environment is simpler).
   - Define `pixi run configure` / `pixi run build` tasks wrapping the CMake/Ninja invocation
     (replacing the manual "CMake: Configure"/"CMake: Build" VS Code steps, though the VS Code
     CMake Tools extension can still be used against the pixi-provided environment).

2. **Update CMake wiring.**
   - Remove `CMAKE_TOOLCHAIN_FILE` (vcpkg toolchain) from `CMakePresets.json`.
   - Point `CMAKE_PREFIX_PATH` at the pixi environment's prefix so `find_package(VTK)`,
     `find_package(glm CONFIG)`, `find_package(assimp)`, `find_package(GTest)`, etc. resolve
     against the conda-forge-installed CMake config files. **Confirm the exact env var pixi
     exposes for this** (likely `CONDA_PREFIX`-compatible, needs verification — do not assume).
   - Delete the `x86-*` presets if 32-bit is not actually needed (not currently confirmed
     either way — ask before removing).
   - Update `vcpkg.json` / `vcpkg-configuration.json` for removal once the pixi environment is
     proven to build everything successfully — don't delete until the new path is verified
     end-to-end, so there's a fallback during the transition.

3. **Delete the vcpkg workaround** once the pixi build is verified working:
   - `overlay-ports/` directory
   - `vcpkg.json`, `vcpkg-configuration.json`
   - The `VCPKG_ROOT` setup step in these build docs

4. **Verify VTK component availability.** Confirm the conda-forge VTK build actually exposes
   the specific components used (`RenderingOpenGL2`, `InteractionWidgets`, `RenderingAnnotation`,
   `RenderingFreeType`, etc.) via its CMake config — conda-forge ships one fixed VTK build, so
   if a component is missing there's no "just enable the feature" fallback like vcpkg had.
   This is the main technical risk of the migration and should be checked early, before doing
   any other steps.

5. **MATLAB bindings.**
   - Keep `matlab/CMakeLists.txt` as-is (`find_package(Matlab REQUIRED ...)`,
     `matlab_add_mex(...)`) — no changes expected here.
   - Confirm `matlab_add_mex` on Windows still works when CMake's compiler is resolved via a
     pixi-activated environment rather than VS Code's CMake Tools-managed Developer
     environment (this is about *which cl.exe* CMake picks up, not about MATLAB itself).

6. **Cross-platform validation** (explicitly in scope now, not deferred):
   - Get a clean `pixi install && pixi run configure && pixi run build` working on Windows
     first (lowest-risk, matches current dev environment), then Linux, then macOS.
   - MATLAB bindings only need to be validated on whichever OS(es) actually run MATLAB in
     practice — confirm this scope with the user before spending time on it for all three.

7. **Update documentation.** Once the migration is verified end-to-end, rewrite the
   "Build Instructions" section of this file to describe the pixi-based flow, and delete this
   "Migration Plan" section (or mark it done) so `CLAUDE.md` reflects only the current,
   working process — don't leave two conflicting sets of build instructions live long-term.

## Progress so far (branch `migrate-build-to-pixi`)

- `pixi.toml`/`pixi.lock` added at repo root with `cmake`, `ninja`, `c-compiler`,
  `cxx-compiler`, `vtk`, `assimp`, `glew`, `glfw`, `glm`, `gtest`, `spdlog` for
  `win-64`/`linux-64`/`osx-64`/`osx-arm64`. `pixi run build` (→ `configure` → `build`)
  produces a working `main.exe` on Windows, `CMAKE_PREFIX_PATH=$CONDA_PREFIX` resolves
  every dependency's CMake config with no `CMakeLists.txt` changes needed — package
  names, config filenames, and target names (`glfw`, `GLEW::GLEW`, `VTK::*`, etc.) all
  matched what the source already expected.
- Confirmed pixi does set `CONDA_PREFIX` to the environment path (e.g.
  `<project>/.pixi/envs/default`) — one of the previously-open items, now verified.
- Confirmed all 10 VTK components the project links against exist in the conda-forge
  9.6.2 build (`RenderingOpenGL2`, `InteractionWidgets`, `RenderingAnnotation`,
  `RenderingFreeType`, etc.) — the main technical risk from item 4 above is cleared.
- **New, important finding not in the original research:** conda-forge's Windows C++
  packages are Release-only (`/MD`, linked against `MSVCP140.dll`) — there is no Debug
  (`/MDd`) variant. Configuring with `CMAKE_BUILD_TYPE=Debug` on Windows mixes `/MDd`
  application code with `/MD` third-party DLLs (spdlog, assimp, VTK, …); any
  `std::string`/`std::vector` crossing that boundary corrupts memory and segfaults
  almost immediately (reproduced: crashed inside `StaticMeshSatellite`'s constructor
  before its first log line). **Use `RelWithDebInfo` instead of `Debug` on Windows** —
  it maps to `/MD` (CRT-compatible) while still keeping debug symbols. This didn't come
  up with vcpkg because vcpkg triplets build separate debug and release copies of every
  library. `pixi.toml`'s `configure` task now uses `RelWithDebInfo`.
- Running a built executable directly (double-click or bare `./main.exe`) fails with
  "X.dll was not found" — conda-forge's shared DLLs live in `.pixi/envs/default/Library/bin`,
  which is only on `PATH` inside a pixi-activated shell. Added a `pixi run run` task
  instead of solving this with a post-build DLL-copy step (deferred — revisit if VS
  Code's debugger needs it).
- Fixed three pre-existing source bugs surfaced (not caused) by the newer VTK/compiler:
  missing `#include <vector>` in `Ishading_pipeline.h` and `Ishading_algorithm.h`, and a
  hardcoded `vtk-9.3/` include prefix in `show_mesh.cpp` (VTK's CMake targets already put
  the versioned include dir on the path, so the version-agnostic `#include <vtkActor.h>`
  form is correct going forward regardless of VTK version).
- Fixed `main.cpp` pointing at `main/International Space Station.obj`, which was never
  actually committed to git (0 history) — repointed at `matlab/soar_satellite.obj`, the
  only mesh asset that actually exists in the repo.
- Noted but not yet resolved: conda-forge's VTK build is Python-version-locked
  (`vtk-9.6.2-py314h...`) and pulls in a full Python + numpy/matplotlib-style dependency
  chain (`aiohttp`, `contourpy`, `cycler`, etc.) that the project doesn't use. Still far
  faster than building VTK from source, but a real footprint/complexity trade-off that
  wasn't visible before actually running `pixi install`.
- Added `pixi-debug`/`pixi-release` presets to `CMakePresets.json` (`CMAKE_PREFIX_PATH`
  set to `$env{CONDA_PREFIX}`, `RelWithDebInfo`/`Release`), plus matching `buildPresets`
  and `testPresets` entries (`cmake --build --preset <name>` and `ctest --preset <name>`
  both require an explicit entry in those arrays — a bare `configurePresets` entry isn't
  enough). The old vcpkg-based `x64-debug`/`x64-release`/`x86-*` presets are untouched,
  kept as fallback. `pixi.toml`'s tasks now call these presets instead of duplicating
  raw `cmake` flags, so there's one source of truth.
- **Another environment gotcha found via presets specifically:** pixi's Windows
  `cxx-compiler` activation (`vs2022_win-64`) sets `CMAKE_GENERATOR=Visual Studio 17
  2022`, `CMAKE_GENERATOR_PLATFORM=x64`, `CMAKE_GENERATOR_TOOLSET=v143`, and
  `CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Release` as convenience defaults for people not using
  presets. Configuring through a preset that explicitly picks `Ninja` then fails with
  "Generator Ninja does not support platform specification, but platform x64 was
  specified" — the leftover `CMAKE_GENERATOR_PLATFORM` env var conflicts with Ninja
  (a single-platform generator). A raw `cmake -G Ninja ...` command line isn't affected,
  only the preset path is. **Setting `"environment": {"CMAKE_GENERATOR_PLATFORM": null}`
  inside the preset itself does NOT fix this** — confirmed by testing — CMake reads
  these env vars before applying preset-level environment overrides. The working fix:
  clear `CMAKE_GENERATOR`/`CMAKE_GENERATOR_PLATFORM`/`CMAKE_GENERATOR_TOOLSET`/
  `CMAKE_ARGS` at the shell/task level, before `cmake` even starts (done via `env = {...}`
  on the `configure` task in `pixi.toml`). Anyone driving these presets from a plain
  `pixi shell` (rather than the `pixi run` tasks) will hit this and need to `unset` those
  four variables manually first.
- **MATLAB bindings work with no changes to `matlab/CMakeLists.txt`.** Initially added
  `BUILD_MATLAB_BINDINGS: ON` to the shared `pixi-base` preset — wrong, caught before
  it shipped in the README: `find_package(Matlab)` is `REQUIRED`, so that would have
  made MATLAB a hard prerequisite for building the core toolbox at all, contradicting
  the project's own "MATLAB only required for MATLAB bindings" framing. Fixed: it's
  opt-in via dedicated `pixi-debug-matlab`/`pixi-release-matlab` presets (inheriting
  `pixi-debug`/`pixi-release`) and matching `configure-matlab`/`build-matlab` tasks in
  `pixi.toml`, building into their own `out/build/pixi-debug-matlab` directory so they
  don't collide with a plain core build. Verified both independently: the plain
  `pixi run build` produces 48 targets and never touches MATLAB; `pixi run build-matlab`
  additionally builds `MexGateway.mexw64` (`matlab_add_mex()` found MATLAB — R2026a/
  R2024b both installed — and built against the pixi-resolved compiler with only a
  benign narrowing-conversion warning).
  - Found and cleaned up stale leftover DLLs in `matlab/bin/` from an old vcpkg Debug
    build (`vtkCommonCore-9.3d.dll`, `spdlogd.dll`, `glfw3.dll`, etc.) — a real hazard,
    since Windows resolves a DLL's dependencies from its own folder before `PATH`, so a
    freshly-built `MexGateway.mexw64` sitting next to stale same-named DLLs
    (`glfw3.dll` in particular looked like a likely name collision) could have silently
    loaded the wrong, incompatible library.
  - `MexGateway.mexw64` needs ~15 conda-forge DLLs at runtime (spdlog, VTK, assimp,
    glew, glfw, …) that aren't copied next to it by default, and unlike `main.exe`,
    MATLAB can't reasonably be launched via `pixi run` for daily use. First decided to
    just document a `PATH` prerequisite instead of solving it in the build — reversed
    that: added `cmake/copy_runtime_deps.cmake`, a `file(GET_RUNTIME_DEPENDENCIES)`-based
    script wired as a `POST_BUILD` step on the `MexGateway` target (Windows +
    `CONDA_PREFIX`-only, guarded so it doesn't touch the vcpkg fallback, which already
    gets this via `VCPKG_APPLOCAL_DEPS`). It walks `MexGateway.mexw64`'s actual PE
    import table recursively (so it can't silently go stale the way a hardcoded DLL
    list would) and copies the resolved closure into `matlab/bin` — 65 DLLs, verified
    with a clean rebuild, zero unresolved dependencies (aside from MATLAB's own
    `libmx`/`libmex`/`libmat`/`libMatlab*` libraries, deliberately excluded since
    MATLAB always has its own install directory on its search path already). Net
    result: `matlab/bin` is fully self-contained, no `PATH` change needed for MATLAB
    at all. Not yet added to the user-facing MATLAB instructions — do that as part of
    step 7 (final doc rewrite).
- Cherry-picked 4 commits from a separate `matlab-script-examples` branch (parallel
  MATLAB-focused work) after reviewing each one: a real bugfix where
  `mex_gateway.cpp`'s shading-algorithm switch used `case 0/1` for Binary/CoP while
  `ShadingPipeline.m`'s own docstring already documented `1/2` — confirmed as a
  genuine pre-existing mismatch, not a style choice, by diffing both the `.m` and
  `.cpp` sides; a `MEX_GATEWAY_VERBOSE_LOGGING` compile-time toggle plus a matching
  `pixi-release-matlab` preset/tasks (see below for why this got simplified further);
  standard MathWorks `.gitattributes` (binary/merge-driver rules for
  `.mat`/`.mlx`/`.slx`/etc.); and `*.asv` (MATLAB autosave files) added to
  `.gitignore`. Left out of the cherry-pick: that branch's
  `matlab/examples/soar_rotatable.m` example-script expansion (sweep demo) — out of
  scope for "how the MATLAB binding is built," and worth the user's own call on
  whether/when to bring it over.
- **Simplified the logging toggle immediately after, on the user's prompt** ("why do
  we need debug-matlab and release-matlab as well as pixi — don't we just need one
  with logging and one without?"). The real answer: the two axes (build type,
  log verbosity) had been coupled 1:1 for no real reason, and `Release` vs
  `RelWithDebInfo` makes negligible practical difference here (both use `/MD`, similar
  optimization; `RelWithDebInfo` just also keeps debug symbols, which is worth having
  for a MEX file). Converted `MEX_GATEWAY_LOG_LEVEL` from a compile-time `#ifdef` to a
  runtime check (`std::getenv`, cached in a function-local `static`) in
  `mex_gateway.cpp` — set `MEX_GATEWAY_LOG_LEVEL=INFO` in the environment before
  starting MATLAB to get verbose logs, no rebuild required. This let the whole
  `MEX_GATEWAY_VERBOSE_LOGGING` CMake option and the `pixi-release-matlab`
  preset/`configure-release-matlab`/`build-release-matlab` tasks be deleted outright —
  down to a single `pixi-debug-matlab` preset and `configure-matlab`/`build-matlab`
  tasks for all MATLAB builds. Verified: `pixi run build-matlab` still builds
  `MexGateway.mexw64` correctly, and the plain `pixi run build` (core toolbox, no
  MATLAB) is unaffected.

## Open items to resolve during implementation (not yet decided/verified)

- Whether `gtest` ships both `GTest::gtest` and `GTest::gtest_main` CMake targets as
  expected by `test/CMakeLists.txt` — not yet tried.
- Whether the `x86-debug`/`x86-release` presets are still needed.
- Whether MATLAB needs to be validated on Linux/macOS or Windows-only in practice.
- Whether VS Code's CMake Tools extension needs any special configuration to pick up a
  pixi-activated environment, or whether `pixi run` wrapping `cmake`/`ninja` directly is
  simpler and should replace the CMake Tools UI workflow entirely.
- Whether to add the same CMake post-build DLL-copy step (now implemented for
  `MexGateway`, see above) to `main.exe`/other executables too, so they run standalone
  without `pixi run` (needed for VS Code's debugger; skipped for now for `main.exe`
  specifically since `pixi run run` covers today's needs).

## References consulted while planning this

- [VTK package — prefix.dev/conda-forge](https://prefix.dev/channels/conda-forge/packages/vtk)
- [Build C++ projects with Pixi — prefix.dev blog](https://prefix.dev/blog/pixi-build-for-cmake-projects)
- [Building a C++ Package — Pixi docs](https://prefix-dev.github.io/pixi/v0.40.0/build/cpp/)
- Local vcpkg checkout at `C:\vcpkg`, ports `vtk`, `hdf5`, `cgns`, `seacas`, `netcdf-c`,
  `libaec` inspected directly at the pinned baseline commit to trace the actual dependency
  resolution (not guessed from vcpkg.json alone — cross-checked against
  `out/build/x64-debug/vcpkg-manifest-install.log`).
