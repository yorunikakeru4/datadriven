#pragma once

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace datadriven {

// Options controls runner behavior for a single RunTest or
// RunTestFromString call.
struct Options {
  // rewrite replaces each expected block with handler output when true.
  // RunTest writes the updated file atomically after all directives pass.
  // RunTestFromString rejects rewrite mode because it has no path to update.
  bool rewrite = false;
};

// BenchmarkOptions controls BenchmarkFor behaviour.
struct BenchmarkOptions {
  // iterations is the number of measured calls to fn. Must be >= 1.
  int iterations = 1000;
  // warmup is the number of calls before measurement begins.
  // Use to prime caches and branch predictor for short hot-path benchmarks.
  int warmup = 0;
  // tolerance is the maximum relative deviation from the recorded expected
  // stats before a benchmark fails (0.10 = ±10%).
  double tolerance = 0.10;
};

// CmdArg is one directive argument, optionally with one or more values.
struct CmdArg {
  // key is the argument name exactly as parsed from the directive line.
  std::string key;
  // vals stores values from key=value or key=(v1, v2). It is empty for flags.
  std::vector<std::string> vals;

  // ToString returns the canonical datadriven argument spelling without
  // mutating the argument. Flags are formatted as key, single values as
  // key=value, and multi-values as key=(v1, v2).
  std::string ToString() const;
  // FirstVal returns the first parsed value. Throws std::runtime_error when the
  // argument has no values.
  std::string FirstVal() const;
  // SingleVal returns the only parsed value. Throws std::runtime_error unless
  // vals contains exactly one entry.
  std::string SingleVal() const;
  // TwoVals returns the two parsed values. Throws std::runtime_error unless
  // vals contains exactly two entries.
  std::pair<std::string, std::string> TwoVals() const;
  // ExpectNumVals validates the exact value count. Throws std::runtime_error
  // with the argument key and observed count when vals.size() != n.
  void ExpectNumVals(std::size_t n) const;
  // ExpectNumValsGE validates the minimum value count. Throws
  // std::runtime_error with the argument key and observed count when
  // vals.size() < n.
  void ExpectNumValsGE(std::size_t n) const;

  // Scan parses vals[i] into T without mutating vals. Supported scalar types
  // are std::string, bool, integral, and floating point types. Throws
  // std::runtime_error when i is out of range or the value cannot be parsed.
  template <class T> T Scan(std::size_t i = 0) const;
};

// TestData is one parsed datadriven directive plus its input and expected
// block.
struct TestData {
  // pos is source position in "path:line" form.
  std::string pos;
  // cmd is the directive command.
  std::string cmd;
  // cmd_args are directive arguments in source order.
  std::vector<CmdArg> cmd_args;
  // input is the optional input block with surrounding blank space trimmed.
  std::string input;
  // expected is the expected output block, including its trailing newline.
  std::string expected;
  // rewrite is true while RunTest is rewriting expected blocks.
  bool rewrite = false;

  // FullCmd returns cmd and all arguments in canonical directive-line form. It
  // does not include input, separators, expected output, or a trailing newline.
  std::string FullCmd() const;
  // ToString returns the directive and input block without expected output. The
  // returned string always ends in a newline and does not mutate the TestData.
  std::string ToString() const;
  // HasArg reports whether an argument with key is present. It performs an
  // exact key match and leaves repeated arguments in source order.
  bool HasArg(std::string_view key) const;
  // Arg returns a pointer to the first argument matching key, or nullptr when
  // absent. The pointer is owned by this TestData and remains valid until
  // cmd_args is mutated or the TestData is destroyed.
  const CmdArg *Arg(std::string_view key) const;

  // MaybeScanArg parses the argument into out and returns false when missing.
  // Scalar outputs require exactly one argument value. For vector<T>, every
  // argument value is parsed and out is cleared before new values are appended.
  template <class T> bool MaybeScanArg(std::string_view key, T &out) const;

  // ScanArg parses a required argument into out or throws std::runtime_error
  // with source position context when the argument is missing. It follows the
  // same scalar and vector value-count rules as MaybeScanArg.
  template <class T> void ScanArg(std::string_view key, T &out) const;

  // RetryFor repeatedly calls fn until its trimmed output matches expected for
  // 3 consecutive stable calls, or until timeout elapses. Sleep interval is
  // ~1% of timeout (floor 1ms). In rewrite mode it sleeps for timeout/10,
  // calls fn once, and returns that output.
  template <class Fn>
  std::string RetryFor(std::chrono::milliseconds timeout, Fn &&fn) const;
  // Retry calls RetryFor with the default one-second timeout.
  template <class Fn> std::string Retry(Fn &&fn) const;

  // BenchmarkFor runs fn() opts.warmup times (unmeasured) then opts.iterations
  // times (measured). It computes mean, p50, p95, p99 nanosecond stats and
  // returns a formatted stats line.
  //
  // When rewrite is true or expected is empty, it returns the actual stats.
  // Otherwise it parses expected, compares each metric within opts.tolerance,
  // and returns the expected text unchanged when all metrics are in range —
  // so the test passes without churn from normal timing variance. When any
  // metric falls outside tolerance it returns the actual stats, failing the
  // comparison with a diff showing what changed.
  //
  // Throws std::invalid_argument when opts.iterations < 1 or opts.warmup < 0.
  template <class Fn>
  std::string BenchmarkFor(const BenchmarkOptions &opts, Fn &&fn) const;
};

} // namespace datadriven
