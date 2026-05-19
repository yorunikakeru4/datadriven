#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

TEST_CASE("public header compiles") {
  datadriven::CmdArg arg;
  arg.key = "x";
  REQUIRE(arg.key == "x");
}

TEST_CASE("CmdArg renders like Go datadriven") {
  REQUIRE(datadriven::CmdArg{"flag", {}}.ToString() == "flag");
  REQUIRE(datadriven::CmdArg{"a", {"b"}}.ToString() == "a=b");
  REQUIRE(datadriven::CmdArg{"c", {"1", "2", "3"}}.ToString() == "c=(1, 2, 3)");
}

TEST_CASE("TestData renders full command and input") {
  datadriven::TestData td;
  td.cmd = "cmd";
  td.cmd_args = {{"a", {"b"}}, {"flag", {}}};
  REQUIRE(td.FullCmd() == "cmd a=b flag");
  REQUIRE(td.ToString() == "cmd a=b flag\n");

  td.input = "hello";
  REQUIRE(td.ToString() == "cmd a=b flag\nhello\n");
}

TEST_CASE("ParseLine parses empty and arguments") {
  auto [empty_cmd, empty_args] = datadriven::ParseLine("   ");
  REQUIRE(empty_cmd.empty());
  REQUIRE(empty_args.empty());

  auto [cmd, args] = datadriven::ParseLine("xx a=b a=c");
  REQUIRE(cmd == "xx");
  REQUIRE(args.size() == 2);
  REQUIRE(args[0].key == "a");
  REQUIRE(args[0].vals == std::vector<std::string>{"b"});
  REQUIRE(args[1].key == "a");
  REQUIRE(args[1].vals == std::vector<std::string>{"c"});
}

TEST_CASE("ParseLine parses list args and nested parens") {
  auto [cmd, args] =
      datadriven::ParseLine("cmd exprs=(a + (b + c), d + f) flag x=");
  REQUIRE(cmd == "cmd");
  REQUIRE(args.size() == 3);
  REQUIRE(args[0].key == "exprs");
  REQUIRE(args[0].vals == std::vector<std::string>{"a + (b + c)", "d + f"});
  REQUIRE(args[1].key == "flag");
  REQUIRE(args[1].vals.empty());
  REQUIRE(args[2].key == "x");
  REQUIRE(args[2].vals == std::vector<std::string>{""});
}

TEST_CASE("ParseLine reports malformed input with column") {
  try {
    (void)datadriven::ParseLine("xx =");
    FAIL("ParseLine accepted malformed directive");
  } catch (const std::runtime_error &e) {
    REQUIRE(std::string(e.what()) ==
            "cannot parse directive at column 4: xx =");
  }
}
