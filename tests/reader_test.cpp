#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

#include "test_data_reader.hpp"

#include <sstream>

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

TEST_CASE("TestDataReader skips comments and joins directive continuations") {
  std::istringstream input("# comment\ncmd \\\n  a=b\n----\nok\n\n");
  datadriven::internal::TestDataReader reader("<string>", input, false);
  auto td = reader.Next();
  REQUIRE(td.has_value());
  REQUIRE(td->cmd == "cmd");
  REQUIRE(td->cmd_args[0].ToString() == "a=b");
}
