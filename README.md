# datadriven-cpp

C++20 port of the core CockroachDB datadriven testing style.

## Build

```bash
just build
```

## Test

```bash
just test
```

## Example

```cpp
datadriven::RunTest("tests/testdata/directive", [](const datadriven::TestData& d) {
  if (d.cmd == "echo") return d.input;
  throw std::runtime_error("unknown directive: " + d.cmd);
});
```

## Rewrite

```bash
DATADRIVEN_REWRITE=1 just test
```

or call:

```cpp
datadriven::RunTest(path, handler, datadriven::Options{.rewrite = true});
```
