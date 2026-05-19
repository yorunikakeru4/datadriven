#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "test_paths.hpp"

namespace {

std::string ReadFile(const std::filesystem::path &path) {
  std::ifstream in(path);
  std::ostringstream out;
  out << in.rdbuf();
  return out.str();
}

std::string RewriteHandler(const datadriven::TestData &d) {
  if (d.cmd == "noop")
    return d.input;
  if (d.cmd == "duplicate")
    return d.input + "\n" + d.input;
  if (d.cmd == "duplicate-with-blank")
    return d.input + "\n\n" + d.input;
  throw std::runtime_error("unknown directive: " + d.cmd);
}

} // namespace

TEST_CASE("RunTest rewrite matches golden files") {
  const auto dir =
      std::filesystem::temp_directory_path() / "datadriven-cpp-rewrite";
  std::filesystem::create_directories(dir);

  for (std::string name : {"basic", "eof"}) {
    const auto source =
        datadriven::test::TestDataPath("rewrite") / (name + "-before");
    const auto expected =
        datadriven::test::TestDataPath("rewrite") / (name + "-after");
    const auto work = dir / name;
    datadriven::test::WriteFile(work, ReadFile(source));
    datadriven::RunTest(work.string(), RewriteHandler,
                        datadriven::Options{.rewrite = true});
    REQUIRE(ReadFile(work) == ReadFile(expected));
  }
}

TEST_CASE("RunTestFromString rejects rewrite mode") {
  try {
    datadriven::RunTestFromString(
        "cmd\n----\nold\n", [](const datadriven::TestData &) { return "new"; },
        datadriven::Options{.rewrite = true});
    FAIL("rewrite mode should be rejected for string input");
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()) ==
            "RunTestFromString does not support rewrite mode");
  }
}

TEST_CASE("ClearResults removes expected output") {
  const auto dir =
      std::filesystem::temp_directory_path() / "datadriven-cpp-clear";
  std::filesystem::create_directories(dir);
  const auto work = dir / "case";
  datadriven::test::WriteFile(work, "cmd\ninput\n----\nold\n");
  datadriven::ClearResults(work.string());
  REQUIRE(ReadFile(work) == "cmd\ninput\n----\n");
}
