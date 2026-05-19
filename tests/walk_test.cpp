#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

#include <filesystem>
#include <vector>

TEST_CASE("Walk visits files in deterministic order and strips extension for examples") {
  std::vector<std::string> paths;
  datadriven::Walk("tests/testdata/walk", [&](std::string_view path) {
    paths.emplace_back(path);
    const std::string name = std::filesystem::path(path).stem().string();
    datadriven::RunTest(path, [&](const datadriven::TestData& d) {
      REQUIRE(d.cmd == "name");
      return "test name: " + name;
    });
  });
  REQUIRE(paths.size() == 2);
  REQUIRE(paths[0].find("ext.test") != std::string::npos);
  REQUIRE(paths[1].find("noext") != std::string::npos);
}
