#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>
#include <filesystem>
#include <vector>

#include "test_paths.hpp"

TEST_CASE("Walk visits files in deterministic order and strips extension for "
          "examples") {
  const auto dir =
      std::filesystem::temp_directory_path() / "datadriven-cpp-walk";
  std::filesystem::remove_all(dir);
  std::filesystem::create_directories(dir);
  datadriven::test::WriteFile(dir / "noext", "name\n----\ntest name: noext\n");
  datadriven::test::WriteFile(dir / "ext.test", "name\n----\ntest name: ext\n");

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
