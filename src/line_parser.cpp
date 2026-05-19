#include <datadriven/datadriven.hpp>

#include <cctype>
#include <stdexcept>
#include <string>

#include "string_util.hpp"

namespace datadriven {
namespace {

std::size_t FindAny(std::string_view s, std::string_view chars) {
  const auto idx = s.find_first_of(chars);
  if (idx == std::string_view::npos) {
    return s.size();
  }
  return idx;
}

void TrimLineStart(std::string_view& line) {
  while (!line.empty() &&
         std::isspace(static_cast<unsigned char>(line.front())) != 0) {
    line.remove_prefix(1);
  }
}

[[noreturn]] void ParseError(const std::string& original, std::string_view rest) {
  const auto column = original.size() - rest.size() + 1;
  throw std::runtime_error("cannot parse directive at column " + std::to_string(column) +
                           ": " + std::string(original));
}

std::string ConsumeUntil(std::string_view& line, std::string_view chars) {
  const auto idx = FindAny(line, chars);
  std::string result(line.substr(0, idx));
  line.remove_prefix(idx);
  return result;
}

void ConsumeListValue(const std::string& original, std::string_view& line, CmdArg& arg) {
  std::size_t pos = 1;
  int nest = 1;
  std::size_t last = pos;
  while (nest > 0) {
    if (pos == line.size()) {
      ParseError(original, line);
    }
    const char ch = line[pos++];
    if (ch == ',' && nest == 1) {
      arg.vals.emplace_back(line.substr(last, pos - last - 1));
      while (pos < line.size() &&
             std::isspace(static_cast<unsigned char>(line[pos])) != 0) {
        ++pos;
      }
      last = pos;
      continue;
    }
    if (ch == '(') {
      ++nest;
      continue;
    }
    if (ch == ')') {
      --nest;
    }
  }
  arg.vals.emplace_back(line.substr(last, pos - last - 1));
  line.remove_prefix(pos);
}

}  // namespace

std::pair<std::string, std::vector<CmdArg>> ParseLine(std::string_view input) {
  const std::string original_owned = internal::TrimSpace(input);
  std::string owned = original_owned;
  std::string_view line(owned);
  if (line.empty()) {
    return {"", {}};
  }

  std::string cmd = ConsumeUntil(line, " \t");
  if (cmd.empty()) {
    ParseError(original_owned, line);
  }

  TrimLineStart(line);
  std::vector<CmdArg> args;
  while (!line.empty()) {
    CmdArg arg;
    arg.key = ConsumeUntil(line, " =\t");
    if (arg.key.empty()) {
      ParseError(original_owned, line);
    }

    if (!line.empty() && line.front() == '=') {
      line.remove_prefix(1);
      if (line.empty() || std::isspace(static_cast<unsigned char>(line.front())) != 0) {
        arg.vals.push_back("");
      } else if (line.front() != '(') {
        arg.vals.push_back(ConsumeUntil(line, " \t"));
      } else {
        ConsumeListValue(original_owned, line, arg);
      }
    }

    args.push_back(std::move(arg));
    TrimLineStart(line);
  }

  return {cmd, args};
}

}  // namespace datadriven
