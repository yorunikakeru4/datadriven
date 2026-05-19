#pragma once

#include <filesystem>
#include <string_view>

namespace datadriven::test {

inline std::filesystem::path TestDataPath(std::string_view relative) {
  return std::filesystem::path(DATADRIVEN_TESTDATA_DIR) / relative;
}

} // namespace datadriven::test
