#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>

TEST_CASE("ScanArg scans scalar values") {
  datadriven::RunTestFromString(
      R"(
scalar i=16 u=32 b=true f=1.5 s=hello
----
ok
)",
      [](const datadriven::TestData& d) {
        int i = 0;
        unsigned u = 0;
        bool b = false;
        double f = 0;
        std::string s;
        d.ScanArg("i", i);
        d.ScanArg("u", u);
        d.ScanArg("b", b);
        d.ScanArg("f", f);
        d.ScanArg("s", s);
        REQUIRE(i == 16);
        REQUIRE(u == 32);
        REQUIRE(b);
        REQUIRE(f == 1.5);
        REQUIRE(s == "hello");
        return "ok";
      });
}

TEST_CASE("ScanArg scans vector values") {
  datadriven::RunTestFromString(
      R"(
vector vals=(1, 2, 3)
----
ok
)",
      [](const datadriven::TestData& d) {
        std::vector<int> vals;
        d.ScanArg("vals", vals);
        REQUIRE(vals == std::vector<int>{1, 2, 3});
        return "ok";
      });
}
