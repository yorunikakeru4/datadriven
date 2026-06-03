#include <catch2/catch_test_macros.hpp>
#include <datadriven/internal/benchmark.hpp>
#include <cmath>

using datadriven::internal::BenchmarkStats;
using datadriven::internal::ComputeStats;
using datadriven::internal::FormatStats;
using datadriven::internal::ParseStats;
using datadriven::internal::StatsWithinTolerance;

TEST_CASE("FormatStats formats nanoseconds") {
  BenchmarkStats s{142.0, 138.0, 187.0, 201.0};
  REQUIRE(FormatStats(s) == "mean=142ns p50=138ns p95=187ns p99=201ns");
}

TEST_CASE("FormatStats formats microseconds") {
  BenchmarkStats s{1420.0, 1380.0, 1870.0, 2010.0};
  REQUIRE(FormatStats(s) == "mean=1.42us p50=1.38us p95=1.87us p99=2.01us");
}

TEST_CASE("FormatStats formats milliseconds") {
  BenchmarkStats s{1420000.0, 1380000.0, 1870000.0, 2010000.0};
  REQUIRE(FormatStats(s) == "mean=1.42ms p50=1.38ms p95=1.87ms p99=2.01ms");
}

TEST_CASE("FormatStats formats mixed units") {
  BenchmarkStats s{142.0, 1420.0, 1420000.0, 2010000.0};
  REQUIRE(FormatStats(s) == "mean=142ns p50=1.42us p95=1.42ms p99=2.01ms");
}

TEST_CASE("ComputeStats computes mean and percentiles") {
  // 100 samples: values 1..100 ns
  std::vector<double> samples(100);
  for (int i = 0; i < 100; ++i) samples[i] = static_cast<double>(i + 1);
  const auto s = ComputeStats(std::move(samples));
  REQUIRE(std::abs(s.mean_ns - 50.5) < 0.01);
  REQUIRE(s.p50_ns == 50.0);
  REQUIRE(s.p95_ns == 95.0);
  REQUIRE(s.p99_ns == 99.0);
}

TEST_CASE("ComputeStats single sample") {
  std::vector<double> samples{42.0};
  const auto s = ComputeStats(std::move(samples));
  REQUIRE(s.mean_ns == 42.0);
  REQUIRE(s.p50_ns == 42.0);
  REQUIRE(s.p95_ns == 42.0);
  REQUIRE(s.p99_ns == 42.0);
}

TEST_CASE("ParseStats round-trips nanoseconds") {
  const BenchmarkStats orig{142.0, 138.0, 187.0, 201.0};
  const auto parsed = ParseStats(FormatStats(orig));
  REQUIRE(parsed.has_value());
  REQUIRE(std::abs(parsed->mean_ns - orig.mean_ns) < 1.0);
  REQUIRE(std::abs(parsed->p50_ns  - orig.p50_ns)  < 1.0);
  REQUIRE(std::abs(parsed->p95_ns  - orig.p95_ns)  < 1.0);
  REQUIRE(std::abs(parsed->p99_ns  - orig.p99_ns)  < 1.0);
}

TEST_CASE("ParseStats round-trips microseconds") {
  const BenchmarkStats orig{1420.0, 1380.0, 1870.0, 2010.0};
  const auto parsed = ParseStats(FormatStats(orig));
  REQUIRE(parsed.has_value());
  REQUIRE(std::abs(parsed->mean_ns - orig.mean_ns) < 10.0);
}

TEST_CASE("ParseStats round-trips milliseconds") {
  const BenchmarkStats orig{1420000.0, 1380000.0, 1870000.0, 2010000.0};
  const auto parsed = ParseStats(FormatStats(orig));
  REQUIRE(parsed.has_value());
  REQUIRE(std::abs(parsed->mean_ns - orig.mean_ns) < 10000.0);
}

TEST_CASE("ParseStats tolerates trailing newline") {
  const auto parsed = ParseStats("mean=142ns p50=138ns p95=187ns p99=201ns\n");
  REQUIRE(parsed.has_value());
  REQUIRE(std::abs(parsed->mean_ns - 142.0) < 1.0);
}

TEST_CASE("ParseStats returns nullopt for malformed input") {
  REQUIRE_FALSE(ParseStats("garbage").has_value());
  REQUIRE_FALSE(ParseStats("mean=142ns p50=138ns").has_value()); // missing p95/p99
  REQUIRE_FALSE(ParseStats("mean=142XX p50=138ns p95=187ns p99=201ns").has_value());
}

TEST_CASE("StatsWithinTolerance passes when all metrics within 10%") {
  const BenchmarkStats expected{100.0, 100.0, 100.0, 100.0};
  const BenchmarkStats actual{105.0, 95.0, 109.0, 91.0};
  REQUIRE(StatsWithinTolerance(expected, actual, 0.10));
}

TEST_CASE("StatsWithinTolerance fails when one metric exceeds tolerance") {
  const BenchmarkStats expected{100.0, 100.0, 100.0, 100.0};
  const BenchmarkStats actual{115.0, 100.0, 100.0, 100.0}; // mean 15% over
  REQUIRE_FALSE(StatsWithinTolerance(expected, actual, 0.10));
}

TEST_CASE("StatsWithinTolerance handles tolerance of 0") {
  const BenchmarkStats expected{100.0, 100.0, 100.0, 100.0};
  REQUIRE(StatsWithinTolerance(expected, expected, 0.0));
  const BenchmarkStats actual{100.1, 100.0, 100.0, 100.0};
  REQUIRE_FALSE(StatsWithinTolerance(expected, actual, 0.0));
}
