#pragma once

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string_view>

namespace datadriven::test {

inline std::filesystem::path TestDataPath(std::string_view relative) {
  return std::filesystem::path(DATADRIVEN_TESTDATA_DIR) / relative;
}

inline void WriteFile(const std::filesystem::path &path,
                      std::string_view data) {
  std::ofstream out(path, std::ios::trunc);
  if (!out) {
    throw std::runtime_error("failed to open for writing: " + path.string());
  }
  out << data;
}

} // namespace datadriven::test
