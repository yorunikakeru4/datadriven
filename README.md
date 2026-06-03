# datadriven-cpp

C++20 port of the core [CockroachDB datadriven](https://github.com/cockroachdb/datadriven) testing style.

Data-driven testing keeps test logic in code and test cases in plain text files. Each file contains a sequence of directives — a command with optional arguments, an optional input block, and an expected output block. The test handler receives parsed directives and returns actual output; the library compares it against expected and reports mismatches with file and line context.

## Quick Start

**1. Add to your CMake project via FetchContent:**

```cmake
include(FetchContent)
FetchContent_Declare(
  datadriven
  GIT_REPOSITORY https://github.com/yorunikakeru4/datadriven
  GIT_TAG        main
)
FetchContent_MakeAvailable(datadriven)
```

**2. Link against the library:**

```cmake
target_link_libraries(my_tests PRIVATE datadriven)
```

**3. Write a testdata file** (`tests/testdata/mytest`):

```
echo
hello world
----
hello world

upper
hello
----
HELLO
```

**4. Write the test** (`tests/mytest.cpp`):

No test framework required — `RunTest` throws `std::runtime_error` with file/line context on any mismatch.

```cpp
#include <datadriven/datadriven.hpp>

int main() {
    datadriven::RunTest("tests/testdata/mytest", [](const datadriven::TestData& d) {
        if (d.cmd == "echo") return d.input;
        if (d.cmd == "upper") {
            std::string s = d.input;
            std::ranges::transform(s, s.begin(), ::toupper);
            return s;
        }
        throw std::runtime_error("unknown cmd: " + d.cmd);
    });
}
```

Add the executable to `CMakeLists.txt`:

```cmake
add_executable(mytest tests/mytest.cpp)
target_link_libraries(mytest PRIVATE datadriven)
```

<details>
<summary>With Catch2</summary>

```cpp
#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

TEST_CASE("mytest") {
    datadriven::RunTest("tests/testdata/mytest", [](const datadriven::TestData& d) {
        if (d.cmd == "echo") return d.input;
        if (d.cmd == "upper") {
            std::string s = d.input;
            std::ranges::transform(s, s.begin(), ::toupper);
            return s;
        }
        throw std::runtime_error("unknown cmd: " + d.cmd);
    });
}
```

</details>

**5. Run:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
./build/mytest
```

On failure you get the file path and line number of the mismatched directive. To regenerate expected output after logic changes, run with `DATADRIVEN_REWRITE=1`.

## How It Works

A testdata file consists of one or more entries with this structure:

```
<cmd> [arg] [key=value] [key=(v1, v2)]
<optional input block>
----
<expected output>
```

`RunTest` reads the file, calls your handler once per directive, and diffs the return value against the expected block. On mismatch it throws with the file path and line number so the failure is immediately locatable.

## Testdata Format

### Basic directive

```
echo
----
hello
```

The command is `echo`, there is no input, and the expected output is `hello\n`.

### Directive with input

```
upper
hello world
----
HELLO WORLD
```

The text before `----` (after the directive line) becomes `TestData::input`.

### Arguments

```
add x=1 y=2
----
3
```

Arguments support flags, single values, and multi-values:

```
cmd flag key=value multi=(a, b, c)
----
```

| Syntax          | `CmdArg::key` | `CmdArg::vals`    |
| --------------- | ------------- | ----------------- |
| `flag`          | `"flag"`      | `{}`              |
| `key=value`     | `"key"`       | `{"value"}`       |
| `key=(a, b, c)` | `"key"`       | `{"a", "b", "c"}` |

### Multiline output

When expected output is longer than five lines, `----` delimiters wrap both sides:

```
dump
----
----
line one
line two
line three
line four
line five
line six
----
----
```

### Subtests

Directives can be grouped into named subtests that nest arbitrarily:

```
subtest outer

hello
----
world

subtest outer/inner

greet
----
hi

subtest end outer/inner

subtest end outer
```

### Retry

For async behavior, the handler can call `d.Retry` or `d.RetryFor` to poll until output stabilises:

```
inc n=5
----

read
----
5
```

```cpp
if (d.cmd == "inc") {
    int n = 1;
    d.MaybeScanArg("n", n);
    // launch n async increments ...
    return {};
}
if (d.cmd == "read") {
    return d.Retry([&] { return std::to_string(counter.load()); });
}
```

### Benchmarks

Record timing stats alongside regular testdata:

```
bench_sort iters=5000 warmup=100 tolerance=15
1 5 3 2 4
----
mean=142ns p50=138ns p95=187ns p99=201ns
```

On every run the library compares actual stats against the recorded values. When all metrics fall within `tolerance`% of the baseline, the test passes without touching the file. When timing drifts outside the band the test fails with a diff — you decide whether to update the baseline.

## Usage

### RunTest

```cpp
#include <datadriven/datadriven.hpp>

datadriven::RunTest("tests/testdata/mytest", [](const datadriven::TestData& d) {
    if (d.cmd == "echo") return d.input;
    if (d.cmd == "upper") {
        std::string s = d.input;
        std::ranges::transform(s, s.begin(), ::toupper);
        return s;
    }
    throw std::runtime_error("unknown directive: " + d.cmd);
});
```

### RunTestFromString

Embed testdata inline in a test without a file:

```cpp
datadriven::RunTestFromString(R"(
echo
hello
----
hello
)", [](const datadriven::TestData& d) {
    return d.input;
});
```

### Reading arguments

```cpp
int x = 0, y = 0;
d.ScanArg("x", x);   // throws if missing
d.ScanArg("y", y);

std::vector<std::string> items;
d.MaybeScanArg("items", items);  // returns false if absent
```

### Walk

Run a handler across every file in a directory in deterministic order:

```cpp
datadriven::Walk("tests/testdata", [](std::string_view path) {
    datadriven::RunTest(path, handler);
});
```

### ClearResults

Erase all expected blocks in a testdata file (useful when regenerating from scratch):

```cpp
datadriven::ClearResults("tests/testdata/mytest");
```

### BenchmarkFor

Time arbitrary work inside a handler and return a stats line the runner can compare:

```cpp
datadriven::BenchmarkOptions opts;
int tol_pct = 10;
d.MaybeScanArg("iters",     opts.iterations);
d.MaybeScanArg("warmup",    opts.warmup);
d.MaybeScanArg("tolerance", tol_pct);
opts.tolerance = tol_pct / 100.0;
return d.BenchmarkFor(opts, [&] { sort_data(d.input); });
```

`BenchmarkFor` runs `opts.warmup` unmeasured calls to prime caches, then `opts.iterations` measured calls, and returns `mean=Xns p50=Xns p95=Xns p99=Xns` (auto-scaled to ns, us, or ms). Use `Options{.rewrite = true}` to record a new baseline when the workload changes.

## Rewrite Mode

When the expected output changes, update the testdata file instead of editing it by hand.

Via environment variable:

```bash
DATADRIVEN_REWRITE=1 just test
```

Via `Options`:

```cpp
datadriven::RunTest(path, handler, datadriven::Options{.rewrite = true});
```

In rewrite mode the library replaces each expected block with actual handler output and writes the file atomically. Run without rewrite afterward to confirm all entries pass.

## Build

```bash
just build
```

or with CMake directly:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --parallel
```

## Test

```bash
just test
```

or:

```bash
ctest --test-dir build --output-on-failure
```

## Inspired By

- [cockroachdb/datadriven](https://github.com/cockroachdb/datadriven) — original Go implementation
