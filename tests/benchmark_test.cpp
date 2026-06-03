#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>
#include <datadriven/internal/benchmark.hpp>

#include "test_paths.hpp"

namespace {

// Handler for testdata/benchmark: reads iters/warmup/tolerance from the
// directive, runs a no-op, returns BenchmarkFor stats.
std::string BenchHandler(const datadriven::TestData &d) {
  datadriven::BenchmarkOptions opts;
  opts.iterations = 50;
  int tol_pct = 10;
  d.MaybeScanArg("iters",     opts.iterations);
  d.MaybeScanArg("warmup",    opts.warmup);
  d.MaybeScanArg("tolerance", tol_pct);
  opts.tolerance = tol_pct / 100.0;
  return d.BenchmarkFor(opts, [] { /* no-op */ });
}

} // namespace

TEST_CASE("BenchmarkFor within-tolerance path returns expected text") {
  // Construct TestData manually so we control expected and tolerance.
  datadriven::TestData d;
  d.expected  = "mean=100ns p50=100ns p95=100ns p99=100ns\n";
  d.rewrite   = false;

  datadriven::BenchmarkOptions opts;
  opts.iterations = 20;
  opts.tolerance  = 100.0; // ±10000% — always passes

  const std::string result = d.BenchmarkFor(opts, [] {});
  // Within tolerance: must return the recorded expected text (stripped newline).
  REQUIRE(result == "mean=100ns p50=100ns p95=100ns p99=100ns");
}

TEST_CASE("BenchmarkFor outside-tolerance path returns actual text") {
  datadriven::TestData d;
  // Expect 1 second per op — a no-op will be millions of times faster.
  d.expected = "mean=1000000000ns p50=1000000000ns p95=1000000000ns p99=1000000000ns\n";
  d.rewrite  = false;

  datadriven::BenchmarkOptions opts;
  opts.iterations = 10;
  opts.tolerance  = 0.10; // ±10%

  const std::string result = d.BenchmarkFor(opts, [] {});
  // Should NOT return the impossibly-slow expected value.
  REQUIRE(result != "mean=1000000000ns p50=1000000000ns p95=1000000000ns p99=1000000000ns");
  // Should be a valid stats string.
  const auto parsed = datadriven::internal::ParseStats(result);
  REQUIRE(parsed.has_value());
}

TEST_CASE("BenchmarkFor rewrite=true returns actual stats") {
  datadriven::TestData d;
  d.expected = "mean=999ns p50=999ns p95=999ns p99=999ns\n";
  d.rewrite  = true; // rewrite mode: always return actual

  datadriven::BenchmarkOptions opts;
  opts.iterations = 10;

  const std::string result = d.BenchmarkFor(opts, [] {});
  // Must not return the stale expected value.
  REQUIRE(result != "mean=999ns p50=999ns p95=999ns p99=999ns");
  const auto parsed = datadriven::internal::ParseStats(result);
  REQUIRE(parsed.has_value());
}

TEST_CASE("BenchmarkFor empty expected returns actual stats") {
  datadriven::TestData d;
  d.expected = "";
  d.rewrite  = false;

  datadriven::BenchmarkOptions opts;
  opts.iterations = 10;

  const std::string result = d.BenchmarkFor(opts, [] {});
  const auto parsed = datadriven::internal::ParseStats(result);
  REQUIRE(parsed.has_value());
}

TEST_CASE("BenchmarkFor warmup does not count toward stats") {
  // Not a timing assertion — just verifies warmup calls don't crash.
  datadriven::TestData d;
  d.expected = "";

  datadriven::BenchmarkOptions opts;
  opts.iterations = 5;
  opts.warmup     = 3;

  REQUIRE_NOTHROW(d.BenchmarkFor(opts, [] {}));
}

TEST_CASE("BenchmarkFor throws on invalid iterations") {
  datadriven::TestData d;
  d.expected = "";

  datadriven::BenchmarkOptions opts;
  opts.iterations = 0;

  REQUIRE_THROWS_AS(d.BenchmarkFor(opts, [] {}), std::invalid_argument);
}

TEST_CASE("BenchmarkFor throws on negative warmup") {
  datadriven::TestData d;
  d.expected = "";

  datadriven::BenchmarkOptions opts;
  opts.warmup = -1;

  REQUIRE_THROWS_AS(d.BenchmarkFor(opts, [] {}), std::invalid_argument);
}

TEST_CASE("BenchmarkFor runs testdata fixture") {
  datadriven::Walk(
      datadriven::test::TestDataPath("benchmark").string(),
      [](std::string_view path) {
        datadriven::RunTest(path, BenchHandler);
      });
}
