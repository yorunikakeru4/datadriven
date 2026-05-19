#pragma once

#include <charconv>
#include <chrono>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
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
  // a stable period, or until timeout-derived attempts are exhausted. In
  // rewrite mode it sleeps for timeout/10, calls fn once, and returns that
  // output.
  template <class Fn>
  std::string RetryFor(std::chrono::milliseconds timeout, Fn &&fn) const;
  // Retry calls RetryFor with the default one-second timeout.
  template <class Fn> std::string Retry(Fn &&fn) const;
};

// Handler is called once per non-subtest directive. It receives immutable
// parsed test data and returns the actual output to compare or write.
using Handler = std::function<std::string(const TestData &)>;

// ParseLine parses one directive line into command and arguments. Leading and
// trailing whitespace is ignored, list values may be nested, and malformed
// argument syntax throws std::runtime_error with column context.
std::pair<std::string, std::vector<CmdArg>> ParseLine(std::string_view line);
// RunTest reads path, calls handler for each directive, and compares handler
// output with expected blocks. Throws std::runtime_error with file and
// directive context on parse errors, invalid subtests, open failures, or output
// mismatches. When options.rewrite is true, path is rewritten atomically with
// handler output after all directives have been processed.
void RunTest(std::string_view path, Handler handler, Options options = {});
// RunTestFromString runs datadriven input supplied as a string. It calls
// handler with source position "<string>:line" and throws std::runtime_error on
// parse errors or output mismatches. It throws std::runtime_error when
// options.rewrite is true because there is no file to update.
void RunTestFromString(std::string_view input, Handler handler,
                       Options options = {});
// Walk visits path or all files under path in deterministic lexical order.
// Temporary editor files are skipped during directory traversal. Throws
// std::runtime_error when path does not exist.
void Walk(std::string_view path,
          const std::function<void(std::string_view)> &fn);
// ClearResults rewrites path with empty expected output blocks by running the
// rewrite engine with an empty handler. It mutates the file at path atomically
// and throws std::runtime_error on parse or filesystem errors.
void ClearResults(std::string_view path);

namespace internal {

template <class T> struct IsVector : std::false_type {};

template <class T, class Alloc>
struct IsVector<std::vector<T, Alloc>> : std::true_type {};

template <class T> inline constexpr bool always_false_v = false;

template <class T> T ParseScalar(const std::string &s) {
  if constexpr (std::is_same_v<T, std::string>) {
    return s;
  } else if constexpr (std::is_same_v<T, bool>) {
    if (s == "true" || s == "1") {
      return true;
    }
    if (s == "false" || s == "0") {
      return false;
    }
    throw std::runtime_error("parse bool: " + s);
  } else if constexpr (std::is_integral_v<T>) {
    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
      throw std::runtime_error("parse integer: " + s);
    }
    return value;
  } else if constexpr (std::is_floating_point_v<T>) {
    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
      throw std::runtime_error("parse float: " + s);
    }
    return value;
  } else {
    static_assert(always_false_v<T>, "unsupported Scan type");
  }
}

inline std::string RetryTrimSpace(std::string_view s) {
  auto begin = s.begin();
  auto end = s.end();
  while (begin != end && (*begin == ' ' || *begin == '\t' || *begin == '\n' ||
                          *begin == '\r' || *begin == '\f' || *begin == '\v')) {
    ++begin;
  }
  while (begin != end &&
         (*(end - 1) == ' ' || *(end - 1) == '\t' || *(end - 1) == '\n' ||
          *(end - 1) == '\r' || *(end - 1) == '\f' || *(end - 1) == '\v')) {
    --end;
  }
  return std::string(begin, end);
}

} // namespace internal

template <class T> T CmdArg::Scan(std::size_t i) const {
  if (i >= vals.size()) {
    throw std::runtime_error("cannot scan index " + std::to_string(i) +
                             " of key " + key);
  }
  return internal::ParseScalar<T>(vals[i]);
}

template <class T>
bool TestData::MaybeScanArg(std::string_view key, T &out) const {
  const CmdArg *arg = Arg(key);
  if (arg == nullptr) {
    return false;
  }
  if constexpr (internal::IsVector<T>::value) {
    out.clear();
    using Elem = typename T::value_type;
    for (std::size_t i = 0; i < arg->vals.size(); ++i) {
      out.push_back(arg->Scan<Elem>(i));
    }
    return true;
  } else {
    arg->ExpectNumVals(1);
    out = arg->Scan<T>(0);
    return true;
  }
}

template <class T> void TestData::ScanArg(std::string_view key, T &out) const {
  if (!MaybeScanArg(key, out)) {
    throw std::runtime_error(pos + ": missing argument: " + std::string(key));
  }
}

template <class Fn>
std::string TestData::RetryFor(std::chrono::milliseconds timeout,
                               Fn &&fn) const {
  if (rewrite) {
    std::this_thread::sleep_for(timeout / 10);
    return std::invoke(std::forward<Fn>(fn));
  }
  constexpr int attempts = 100;
  constexpr int stable = 3;
  int ok = 0;
  const auto expected_trimmed = internal::RetryTrimSpace(expected);
  for (int i = 0;; ++i) {
    std::string s = std::invoke(fn);
    if (internal::RetryTrimSpace(s) == expected_trimmed) {
      ++ok;
      if (ok == stable || i == attempts) {
        return s;
      }
      std::this_thread::sleep_for(timeout / attempts +
                                  std::chrono::milliseconds(1));
      continue;
    }
    ok = 0;
    if (i == attempts) {
      return s;
    }
    std::this_thread::sleep_for(timeout / attempts +
                                std::chrono::milliseconds(1));
  }
}

template <class Fn> std::string TestData::Retry(Fn &&fn) const {
  return RetryFor(std::chrono::seconds(1), std::forward<Fn>(fn));
}

} // namespace datadriven
