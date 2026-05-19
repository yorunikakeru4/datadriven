#include <atomic>
#include <catch2/catch_test_macros.hpp>
#include <chrono>
#include <datadriven/datadriven.hpp>
#include <future>
#include <memory>
#include <sstream>
#include <thread>
#include <vector>

#include "test_paths.hpp"

namespace {

std::string RenderVector(const std::vector<std::string> &vals) {
  if (vals.empty())
    return "{}";
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < vals.size(); ++i) {
    if (i != 0)
      out << ", ";
    out << '"' << vals[i] << '"';
  }
  out << "}";
  return out.str();
}

} // namespace

TEST_CASE("RunTest handles directive example") {
  datadriven::RunTest(datadriven::test::TestDataPath("directive").string(),
                      [](const datadriven::TestData &d) {
                        std::ostringstream out;
                        out << "cmd: " << d.cmd << "\n";
                        out << d.cmd_args.size() << " arguments\n";
                        for (const auto &arg : d.cmd_args) {
                          out << "key=\"" << arg.key
                              << "\" vals=" << RenderVector(arg.vals) << "\n";
                        }
                        return out.str();
                      });
}

TEST_CASE("RunTest handles multiline example") {
  datadriven::RunTest(datadriven::test::TestDataPath("multiline").string(),
                      [](const datadriven::TestData &d) {
                        if (d.cmd == "small")
                          return std::string("just\ntwo lines of output");
                        if (d.cmd == "large")
                          return std::string(
                              "more\nthan\nfive\nlines\nof\noutput");
                        throw std::runtime_error("unknown directive: " + d.cmd);
                      });
}

TEST_CASE("RunTest handles string example") {
  datadriven::RunTest(datadriven::test::TestDataPath("string").string(),
                      [](const datadriven::TestData &d) {
                        if (d.cmd == "empty")
                          return std::string("empty");
                        return d.ToString();
                      });
}

TEST_CASE("RunTest handles retry example") {
  std::atomic<int> value = 0;
  std::vector<std::future<void>> increments;
  datadriven::RunTest(
      datadriven::test::TestDataPath("retry").string(),
      [&](const datadriven::TestData &d) {
        if (d.cmd == "inc") {
          int n = 1;
          d.MaybeScanArg("n", n);
          for (int i = 0; i < n; ++i) {
            increments.push_back(std::async(std::launch::async, [&value] {
              std::this_thread::sleep_for(std::chrono::milliseconds(1));
              value.fetch_add(1);
            }));
          }
          return std::string{};
        }
        if (d.cmd == "read") {
          return d.Retry([&] { return std::to_string(value.load()); });
        }
        throw std::runtime_error("unknown directive: " + d.cmd);
      });
  for (auto &increment : increments) {
    increment.get();
  }
}

TEST_CASE("Retry accepts move-only callables") {
  datadriven::RunTestFromString(
      R"(
read
----
7
)",
      [](const datadriven::TestData &d) {
        return d.RetryFor(std::chrono::milliseconds(1),
                          [value = std::make_unique<int>(7)] {
                            return std::to_string(*value);
                          });
      });
}

TEST_CASE("RetryFor keeps the timeout budget") {
  datadriven::TestData d;
  d.expected = "expected";
  const auto timeout = std::chrono::milliseconds(100);
  const auto start = std::chrono::steady_clock::now();
  const auto result =
      d.RetryFor(timeout, [] { return std::string("actual"); });
  const auto elapsed = std::chrono::steady_clock::now() - start;
  REQUIRE(result == "actual");
  CHECK(elapsed < std::chrono::milliseconds(170));
}
