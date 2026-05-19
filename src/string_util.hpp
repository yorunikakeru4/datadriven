#pragma once

#include <algorithm>
#include <string>
#include <string_view>

#include <datadriven/internal/trim_space.hpp>

namespace datadriven::internal {

inline bool StartsWith(std::string_view s, std::string_view prefix) {
  return s.substr(0, prefix.size()) == prefix;
}

inline bool EndsWith(std::string_view s, std::string_view suffix) {
  return s.size() >= suffix.size() &&
         s.substr(s.size() - suffix.size()) == suffix;
}

inline bool HasWhitespaceOnlyLine(std::string_view s) {
  bool at_line_start = true;
  bool only_space = true;
  for (char ch : s) {
    if (ch == '\n') {
      if (at_line_start || only_space) {
        return true;
      }
      at_line_start = true;
      only_space = true;
      continue;
    }
    at_line_start = false;
    if (ch != ' ' && ch != '\t') {
      only_space = false;
    }
  }
  return only_space && !at_line_start;
}

inline std::string IndentLines(std::string_view s) {
  if (s.empty()) {
    return "";
  }
  std::string out;
  out.reserve(s.size() + 2 * (std::count(s.begin(), s.end(), '\n') + 1));
  bool line_start = true;
  for (char ch : s) {
    if (line_start && ch != '\n') {
      out += "  ";
    }
    out += ch;
    line_start = ch == '\n';
  }
  return out;
}

} // namespace datadriven::internal
