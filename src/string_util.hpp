#pragma once

#include <cctype>
#include <sstream>
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
  return false;
}

inline std::string IndentLines(std::string_view s) {
  if (s.empty()) {
    return "";
  }
  std::stringstream out;
  bool line_start = true;
  for (char ch : s) {
    if (line_start && ch != '\n') {
      out << "  ";
    }
    out << ch;
    line_start = ch == '\n';
  }
  return out.str();
}

} // namespace datadriven::internal
