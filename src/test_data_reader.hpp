#pragma once

#include <datadriven/internal/types.hpp>
#include <istream>
#include <ostream>
#include <sstream>
#include <string>

namespace datadriven::internal {

class LineScanner {
public:
  // LineScanner reads newline-delimited text while tracking one-based source
  // line numbers. A trailing '\r' is stripped so CRLF files parse like LF
  // files.
  explicit LineScanner(std::istream &in) : in_(in) {}

  // Scan advances to the next line and returns false at EOF. Text() and Line()
  // describe the most recently scanned line after a true result.
  bool Scan();
  // Text returns the most recently scanned line without its line ending.
  const std::string &Text() const { return text_; }
  // Line returns the one-based line number for Text(), or zero before Scan().
  int Line() const { return line_; }

private:
  std::istream &in_;
  std::string text_;
  int line_ = 0;
};

class TestDataReader {
public:
  // TestDataReader parses datadriven entries from in. When record is true,
  // emitted source and replacement output are accumulated for rewrite mode.
  TestDataReader(std::string source_name, std::istream &in, bool record);

  // Next returns the next parsed directive, or std::nullopt at EOF. Returned
  // TestData positions use source_name:line and rewrite is set from record.
  std::optional<TestData> Next();
  // RewriteOutput returns all content emitted while reading and rewriting.
  std::string RewriteOutput() const { return rewrite_.str(); }
  // Emit appends one logical line to rewrite output when record mode is active.
  void Emit(std::string_view s);

private:
  // ReadExpected consumes the expected block after a ---- separator and stores
  // it in data.expected. In rewrite mode it deliberately does not emit old
  // expected output.
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
