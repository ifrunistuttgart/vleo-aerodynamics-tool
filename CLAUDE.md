# Build System

Dependencies and the C++ toolchain are managed by [pixi](https://pixi.sh) + conda-forge.
See [README.md](README.md) for the actual build/run instructions. This file covers the
*why* behind non-obvious decisions and gotchas that aren't visible from the code alone.

## Background: why pixi instead of vcpkg

The project used to depend on vcpkg. That build was slow (VTK built from source, on the
order of hours) and fragile (a dependency chain reached out to `gitlab.dkrz.de` for
`libaec`, which frequently 429s). Root cause, confirmed by tracing
`out/build/x64-debug/vcpkg-manifest-install.log` and the vcpkg port manifests at the
pinned baseline: `vcpkg.json` depended on plain `"vtk"` with no feature selection, so
vcpkg built VTK's full default feature set (`cgns`, `netcdf`, `seacas`, `proj`, `sql`,
`libharu`, `libtheora`) — none of which the codebase actually uses (only
`CommonCore`/`RenderingOpenGL2`/etc. for plain mesh rendering, see
[aero_sat/visualization/CMakeLists.txt](aero_sat/visualization/CMakeLists.txt)). That
default feature set is what pulled in `netcdf-c` → `hdf5` → `libaec` in the first place —
the GitLab flakiness and most of the build time were self-inflicted.

