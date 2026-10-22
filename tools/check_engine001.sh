#!/usr/bin/env bash
set -euo pipefail
# Fallback for a Linux host with GCC but no CMake/SDL3. It does not build the GUI.
cd "$(dirname "$0")/.."
mkdir -p build/direct
sources=(src/engine001/{json,world,scenes,sampling,facade_treatment,house_geometry,camera,render,environment_art,outdoor,telemetry,options}.cpp)
fixture=docs/architecture/ENGINE001_SCENE_FIXTURE_v0.1.json
source_id=$(git rev-parse HEAD)
if [[ -n $(git status --porcelain) ]]; then source_id+="-dirty"; fi
flags=(-std=c++20 -Wall -Wextra -Wpedantic -Werror -Isrc/engine001 "-DENGINE001_BUILD_ID=\"${source_id}\"")
g++ --version
g++ "${flags[@]}" -O2 -DNDEBUG "${sources[@]}" tests/engine001/core_tests.cpp -o build/direct/engine001_tests
build/direct/engine001_tests "$fixture"
g++ "${flags[@]}" -O2 -DNDEBUG "${sources[@]}" src/engine001/headless.cpp -o build/direct/wildsignal_engine001_headless
python3 tests/engine001/export_test.py build/direct/wildsignal_engine001_headless "$fixture"
g++ "${flags[@]}" -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "${sources[@]}" tests/engine001/core_tests.cpp -o build/direct/engine001_tests_asan
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" build/direct/engine001_tests_asan "$fixture"

g++ "${flags[@]}" -O2 -DNDEBUG "${sources[@]}" tests/engine001/render001_{tests,alloc}.cpp -o build/direct/render001_tests
build/direct/render001_tests .
g++ "${flags[@]}" -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "${sources[@]}" tests/engine001/render001_{tests,alloc}.cpp -o build/direct/render001_tests_asan
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" build/direct/render001_tests_asan .

g++ "${flags[@]}" -O2 -DNDEBUG "${sources[@]}" tests/engine001/sampling_tests.cpp tests/engine001/render001_alloc.cpp -o build/direct/sampling_tests
build/direct/sampling_tests
python3 tests/engine001/sampling_import_test.py build/direct/wildsignal_engine001_headless .
python3 tests/engine001/facade_test.py build/direct/wildsignal_engine001_headless
python3 tests/engine001/vis003_test.py build/direct/wildsignal_engine001_headless .
g++ "${flags[@]}" -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "${sources[@]}" tests/engine001/sampling_tests.cpp tests/engine001/render001_alloc.cpp -o build/direct/sampling_tests_asan
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" build/direct/sampling_tests_asan

g++ "${flags[@]}" -O2 -DNDEBUG "${sources[@]}" tests/engine001/geometry_tests.cpp tests/engine001/render001_alloc.cpp -o build/direct/geometry_tests
build/direct/geometry_tests
g++ "${flags[@]}" -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer "${sources[@]}" tests/engine001/geometry_tests.cpp tests/engine001/render001_alloc.cpp -o build/direct/geometry_tests_asan
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=1}" build/direct/geometry_tests_asan

python3 tests/engine001/geometry_test.py build/direct/wildsignal_engine001_headless .
