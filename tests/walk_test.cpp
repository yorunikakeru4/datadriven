#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>
#include <filesystem>
#include <fstream>
#include <vector>

namespace {

void WriteFile(const std::filesystem::path &path, std::string_view data) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to open for writing: " + path.string());
  }
  out << data;
}

} // namespace

TEST_CASE("Walk visits files in deterministic order and strips extension for "
          "examples") {
  const auto dir =
      std::filesystem::temp_directory_path() / "datadriven-cpp-walk";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  WriteFile(dir / "noext", "name\n----\ntest name: noext\n");
  WriteFile(dir / "ext.test", "name\n----\ntest name: ext\n");

  std::vector<std::string> paths;
  datadriven::Walk(dir.string(), [&](std::string_view path) {
    paths.emplace_back(path);
    const std::string name = std::filesystem::path(path).stem().string();
    datadriven::RunTest(path, [&](const datadriven::TestData &d) {
      REQUIRE(d.cmd == "name");
      return "test name: " + name;
    });
  });
  REQUIRE(paths == std::vector<std::string>{(dir / "ext.test").string(),
                                            (dir / "noext").string()});
}
