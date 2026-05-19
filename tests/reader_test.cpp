#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

#include "test_data_reader.hpp"

#include <sstream>
#include <stdexcept>
#include <string>

TEST_CASE("TestDataReader reads simple testcase") {
  std::istringstream input("cmd a=b\nhello\n----\nworld\n\n");
  datadriven::internal::TestDataReader reader("<string>", input, false);
  auto td = reader.Next();
  REQUIRE(td.has_value());
  REQUIRE(td->pos == "<string>:1");
  REQUIRE(td->cmd == "cmd");
  REQUIRE(td->cmd_args.size() == 1);
  REQUIRE(td->input == "hello");
  REQUIRE(td->expected == "world\n");
  REQUIRE_FALSE(reader.Next().has_value());
}

TEST_CASE("TestDataReader reads multiline expected output") {
  std::istringstream input("large\n----\n----\na\n\nb\n----\n----\n\n");
  datadriven::internal::TestDataReader reader("<string>", input, false);
  auto td = reader.Next();
  REQUIRE(td.has_value());
  REQUIRE(td->cmd == "large");
  REQUIRE(td->expected == "a\n\nb\n");
}

TEST_CASE(
    "TestDataReader keeps single separator inside multiline expected output") {
  std::istringstream input("large\n----\n----\na\n----\nb\n----\n----\n\n");
  datadriven::internal::TestDataReader reader("<string>", input, false);
  auto td = reader.Next();
  REQUIRE(td.has_value());
  REQUIRE(td->expected == "a\n----\nb\n");
}

TEST_CASE("TestDataReader rejects unterminated multiline expected output") {
  std::istringstream input("large\n----\n----\n");
  datadriven::internal::TestDataReader reader("<string>", input, false);

  try {
    (void)reader.Next();
    FAIL("expected unterminated multiline expected output to throw");
  } catch (const std::runtime_error &err) {
    REQUIRE(std::string(err.what()) ==
            "unterminated double ---- separator section");
  }
}

TEST_CASE("TestDataReader skips comments and joins directive continuations") {
  std::istringstream input("# comment\ncmd \\\n  a=b\n----\nok\n\n");
  datadriven::internal::TestDataReader reader("<string>", input, false);
  auto td = reader.Next();
  REQUIRE(td.has_value());
  REQUIRE(td->cmd == "cmd");
  REQUIRE(td->cmd_args[0].ToString() == "a=b");
}

TEST_CASE("TestDataReader strips trailing continuation backslash at EOF") {
  // Directive line ending with '\' but no following line: the '\' must be
  // dropped, not surfaced as a spurious argument key.
  std::istringstream input("cmd \\\n");
  datadriven::internal::TestDataReader reader("<string>", input, false);
  auto td = reader.Next();
  REQUIRE(td.has_value());
  REQUIRE(td->cmd == "cmd");
  REQUIRE(td->cmd_args.empty());
}

TEST_CASE("TestDataReader strips trailing continuation backslash with no "
          "newline at EOF") {
  std::istringstream input("cmd \\");
  datadriven::internal::TestDataReader reader("<string>", input, false);
  auto td = reader.Next();
  REQUIRE(td.has_value());
  REQUIRE(td->cmd == "cmd");
  REQUIRE(td->cmd_args.empty());
}
