#include "test_data_reader.hpp"

#include <stdexcept>
#include <string_view>
#include <utility>

#include "string_util.hpp"

namespace datadriven::internal {
namespace {

constexpr std::string_view kSeparator = "----";

bool IsSeparator(std::string_view line) { return line == kSeparator; }

} // namespace

bool LineScanner::Scan() {
  if (!std::getline(in_, text_))
    return false;
  if (!text_.empty() && text_.back() == '\r')
    text_.pop_back();
  ++line_;
  return true;
}

TestDataReader::TestDataReader(std::string source_name, std::istream &in,
                               bool record)
    : source_name_(std::move(source_name)), scanner_(in), record_(record) {}

void TestDataReader::Emit(std::string_view s) {
  if (record_)
    rewrite_ << s << '\n';
}

std::optional<TestData> TestDataReader::Next() {
  while (scanner_.Scan()) {
    TestData data;
    std::string line = scanner_.Text();
    Emit(line);
    data.pos = source_name_ + ":" + std::to_string(scanner_.Line());

    line = TrimSpace(line);
    if (line.empty() || StartsWith(line, "#"))
      continue;

    while (EndsWith(line, "\\") && scanner_.Scan()) {
      std::string next = scanner_.Text();
      Emit(next);
      line.pop_back();
      line += " ";
      line += TrimSpace(next);
    }

    auto [cmd, args] = ParseLine(line);
    if (cmd.empty())
      continue;
    data.cmd = std::move(cmd);
    data.cmd_args = std::move(args);

    if (data.cmd == "subtest")
      return data;

    std::ostringstream input;
    bool separator = false;
    while (scanner_.Scan()) {
      line = scanner_.Text();
      if (IsSeparator(line)) {
        separator = true;
        break;
      }
      Emit(line);
      input << line << '\n';
    }
    data.input = TrimSpace(input.str());
    if (separator)
      ReadExpected(data);
    data.rewrite = record_;
    return data;
  }
  return std::nullopt;
}

void TestDataReader::ReadExpected(TestData &data) {
  // Expected output is deliberately not emitted here. In rewrite mode the
  // runner replaces this whole block by calling EmitActual after the handler
  // returns.
  std::ostringstream out;
  std::string line;
  bool allow_blank_lines = false;

  if (scanner_.Scan()) {
    line = scanner_.Text();
    if (IsSeparator(line))
      allow_blank_lines = true;
  }

  ParseBlock(allow_blank_lines, line, out);
  data.expected = out.str();
}

void TestDataReader::ParseBlock(bool allow_blank_lines, std::string &line,
                                std::ostream &out) {
  if (allow_blank_lines) {
    ParseBlockAllowingBlankLines(line, out);
    return;
  }

  ParseBlockUntilBlankLine(line, out);
}

void TestDataReader::ParseBlockAllowingBlankLines(std::string &line,
                                                  std::ostream &out) {
  while (scanner_.Scan()) {
    line = scanner_.Text();

    if (!IsSeparator(line)) {
      out << line << '\n';
      continue;
    }

    if (!scanner_.Scan()) {
      out << line << '\n';
      continue;
    }

    std::string line2 = scanner_.Text();

    if (!IsSeparator(line2)) {
      out << line << '\n' << line2 << '\n';
      continue;
    }

    if (scanner_.Scan() && !scanner_.Text().empty()) {
      throw std::runtime_error(
          "non-blank line after end of double ---- separator section");
    }

    return;
  }

  throw std::runtime_error("unterminated double ---- separator section");
}

void TestDataReader::ParseBlockUntilBlankLine(std::string &line,
                                              std::ostream &out) {
  while (true) {
    if (TrimSpace(line).empty())
      return;

    out << line << '\n';

    if (!scanner_.Scan())
      return;

    line = scanner_.Text();
  }
}

} // namespace datadriven::internal
