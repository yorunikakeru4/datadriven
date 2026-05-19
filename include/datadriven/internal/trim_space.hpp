#pragma once

#include <cctype>
#include <string>
#include <string_view>

namespace datadriven::internal {

inline std::string TrimSpace(std::string_view s) {
  auto begin = s.begin();
  auto end = s.end();
  while (begin != end && std::isspace(static_cast<unsigned char>(*begin))) {
    ++begin;
  }
  while (begin != end && std::isspace(static_cast<unsigned char>(*(end - 1)))) {
    --end;
  }
  return std::string(begin, end);
}

} // namespace datadriven::internal
