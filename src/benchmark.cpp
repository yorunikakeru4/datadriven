#include <datadriven/internal/benchmark.hpp>

#include <algorithm>
#include <charconv>
#include <cmath>
#include <numeric>
#include <sstream>
#include <iomanip>

namespace datadriven::internal {

namespace {

std::string FormatDuration(double ns) {
  std::ostringstream out;
  if (ns < 1000.0) {
    out << static_cast<long>(ns) << "ns";  // floor, not round — keeps value below threshold
  } else if (ns < 1e6) {
    out << std::fixed << std::setprecision(2) << (ns / 1000.0) << "us";
  } else {
    out << std::fixed << std::setprecision(2) << (ns / 1e6) << "ms";
  }
  return out.str();
}

// Parses "142ns", "1.42us", "1.42ms" → nanoseconds. Returns nullopt on error.
std::optional<double> ParseDurationNs(std::string_view s) {
  double multiplier = 1.0;
  if (s.ends_with("ms")) {
    multiplier = 1e6;
    s.remove_suffix(2);
  } else if (s.ends_with("us")) {
    multiplier = 1000.0;
    s.remove_suffix(2);
  } else if (s.ends_with("ns")) {
    s.remove_suffix(2);
  } else {
    return std::nullopt;
  }
  double value{};
  const auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), value);
  if (ec != std::errc() || ptr != s.data() + s.size()) {
    return std::nullopt;
  }
  return value * multiplier;
}

} // namespace

BenchmarkStats ComputeStats(std::vector<double> samples) {
  if (samples.empty()) {
    throw std::invalid_argument("ComputeStats: samples must be non-empty");
  }
  std::sort(samples.begin(), samples.end());
  const std::size_t n = samples.size();
  const double mean =
      std::accumulate(samples.begin(), samples.end(), 0.0) / static_cast<double>(n);
  const double p50 = samples[((n - 1) * 50) / 100];
  const double p95 = samples[((n - 1) * 95) / 100];
  const double p99 = samples[((n - 1) * 99) / 100];
  return {mean, p50, p95, p99};
}

std::string FormatStats(const BenchmarkStats &s) {
  return "mean=" + FormatDuration(s.mean_ns) +
         " p50=" + FormatDuration(s.p50_ns) +
         " p95=" + FormatDuration(s.p95_ns) +
         " p99=" + FormatDuration(s.p99_ns);
}

std::optional<BenchmarkStats> ParseStats(std::string_view s) {
  while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' '))
    s.remove_suffix(1);

  BenchmarkStats result{};
  bool got_mean = false, got_p50 = false, got_p95 = false, got_p99 = false;

  while (!s.empty()) {
    const auto sp = s.find(' ');
    const std::string_view token = (sp == std::string_view::npos) ? s : s.substr(0, sp);
    s = (sp == std::string_view::npos) ? std::string_view{} : s.substr(sp + 1);

    const auto eq = token.find('=');
    if (eq == std::string_view::npos) return std::nullopt;

    const std::string_view key = token.substr(0, eq);
    const auto ns = ParseDurationNs(token.substr(eq + 1));
    if (!ns) return std::nullopt;

    if      (key == "mean") { result.mean_ns = *ns; got_mean = true; }
    else if (key == "p50")  { result.p50_ns  = *ns; got_p50  = true; }
    else if (key == "p95")  { result.p95_ns  = *ns; got_p95  = true; }
    else if (key == "p99")  { result.p99_ns  = *ns; got_p99  = true; }
    else return std::nullopt;
  }

  if (!got_mean || !got_p50 || !got_p95 || !got_p99) return std::nullopt;
  return result;
}

bool StatsWithinTolerance(const BenchmarkStats &expected, const BenchmarkStats &actual,
                          double tolerance) {
  auto within = [tolerance](double exp, double act) {
    if (exp == 0.0) return act == 0.0;
    return std::abs(act - exp) / exp <= tolerance;
  };
  return within(expected.mean_ns, actual.mean_ns) &&
         within(expected.p50_ns,  actual.p50_ns)  &&
         within(expected.p95_ns,  actual.p95_ns)  &&
         within(expected.p99_ns,  actual.p99_ns);
}

} // namespace datadriven::internal
