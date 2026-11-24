# Formatting and linting

VAT uses formatters for C++, CMake and MATLAB, plus two linters (clang-tidy for C++,
MATLAB's `codeIssues` for the MATLAB package). pixi installs all of them at the versions
pinned in `pixi.lock`, so everyone gets the same output.

| Language | Tool | Config | Kind |
|---|---|---|---|
| C/C++ | clang-format 23 | [`.clang-format`](.clang-format) | formatter |
| CMake | gersemi | none (default settings) | formatter |
| MATLAB | MISS_HIT `mh_style` | [`miss_hit.cfg`](miss_hit.cfg) | formatter |
| C/C++ | clang-tidy | [`.clang-tidy`](.clang-tidy) | linter |
| MATLAB | MATLAB `codeIssues` | none | linter |

The tools live in three separate pixi environments:

| Environment | Contains | Builds on the default env? |
|---|---|---|
| `format` | the formatters: clang-format, gersemi, MISS_HIT (miss-hit-core), and prek (a pre-commit runner) | no, so it installs quickly and the git hooks start fast |
| `matlab` | only the `lint-matlab` task, no packages; it calls your local MATLAB | no |
| `tidy` | clang-tools, which provides clang-tidy | yes |

## Quick start

```powershell
pixi run -e format setup-hooks     # one-time: install the git pre-commit hooks
pixi run -e format format          # format the whole repository
```

After `setup-hooks`, every `git commit` formats the staged files. If a hook changes a
file, the commit stops. Review the changes, `git add` them and commit again.

## Commands

| Command | What it does |
|---|---|
| `pixi run -e format setup-hooks` | Installs the hooks from `.pre-commit-config.yaml` into `.git/hooks` (`prek install`). |
| `pixi run -e format format` | Runs every hook on every file (`prek run --all-files`). |
| `pixi run -e matlab lint-matlab` | Runs MATLAB's `codeIssues` on `matlab/+vat`. Prints the number of errors and issues and fails only if there are errors. Needs a local MATLAB on `PATH`. |
| `pixi run build; pixi run -e tidy lint-cpp` | Runs clang-tidy on every translation unit in `out/build/dev/compile_commands.json`. |

To run a single tool on a single file:

```powershell
pixi run -e format clang-format -i src/loads/pixel_load_integrator.cpp
pixi run -e format gersemi -i src/loads/CMakeLists.txt
pixi run -e format mh_style --fix matlab/+vat/+loads/PixelForceTorqueCalculator.m
pixi run -e format prek run clang-format --all-files     # one hook, whole repo
```

### clang-tidy needs a fresh build

`lint-cpp` reads `out/build/dev/compile_commands.json`. The `dev` CMake preset writes that
file because it sets `CMAKE_EXPORT_COMPILE_COMMANDS=ON`. Run `pixi run build` first, and
again whenever you add files or change compile flags.

Do not add a `build` dependency to the tidy tasks. A build started from the `tidy`
environment breaks the post-build gtest discovery, because the DLLs the tests need live in
the default environment.

## Configuration

### C++: `.clang-format`

LLVM base style with these changes:

- 4-space indent, no tabs, 120-column limit
- `PointerAlignment: Left` (`const T& x`, `T* p`)
- `AccessModifierOffset: -4`, so `public:`/`private:` line up with `class`
- `LineEnding: DeriveLF`: each file keeps its own line endings. This works with the
  `* text=auto` rule in `.gitattributes`.
- `SortIncludes: Never`. `hybrid_aero_load_calculator.cpp` defines `FMT_UNICODE` before it
  includes spdlog, and sorting the includes would break that.

### C++: `.clang-tidy`

The check set is deliberately small, to be widened once the existing findings are triaged:

- Enabled: `bugprone-*`, `performance-*`, `modernize-use-nullptr`, `modernize-use-override`
- Disabled because they are too noisy for this code base:
  - `bugprone-reserved-identifier`: the unit-suffix naming convention
    (`v_rel_B__m_per_s`) uses `__`, which triggers about 1,100 findings.
  - `bugprone-easily-swappable-parameters`, `bugprone-throwing-static-initialization`,
    `bugprone-implicit-widening-of-multiplication-result`,
    `bugprone-unhandled-code-paths`, `bugprone-switch-missing-default-case`,
    `bugprone-signed-bitwise`, `performance-enum-size`
- `HeaderFilterRegex` only reports findings in `src/`, `test/`, `examples/` and
  `bindings/`, never in third-party headers.
- `WarningsAsErrors` is empty, so findings never fail the run.

### CMake: gersemi

Uses gersemi's default settings. The hook runs on every `CMakeLists.txt` and `*.cmake`
file.

### MATLAB: `miss_hit.cfg`

- 120-column limit, 4-space tabs
- Suppressed rules:
  - `naming_functions`, `naming_parameters`, `naming_classes`, `naming_scripts`: the
    MATLAB code uses snake_case.
  - `copyright_notice`: the files have no copyright headers.
  - `end_of_statements`: the examples print intermediate results by leaving off the
    semicolon on purpose.
- The hook skips `matlab/bin/` and `matlab/resources/`, which hold build output and the
  generated toolbox.

### Hooks: `.pre-commit-config.yaml`

All three hooks are `local` hooks whose `entry` is `pixi run -e format <tool>`. They use the
pinned versions from `pixi.lock`, not whatever is installed on the machine. prek reads the
standard pre-commit format, so `pre-commit` works on this file as well.

## Known limitations

- clang-tidy cannot parse CGAL's `FPU.h`, so the sources in `src/remeshing/` are only
  partly analysed.
- No CI job runs these tools yet. Formatting is enforced only by the local hooks.
- The existing sources have not been reformatted yet. The first `format` run on the
  whole repository will change many files. Commit that run on its own, separate from other
  work.
