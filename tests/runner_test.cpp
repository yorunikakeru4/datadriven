#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

#include <atomic>
#include <chrono>
#include <sstream>
#include <thread>

namespace {

std::string RenderVector(const std::vector<std::string>& vals) {
  if (vals.empty()) return "{}";
  std::ostringstream out;
  out << "{";
  for (std::size_t i = 0; i < vals.size(); ++i) {
    if (i != 0) out << ", ";
    out << '"' << vals[i] << '"';
  }
  out << "}";
  return out.str();
}

}  // namespace

TEST_CASE("RunTest handles directive example") {
  datadriven::RunTest("tests/testdata/directive", [](const datadriven::TestData& d) {
    std::ostringstream out;
    out << "cmd: " << d.cmd << "\n";
    out << d.cmd_args.size() << " arguments\n";
    for (const auto& arg : d.cmd_args) {
      out << "key=\"" << arg.key << "\" vals=" << RenderVector(arg.vals) << "\n";
    }
    return out.str();
  });
}

TEST_CASE("RunTest handles multiline example") {
  datadriven::RunTest("tests/testdata/multiline", [](const datadriven::TestData& d) {
    if (d.cmd == "small") return std::string("just\ntwo lines of output");
    if (d.cmd == "large") return std::string("more\nthan\nfive\nlines\nof\noutput");
    throw std::runtime_error("unknown directive: " + d.cmd);
  });
}

TEST_CASE("RunTest handles string example") {
  datadriven::RunTest("tests/testdata/string", [](const datadriven::TestData& d) {
    if (d.cmd == "empty") return std::string("empty");
    return d.ToString();
  });
}

TEST_CASE("RunTest handles retry example") {
  std::atomic<int> value = 0;
  datadriven::RunTest("tests/testdata/retry", [&](const datadriven::TestData& d) {
    if (d.cmd == "inc") {
      int n = 1;
      d.MaybeScanArg("n", n);
      for (int i = 0; i < n; ++i) {
        std::thread([&value] {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          value.fetch_add(1);
        }).detach();
      }
      return std::string("");
    }
    if (d.cmd == "read") {
      return d.Retry([&] { return std::to_string(value.load()); });
    }
    throw std::runtime_error("unknown directive: " + d.cmd);
  });
}
