#include "datadriven/internal/platform.hpp"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("process id macro returns a positive process id") {
  CHECK(DATADRIVEN_PROCESS_ID() > 0);
}
