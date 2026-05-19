#pragma once

#include <datadriven/datadriven.hpp>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>

namespace datadriven::internal {

class LineScanner {
public:
  explicit LineScanner(std::istream &in) : in_(in) {}

  bool Scan();
  const std::string &Text() const { return text_; }
  int Line() const { return line_; }

private:
  std::istream &in_;
  std::string text_;
  int line_ = 0;
};

class TestDataReader {
public:
  TestDataReader(std::string source_name, std::istream &in, bool record);

  std::optional<TestData> Next();
  std::string RewriteOutput() const { return rewrite_.str(); }
  void Emit(std::string_view s);

private:
  void ReadExpected(TestData &data);
  void ParseBlock(bool allow_blank_lines, std::string &line, std::ostream &out);
  void ParseBlockAllowingBlankLines(std::string &line, std::ostream &out);
  void ParseBlockUntilBlankLine(std::string &line, std::ostream &out);

  std::string source_name_;
  LineScanner scanner_;
  bool record_;
  std::ostringstream rewrite_;
};

} // namespace datadriven::internal
