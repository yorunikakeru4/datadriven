#include <catch2/catch_test_macros.hpp>
#include <datadriven/datadriven.hpp>
#include <locale>

namespace {

class CommaDecimalPunctuation : public std::numpunct<char> {
protected:
  char do_decimal_point() const override { return ','; }
};

class ScopedLocale {
public:
  explicit ScopedLocale(std::locale locale) : previous_(std::locale()) {
    std::locale::global(locale);
  }

  ~ScopedLocale() { std::locale::global(previous_); }

private:
  std::locale previous_;
};

} // namespace

TEST_CASE("ScanArg scans scalar values") {
  datadriven::RunTestFromString(
      R"(
scalar i=16 u=32 b=true f=1.5 s=hello
----
ok
)",
      [](const datadriven::TestData &d) {
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

TEST_CASE("ScanArg parses floating point values independent of locale") {
  ScopedLocale locale(
      std::locale(std::locale::classic(), new CommaDecimalPunctuation));

  datadriven::RunTestFromString(
      R"(
scalar f=1.5
----
ok
)",
      [](const datadriven::TestData &d) {
        double f = 0;
        d.ScanArg("f", f);
        REQUIRE(f == 1.5);
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
      [](const datadriven::TestData &d) {
        std::vector<int> vals;
        d.ScanArg("vals", vals);
        REQUIRE(vals == std::vector<int>{1, 2, 3});
        return "ok";
      });
}
