#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

std::string ReadFile(const std::filesystem::path& path) {
  std::ifstream in(path);
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

void WriteFile(const std::filesystem::path& path, std::string_view data) {
  std::ofstream out(path, std::ios::trunc);
  out << data;
}

std::string RewriteHandler(const datadriven::TestData& d) {
  if (d.cmd == "noop") return d.input;
  if (d.cmd == "duplicate") return d.input + "\n" + d.input;
  if (d.cmd == "duplicate-with-blank") return d.input + "\n\n" + d.input;
  throw std::runtime_error("unknown directive: " + d.cmd);
}

}  // namespace

TEST_CASE("RunTest rewrite matches golden files") {
  const auto dir = std::filesystem::temp_directory_path() / "datadriven-cpp-rewrite";
  std::filesystem::create_directories(dir);

  for (std::string name : {"basic", "eof"}) {
    const auto source = std::filesystem::path("tests/testdata/rewrite") / (name + "-before");
    const auto expected = std::filesystem::path("tests/testdata/rewrite") / (name + "-after");
    const auto work = dir / name;
    WriteFile(work, ReadFile(source));
    datadriven::RunTest(work.string(), RewriteHandler, datadriven::Options{.rewrite = true});
    REQUIRE(ReadFile(work) == ReadFile(expected));
  }
}

TEST_CASE("ClearResults removes expected output") {
  const auto dir = std::filesystem::temp_directory_path() / "datadriven-cpp-clear";
  std::filesystem::create_directories(dir);
  const auto work = dir / "case";
  WriteFile(work, "cmd\ninput\n----\nold\n");
  datadriven::ClearResults(work.string());
  REQUIRE(ReadFile(work) == "cmd\ninput\n----\n");
}
