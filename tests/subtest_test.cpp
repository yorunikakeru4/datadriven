#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

#include "test_paths.hpp"

TEST_CASE("RunTest validates and skips subtest wrapper directives") {
  datadriven::RunTest(datadriven::test::TestDataPath("subtest").string(),
                      [](const datadriven::TestData &d) {
                        REQUIRE(d.cmd == "hello");
                        return d.cmd_args[0].key + " was said";
                      });
}
