#include "test_data_reader.hpp"

#include <stdexcept>
#include <utility>

#include "string_util.hpp"

namespace datadriven::internal {

bool LineScanner::Scan() {
  if (!std::getline(in_, text_)) return false;
  if (!text_.empty() && text_.back() == '\r') text_.pop_back();
  ++line_;
  return true;
}

TestDataReader::TestDataReader(std::string source_name, std::istream& in, bool record)
    : source_name_(std::move(source_name)), scanner_(in), record_(record) {}

void TestDataReader::Emit(std::string_view s) {
  if (record_) rewrite_ << s << '\n';
}

std::optional<TestData> TestDataReader::Next() {
  while (scanner_.Scan()) {
    TestData data;
    std::string line = scanner_.Text();
    Emit(line);
    data.pos = source_name_ + ":" + std::to_string(scanner_.Line());

    line = TrimSpace(line);
    if (line.empty() || StartsWith(line, "#")) continue;

    while (EndsWith(line, "\\") && scanner_.Scan()) {
      std::string next = scanner_.Text();
      Emit(next);
      line.pop_back();
      line += " ";
      line += TrimSpace(next);
    }

    auto [cmd, args] = ParseLine(line);
    if (cmd.empty()) continue;
    data.cmd = std::move(cmd);
    data.cmd_args = std::move(args);

    if (data.cmd == "subtest") return data;

    std::ostringstream input;
    bool separator = false;
    while (scanner_.Scan()) {
      line = scanner_.Text();
      if (line == "----") {
        separator = true;
        break;
      }
      Emit(line);
      input << line << '\n';
    }
    data.input = TrimSpace(input.str());
    if (separator) ReadExpected(data);
    data.rewrite = record_;
    return data;
  }
  return std::nullopt;
}

void TestDataReader::ReadExpected(TestData& data) {
  std::ostringstream out;
  std::string line;
  bool allow_blank_lines = false;

  if (scanner_.Scan()) {
    line = scanner_.Text();
    if (line == "----") allow_blank_lines = true;
  }

  if (allow_blank_lines) {
    while (scanner_.Scan()) {
      line = scanner_.Text();
      if (line == "----") {
        if (scanner_.Scan()) {
          std::string line2 = scanner_.Text();
          if (line2 == "----") {
            if (scanner_.Scan() && !scanner_.Text().empty()) {
              throw std::runtime_error(
                  "non-blank line after end of double ---- separator section");
            }
            break;
          }
          out << line << '\n' << line2 << '\n';
          continue;
        }
      }
      out << line << '\n';
    }
  } else {
    while (true) {
      if (TrimSpace(line).empty()) break;
      out << line << '\n';
      if (!scanner_.Scan()) break;
      line = scanner_.Text();
    }
  }
  data.expected = out.str();
}

}  // namespace datadriven::internal
