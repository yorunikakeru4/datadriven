#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace datadriven::internal {

// BenchmarkStats holds timing percentiles all in nanoseconds.
struct BenchmarkStats {
  double mean_ns = 0.0;
  double p50_ns  = 0.0;
  double p95_ns  = 0.0;
  double p99_ns  = 0.0;
};

// ComputeStats sorts samples and derives mean, p50, p95, p99.
// samples must be non-empty; values are nanoseconds as doubles.
BenchmarkStats ComputeStats(std::vector<double> samples);

// FormatStats serialises stats to "mean=Xns p50=Xns p95=Xns p99=Xns".
// Each value auto-scales to ns (<1000), us (<1e6), or ms.
std::string FormatStats(const BenchmarkStats &stats);

// ParseStats deserialises a string produced by FormatStats.
// Returns nullopt when the string is malformed or has an unknown key.
std::optional<BenchmarkStats> ParseStats(std::string_view s);

// StatsWithinTolerance returns true when every metric in actual is within
// tolerance fraction of the same metric in expected (e.g. 0.10 = ±10%).
bool StatsWithinTolerance(const BenchmarkStats &expected,
                          const BenchmarkStats &actual, double tolerance);

} // namespace datadriven::internal
