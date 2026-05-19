#include "string_util.hpp"
#include <cctype>
#include <datadriven/datadriven.hpp>
#include <stdexcept>
#include <string>

namespace datadriven {
namespace {

std::size_t FindAny(std::string_view s, std::string_view chars) {
  const auto idx = s.find_first_of(chars);
  if (idx == std::string_view::npos) {
    return s.size();
  }
  return idx;
}

void TrimLineStart(std::string_view &line) {
  while (!line.empty() &&
         std::isspace(static_cast<unsigned char>(line.front())) != 0) {
    line.remove_prefix(1);
  }
}

[[noreturn]] void ParseError(std::string_view original,
                             std::string_view rest) {
  const auto column = original.size() - rest.size() + 1;
  throw std::runtime_error("cannot parse directive at column " +
                           std::to_string(column) + ": " +
                           std::string(original));
}

std::string ConsumeUntil(std::string_view &line, std::string_view chars) {
  const auto idx = FindAny(line, chars);
  std::string result(line.substr(0, idx));
  line.remove_prefix(idx);
  return result;
}

void ConsumeListValue(const std::string &original, std::string_view &line,
                      CmdArg &arg) {
  int nest = 1;
  line.remove_prefix(1);
  TrimLineStart(line);
  std::string_view value = line;
  while (nest > 0) {
    if (line.empty()) {
      ParseError(original, line);
    }
    const char ch = line.front();
    line.remove_prefix(1);
    if (ch == ',' && nest == 1) {
      const auto value_size =
          static_cast<std::size_t>((line.data() - value.data()) - 1);
      arg.vals.emplace_back(value.substr(0, value_size));
      TrimLineStart(line);
      value = line;
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
  const auto value_size =
      static_cast<std::size_t>((line.data() - value.data()) - 1);
  arg.vals.emplace_back(value.substr(0, value_size));
}

} // namespace

std::pair<std::string, std::vector<CmdArg>> ParseLine(std::string_view input) {
  const std::string original = internal::TrimSpace(input);
  std::string_view line(original);
  if (line.empty()) {
    return {"", {}};
  }

  std::string cmd = ConsumeUntil(line, " \t");
  if (cmd.empty()) {
    ParseError(original, line);
  }

  TrimLineStart(line);
  std::vector<CmdArg> args;
  while (!line.empty()) {
    CmdArg arg;
    arg.key = ConsumeUntil(line, " =\t");
    if (arg.key.empty()) {
      ParseError(original, line);
    }

    if (line.empty() || line.front() != '=') {
      args.push_back(std::move(arg));
      TrimLineStart(line);
      continue;
    }

    line.remove_prefix(1);
    if (line.empty() ||
        std::isspace(static_cast<unsigned char>(line.front())) != 0) {
      arg.vals.push_back("");
    } else if (line.front() != '(') {
      arg.vals.push_back(ConsumeUntil(line, " \t"));
    } else {
      ConsumeListValue(original, line, arg);
    }

    args.push_back(std::move(arg));
    TrimLineStart(line);
  }

  return {cmd, args};
}

} // namespace datadriven
