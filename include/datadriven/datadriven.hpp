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

#include <datadriven/internal/benchmark.hpp>
#include <datadriven/internal/types.hpp>

namespace datadriven {

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

// Internal implementation details required by header-defined template
// functions. Not part of the public API; names and types may change.
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
  // Require a few consecutive matches to smooth over transient output changes.
  constexpr int stable = 3;
  const auto deadline = std::chrono::steady_clock::now() + timeout;
  // Sleep ~1% of the timeout per attempt; floor at 1ms to avoid busy-waiting.
  const auto sleep_interval =
      std::max(timeout / 100, std::chrono::milliseconds(1));
  int ok = 0;
  // Normalize by appending '\n' if missing, matching how the runner normalizes
  // handler output before comparison. TrimSpace was too aggressive: it would
  // accept whitespace-padded output as "stable" even when it could not pass the
  // runner's exact comparison, causing premature exit.
  auto normalize = [](const std::string &s) -> std::string {
    return (!s.empty() && s.back() == '\n') ? s : s + '\n';
  };
  const std::string expected_normalized = normalize(expected);
  std::string last;
  for (;;) {
    last = std::invoke(fn);
    if (normalize(last) == expected_normalized) {
      if (++ok >= stable) {
        return last;
      }
    } else {
      ok = 0;
    }
    if (std::chrono::steady_clock::now() >= deadline) {
      return last;
    }
    std::this_thread::sleep_for(sleep_interval);
  }
}

template <class Fn> std::string TestData::Retry(Fn &&fn) const {
  return RetryFor(std::chrono::seconds(1), std::forward<Fn>(fn));
}

template <class Fn>
std::string TestData::BenchmarkFor(const BenchmarkOptions &opts,
                                   Fn &&fn) const {
  for (int i = 0; i < opts.warmup; ++i) {
    std::invoke(fn);
  }
  std::vector<double> samples(static_cast<std::size_t>(opts.iterations));
  for (int i = 0; i < opts.iterations; ++i) {
    const auto t0 = std::chrono::steady_clock::now();
    std::invoke(fn);
    const auto t1 = std::chrono::steady_clock::now();
    samples[static_cast<std::size_t>(i)] =
        std::chrono::duration<double, std::nano>(t1 - t0).count();
  }
  const internal::BenchmarkStats actual_stats =
      internal::ComputeStats(std::move(samples));
  const std::string actual = internal::FormatStats(actual_stats);

  if (rewrite) {
    return actual;
  }
  // Strip trailing newline from expected before parsing.
  std::string_view exp = expected;
  while (!exp.empty() && (exp.back() == '\n' || exp.back() == '\r'))
    exp.remove_suffix(1);

  if (exp.empty()) {
    return actual;
  }
  const auto exp_stats = internal::ParseStats(exp);
  if (exp_stats &&
      internal::StatsWithinTolerance(*exp_stats, actual_stats, opts.tolerance)) {
    return std::string(exp); // return exactly what was recorded — diff stays clean
  }
  return actual;
}

} // namespace datadriven
