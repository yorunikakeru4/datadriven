cmake_minimum_required(VERSION 3.20)
project(
  datadriven_cpp
  VERSION 0.1.0
  LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(PROJECT_IS_TOP_LEVEL)
  include(FetchContent)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.6.0
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(Catch2)
endif()

function(datadriven_enable_warnings target)
  target_compile_options(
    ${target} PRIVATE $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra
                      -Wpedantic> $<$<CXX_COMPILER_ID:MSVC>:/W4>)
endfunction()

add_library(datadriven src/line_parser.cpp src/platform.cpp
                       src/test_data_reader.cpp src/runner.cpp src/walk.cpp)
target_include_directories(
  datadriven
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE src)
target_compile_features(datadriven PUBLIC cxx_std_20)
datadriven_enable_warnings(datadriven)

add_executable(datadriven-clear tools/datadriven-clear.cpp)
target_link_libraries(datadriven-clear PRIVATE datadriven)
datadriven_enable_warnings(datadriven-clear)

if(PROJECT_IS_TOP_LEVEL)
  add_executable(
    datadriven_tests
    tests/parse_line_test.cpp
    tests/platform_test.cpp
    tests/reader_test.cpp
    tests/runner_test.cpp
    tests/rewrite_test.cpp
    tests/scan_args_test.cpp
    tests/subtest_test.cpp
    tests/walk_test.cpp)
  target_link_libraries(datadriven_tests PRIVATE datadriven
                                                 Catch2::Catch2WithMain)
  target_include_directories(datadriven_tests PRIVATE src)
  target_compile_definitions(
    datadriven_tests
    PRIVATE DATADRIVEN_TESTDATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/tests/testdata")
  datadriven_enable_warnings(datadriven_tests)

  include(CTest)
  include(Catch)
  catch_discover_tests(datadriven_tests WORKING_DIRECTORY
                       ${CMAKE_CURRENT_SOURCE_DIR})
endif()

install(
  TARGETS datadriven datadriven-clear
  EXPORT datadrivenTargets
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/datadrivenConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion)
configure_package_config_file(
  cmake/datadrivenConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/datadrivenConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/datadriven)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/datadrivenConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/datadrivenConfigVersion.cmake"
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/datadriven)
install(
  EXPORT datadrivenTargets
  NAMESPACE datadriven::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/datadriven)
