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

## Open items to resolve during implementation (not yet decided/verified)

- Exact conda-forge package names for `glfw`/`glfw3`, and whether `gtest` ships both
  `GTest::gtest` and `GTest::gtest_main` CMake targets as expected by `test/CMakeLists.txt`.
- Whether the `x86-debug`/`x86-release` presets are still needed.
- Whether MATLAB needs to be validated on Linux/macOS or Windows-only in practice.
- Exact pixi environment variable(s) to feed into `CMAKE_PREFIX_PATH`.
- Whether VS Code's CMake Tools extension needs any special configuration to pick up a
  pixi-activated environment, or whether `pixi run` wrapping `cmake`/`ninja` directly is
  simpler and should replace the CMake Tools UI workflow entirely.

## References consulted while planning this

- [VTK package — prefix.dev/conda-forge](https://prefix.dev/channels/conda-forge/packages/vtk)
- [Build C++ projects with Pixi — prefix.dev blog](https://prefix.dev/blog/pixi-build-for-cmake-projects)
- [Building a C++ Package — Pixi docs](https://prefix-dev.github.io/pixi/v0.40.0/build/cpp/)
- Local vcpkg checkout at `C:\vcpkg`, ports `vtk`, `hdf5`, `cgns`, `seacas`, `netcdf-c`,
  `libaec` inspected directly at the pinned baseline commit to trace the actual dependency
  resolution (not guessed from vcpkg.json alone — cross-checked against
  `out/build/x64-debug/vcpkg-manifest-install.log`).
