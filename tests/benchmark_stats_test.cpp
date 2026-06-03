#include <catch2/catch_test_macros.hpp>
#include <datadriven/internal/benchmark.hpp>
#include <cmath>

using datadriven::internal::BenchmarkStats;
using datadriven::internal::ComputeStats;
using datadriven::internal::FormatStats;

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