Switched to pixi + conda-forge instead of trying to trim vcpkg's feature selection,
because conda-forge ships VTK **prebuilt** (win-64/linux-64/osx-64/osx-arm64) — no VTK
build at all, and the feature set is fixed by the conda-forge recipe rather than
something to accidentally over-select again later. Docker was considered and rejected:
it virtualizes the whole OS for no benefit here (can't show a native GUI window well,
can't reach a MATLAB install on the host); pixi already gives reproducible, native,
per-OS environments without virtualization. MATLAB stays a manual, host-installed
prerequisite either way (commercial, not distributable via conda-forge) — this is fine
because [matlab/CMakeLists.txt](matlab/CMakeLists.txt) builds `MexGateway` via CMake's
own `matlab_add_mex()`, a normal shared-library target, not MATLAB's own `mex`
CLI/toolchain.

vcpkg has since been fully retired from this branch (`vcpkg.json`,
`vcpkg-configuration.json`, the `overlay-ports/` workaround, and the `x64-*`/`x86-*`
CMake presets are gone) — done ahead of the cross-platform/test-suite items below being
finished, by deliberate choice rather than oversight.

## Gotchas worth knowing

- **Use `RelWithDebInfo`, not `Debug`, on Windows.** conda-forge's Windows C++ packages
  are Release-only (`/MD`, linked against `MSVCP140.dll`) — there's no Debug (`/MDd`)
  variant. A `Debug` build mixes `/MDd` application code with `/MD` third-party DLLs
  (spdlog, assimp, VTK, …); any `std::string`/`std::vector` crossing that boundary
  corrupts memory and segfaults almost immediately. `RelWithDebInfo` maps to `/MD`
  (CRT-compatible) while still keeping debug symbols. This didn't come up with vcpkg,
  since vcpkg triplets build separate debug and release copies of every library. All
  `pixi-*` presets in `CMakePresets.json` use `RelWithDebInfo`/`Release` accordingly.
- **Always run built executables via `pixi run run` (or a `pixi shell`), never
  directly.** conda-forge's shared DLLs live in `.pixi/envs/default/Library/bin`, which
  is only on `PATH` inside a pixi-activated shell. Running `main.exe` directly fails
  with "X.dll was not found".
- **pixi's Windows compiler activation sets `CMAKE_GENERATOR`/`CMAKE_GENERATOR_PLATFORM`/
  `CMAKE_GENERATOR_TOOLSET`/`CMAKE_ARGS`** as convenience defaults for people not using
  presets (targeting the Visual Studio generator). Since our presets explicitly pick
  `Ninja` (a single-platform generator), the leftover `CMAKE_GENERATOR_PLATFORM=x64`
  conflicts: "Generator Ninja does not support platform specification". Setting
  `"environment": {"CMAKE_GENERATOR_PLATFORM": null}` inside the preset itself does
  **not** fix this (CMake reads these env vars before applying preset-level environment
  overrides) — they're cleared at the shell/task level instead, via `env = {...}` on
  the `configure`/`configure-matlab` tasks in `pixi.toml`. Driving these presets from a
  plain `pixi shell` instead of the `pixi run` tasks means `unset`-ing those four
  variables manually first.
- **MATLAB bindings are opt-in** (`pixi-debug-matlab` preset / `pixi run build-matlab`),
  not part of the default build. `find_package(Matlab)` is `REQUIRED` in
  `matlab/CMakeLists.txt`, so turning it on unconditionally would make MATLAB a hard
  prerequisite for the core toolbox, which it isn't.
- **`MexGateway.mexw64`'s runtime DLLs are copied into `matlab/bin` automatically** by
  a `POST_BUILD` step (see [cmake/copy_runtime_deps.cmake](cmake/copy_runtime_deps.cmake)),
  using `file(GET_RUNTIME_DEPENDENCIES)` to walk its actual PE import table rather than
  a hardcoded DLL list that would silently go stale. This exists because MATLAB loads
  the MEX file with no knowledge that a pixi environment exists, so its ~15 conda-forge
  DLL dependencies would otherwise be unfindable — copying them in means no `PATH`
  changes are ever needed to use it from MATLAB. `main.exe` doesn't get this treatment
  (covered by `pixi run run` instead) since, unlike MATLAB, it's always launched through
  a pixi task anyway.
- **`MexGateway`'s log verbosity is a runtime env var, not a compile-time option.**
  `mex_gateway.cpp`'s `log_level()` reads `MEX_GATEWAY_LOG_LEVEL` from the environment
  once (cached in a function-local `static`); set it to `INFO` before starting MATLAB
  to get per-call logs (routes through the MATLAB Engine API, slow, hence not the
  default). This used to be a compile-time `#ifdef` requiring a separate
  `pixi-release-matlab` preset just to get a quiet build — collapsed back to a single
  MATLAB preset once logging became a runtime switch instead.
- **conda-forge's VTK build is Python-version-locked**
  (`vtk-9.6.2-py314h...`) and pulls in a full Python + numpy/matplotlib-style
  dependency chain (`aiohttp`, `contourpy`, `cycler`, etc.) the project doesn't use.
  Still far faster than building VTK from source, but a real footprint trade-off.

## Open items

- `test/` (GoogleTest) isn't wired into the build on any preset — `BUILD_TESTING` is
  never set anywhere, so `add_subdirectory("test")` in the root `CMakeLists.txt` never
  runs. Predates the pixi migration; still open.
- Linux/macOS haven't actually been built or run yet — only Windows has been verified
  end-to-end so far, despite cross-platform being a goal of the migration.
- Whether VS Code's CMake Tools extension needs special configuration to pick up a
  pixi-activated environment, or whether `pixi run` wrapping `cmake`/`ninja` directly
  should just replace that workflow entirely.
- Whether to add the same DLL-copy step (`cmake/copy_runtime_deps.cmake`) to
  `main.exe`/other executables, so they run standalone without `pixi run` (mainly
  relevant for VS Code's debugger).

## References consulted during the migration

- [VTK package — prefix.dev/conda-forge](https://prefix.dev/channels/conda-forge/packages/vtk)
- [Build C++ projects with Pixi — prefix.dev blog](https://prefix.dev/blog/pixi-build-for-cmake-projects)
- [Building a C++ Package — Pixi docs](https://prefix-dev.github.io/pixi/v0.40.0/build/cpp/)
