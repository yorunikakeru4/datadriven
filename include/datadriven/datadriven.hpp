#pragma once

#include <charconv>
#include <chrono>
#include <cstddef>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace datadriven {

// Options controls runner behavior for a single RunTest call.
struct Options {
  // rewrite replaces each expected block with handler output when true.
  bool rewrite = false;
  // quiet is reserved for callers that want reduced diagnostic output.
  bool quiet = false;
};

// CmdArg is one directive argument, optionally with one or more values.
struct CmdArg {
  // key is the argument name exactly as parsed from the directive line.
  std::string key;
  // vals stores values from key=value or key=(v1, v2). It is empty for flags.
  std::vector<std::string> vals;

  // ToString returns the canonical datadriven argument spelling.
  std::string ToString() const;
  // FirstVal returns the first value or throws std::runtime_error when absent.
  std::string FirstVal() const;
  // SingleVal returns the only value or throws std::runtime_error otherwise.
  std::string SingleVal() const;
  // TwoVals returns exactly two values or throws std::runtime_error otherwise.
  std::pair<std::string, std::string> TwoVals() const;
  // ExpectNumVals throws std::runtime_error unless vals has exactly n entries.
  void ExpectNumVals(std::size_t n) const;
  // ExpectNumValsGE throws std::runtime_error unless vals has at least n entries.
  void ExpectNumValsGE(std::size_t n) const;

  // Scan parses vals[i] into T. Supported scalar types are string, bool,
  // integral, and floating point types.
  template <class T>
  T Scan(std::size_t i = 0) const;
};

// TestData is one parsed datadriven directive plus its input and expected block.
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

  // FullCmd returns cmd and all arguments in canonical directive-line form.
  std::string FullCmd() const;
  // ToString returns the directive and input block without expected output.
  std::string ToString() const;
  // HasArg reports whether an argument with key is present.
  bool HasArg(std::string_view key) const;
  // Arg returns the first argument matching key, or nullptr when absent.
  const CmdArg* Arg(std::string_view key) const;

  // MaybeScanArg parses the argument into out and returns false when missing.
  // For vector<T>, all argument values are parsed and out is cleared first.
  template <class T>
  bool MaybeScanArg(std::string_view key, T& out) const;

  // ScanArg parses a required argument into out or throws std::runtime_error
  // with source position context when the argument is missing.
  template <class T>
  void ScanArg(std::string_view key, T& out) const;

  // RetryFor repeatedly calls fn until its trimmed output matches expected for
  // a stable period, or until timeout-derived attempts are exhausted.
  std::string RetryFor(std::chrono::milliseconds timeout,
                       const std::function<std::string()>& fn) const;
  // Retry calls RetryFor with the default one-second timeout.
  std::string Retry(const std::function<std::string()>& fn) const;
};

// Handler is called once per non-subtest directive.
using Handler = std::function<std::string(const TestData&)>;

// ParseLine parses one directive line into command and arguments.
std::pair<std::string, std::vector<CmdArg>> ParseLine(std::string_view line);
// RunTest reads path, calls handler for each directive, and compares expected output.
void RunTest(std::string_view path, Handler handler, Options options = {});
// RunTestFromString runs datadriven input supplied as a string.
void RunTestFromString(std::string_view input, Handler handler, Options options = {});
// Walk visits files under path in deterministic lexical order.
void Walk(std::string_view path, const std::function<void(std::string_view)>& fn);
// ClearResults rewrites path with empty expected output blocks.
void ClearResults(std::string_view path);

namespace internal {

template <class T>
struct IsVector : std::false_type {};

template <class T, class Alloc>
struct IsVector<std::vector<T, Alloc>> : std::true_type {};

template <class T>
T ParseScalar(const std::string& s) {
  if constexpr (std::is_same_v<T, std::string>) {
    return s;
  } else if constexpr (std::is_same_v<T, bool>) {
    if (s == "true" || s == "1") return true;
    if (s == "false" || s == "0") return false;
    throw std::runtime_error("parse bool: " + s);
  } else if constexpr (std::is_integral_v<T>) {
    T value{};
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
    if (ec != std::errc() || ptr != s.data() + s.size()) {
      throw std::runtime_error("parse integer: " + s);
    }
    return value;
  } else if constexpr (std::is_floating_point_v<T>) {
    std::istringstream in(s);
    T value{};
    in >> value;
    if (!in || !in.eof()) throw std::runtime_error("parse float: " + s);
    return value;
  } else {
    static_assert(!sizeof(T), "unsupported Scan type");
  }
}

}  // namespace internal

template <class T>
T CmdArg::Scan(std::size_t i) const {
  if (i >= vals.size()) {
    throw std::runtime_error("cannot scan index " + std::to_string(i) + " of key " + key);
  }
  return internal::ParseScalar<T>(vals[i]);
}

template <class T>
bool TestData::MaybeScanArg(std::string_view key, T& out) const {
  const CmdArg* arg = Arg(key);
  if (arg == nullptr) return false;
  if constexpr (internal::IsVector<T>::value) {
    out.clear();
    using Elem = typename T::value_type;
    for (std::size_t i = 0; i < arg->vals.size(); ++i) {
      out.push_back(arg->Scan<Elem>(i));
    }
  } else {
    arg->ExpectNumVals(1);
    out = arg->Scan<T>(0);
  }
  return true;
}

template <class T>
void TestData::ScanArg(std::string_view key, T& out) const {
  if (!MaybeScanArg(key, out)) {
    throw std::runtime_error(pos + ": missing argument: " + std::string(key));
  }
}

}  // namespace datadriven
