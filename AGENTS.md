@/home/yorunikakeru/.codex/RTK.md

# C++20 Datadriven Library

Rules here apply to this repository. This project is a C++20 implementation of a data-driven testing library compatible with the core behavior of `github.com/cockroachdb/datadriven`.

## Project Direction

- Treat `docs/superpowers/plans/2026-05-19-cpp20-datadriven.md` as the current implementation plan and source of project shape.
- `docs/` is intentionally ignored by git. Read it for context, but do not add, commit, move, or rewrite files under `docs/` unless the user explicitly asks.
- Build the library around parser, reader, runner, rewrite engine, argument scanner, filesystem walker, and a small CLI.
- Public API belongs under `include/datadriven/`; implementation belongs under `src/`; tests belong under `tests/`; CLI tools belong under `tools/`.
- Prefer small, focused translation units. Keep public API stable and implementation details private.

## Core Rules

- Use C++20 only. Do not require compiler extensions.
- Use CMake as the build system and Catch2 v3 for tests.
- Use `std::filesystem` for path traversal and file operations.
- Use `std::chrono` for retry and timeout behavior.
- Keep code portable across common Linux/macOS C++20 toolchains.
- Prefer value types, RAII, and standard library facilities over custom infrastructure.
- Avoid unnecessary allocations and copies. Use `std::string_view` for non-owning inputs when lifetime is clear.
- Prefer early returns and straightforward control flow.
- Keep happy-path code left-aligned; handle invalid input, empty state, and errors first.
- Keep code ordered top-down: public entry points before internal helpers where practical.
- Keep functions small and single-purpose. Split parsing, validation, formatting, and I/O when they start to mix.
- Avoid clever templates and abstractions unless they remove real duplication or protect the public API.
- Keep names consistent with the planned API: `CmdArg`, `TestData`, `Options`, `RunTest`, `RunTestFromString`, `Walk`, `ClearResults`.

## Comments And Documentation

- Document all public functions, structs, fields, aliases, and constants in headers.
- Public comments should explain contract, inputs, outputs, errors, ownership, lifetime, and mutation when relevant.
- Internal comments should explain non-obvious behavior, format edge cases, or compatibility choices.
- Do not add comments that restate obvious code.
- When a function mutates arguments, filesystem state, or testdata, document that mutation at the API boundary.
- Keep examples short and tied to datadriven behavior, not generic C++ usage.

Bad:

```cpp
// Runs test.
void RunTest(std::string_view path, Handler handler);
```

Good:

```cpp
// Runs every datadriven entry in path and compares handler output with the
// expected block. Throws std::runtime_error with file and line context on parse
// errors or output mismatches.
void RunTest(std::string_view path, Handler handler);
```

Bad:

```cpp
// Increments i.
++i;
```

Good:

```cpp
// CockroachDB datadriven treats a blank line after ---- as part of expected
// output, so keep the trailing empty line here.
expected_lines.push_back("");
```

## Control Flow Examples

Bad:

```cpp
std::string CmdArg::SingleVal() const {
  if (vals.size() == 1) {
    return vals[0];
  } else {
    throw std::runtime_error("expected one value");
  }
}
```

Good:

```cpp
std::string CmdArg::SingleVal() const {
  if (vals.size() != 1) {
    throw std::runtime_error("expected one value");
  }
  return vals[0];
}
```

Bad:

```cpp
std::vector<std::filesystem::path> entries;
for (const auto& entry : std::filesystem::directory_iterator(path)) {
  entries.push_back(entry.path());
}
return entries;
```

Good:

```cpp
std::vector<std::filesystem::path> entries;
for (const auto& entry : std::filesystem::directory_iterator(path)) {
  entries.push_back(entry.path());
}
std::ranges::sort(entries);
return entries;
```

## API And Behavior

- Match the core behavior of CockroachDB datadriven where practical:
  - directive line parsing,
  - command arguments with values,
  - input and expected-output blocks,
  - rewrite support,
  - retry helpers,
  - deterministic filesystem walking.
- Preserve human-readable testdata format. Do not make snapshots depend on incidental formatting.
- Error messages should include useful file/position context when parsing or running testdata.
- `Walk` should produce deterministic ordering.
- Rewrite behavior should be explicit through `Options::rewrite` or CLI command, not implicit.

## Repository Conventions

- Keep the top-level `CMakeLists.txt` authoritative for targets.
- Keep `Justfile` recipes aligned with CMake targets.
- Put public includes behind `#include <datadriven/datadriven.hpp>`.
- Use `.hpp` for headers and `.cpp` for implementation files.
- Do not introduce external dependencies beyond those already planned unless there is a clear need and the user agrees.
- Use Conventional Commits for commit messages, for example `feat: add directive parser`.

## Tests

- Prefer tests that exercise public behavior and fixture files over implementation details.
- Use Catch2 assertions.
- Keep testdata readable and close to CockroachDB datadriven examples where useful.
- Use table-style test sections when cases are small and similar.
- Add focused regression tests for parser, reader, runner, rewrite, retry, and walk behavior.
- For rewrite tests, compare full before/after files.

## Commands

- Prefix shell commands with `rtk`.
- Configure: `rtk cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug`
- Build: `rtk cmake --build build --parallel`
- Test: `rtk ctest --test-dir build --output-on-failure`
- Direct test binary: `rtk ./build/datadriven_tests`
- With Just: `rtk just test`
- Format: `rtk just format`

## Changelog

- No changelog is required until the project has a user-facing release process.
