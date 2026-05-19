cmake_minimum_required(VERSION 3.20)
project(datadriven_cpp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.6.0
)
FetchContent_MakeAvailable(Catch2)

add_library(datadriven
  src/line_parser.cpp
  src/test_data_reader.cpp
  src/runner.cpp
  src/walk.cpp
)
target_include_directories(datadriven PUBLIC include PRIVATE src)

add_executable(datadriven-clear tools/datadriven-clear.cpp)
target_link_libraries(datadriven-clear PRIVATE datadriven)

add_executable(datadriven_tests
  tests/parse_line_test.cpp
  tests/reader_test.cpp
  tests/runner_test.cpp
  tests/rewrite_test.cpp
  tests/scan_args_test.cpp
  tests/subtest_test.cpp
  tests/walk_test.cpp
)
target_link_libraries(datadriven_tests PRIVATE datadriven Catch2::Catch2WithMain)
target_include_directories(datadriven_tests PRIVATE src)

include(CTest)
include(Catch)
catch_discover_tests(datadriven_tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
