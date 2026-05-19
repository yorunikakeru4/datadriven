build-dir := "build"

configure:
    rtk cmake -S . -B {{build-dir}} -DCMAKE_BUILD_TYPE=Debug

build: configure
    rtk cmake --build {{build-dir}} --parallel

test: build
    rtk ctest --test-dir {{build-dir}} --output-on-failure

run-tests: build
    rtk ./{{build-dir}}/datadriven_tests

clear file: build
    rtk ./{{build-dir}}/datadriven-clear {{file}}

format:
    find include src tests tools -name '*.hpp' -o -name '*.cpp' | xargs clang-format -i

release:
    rtk cmake -S . -B build-release -DCMAKE_BUILD_TYPE=Release
    rtk cmake --build build-release --parallel
    rtk ctest --test-dir build-release --output-on-failure

clean:
    rm -rf {{build-dir}} build-release
