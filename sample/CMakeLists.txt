# Headless validation consumer for the OpenCV platform docs (opencv-mac.md).
#
# Configure against the guide's install (from the repo root):
#   cmake -S sample -B sample/build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$PWD/dist/opencv/arm64-macos"
#
# Expected run output ends with "Threshold check: exact" and exit code 0
# (see opencv-mac.md, section 3).

cmake_minimum_required (VERSION 3.16)

project (hello_opencv LANGUAGES CXX)

# OpenCV 5.0 builds its libraries as C++17 by default and its headers expect
# a C++17 consumer.
set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# OpenCV's package config supports component selection (unlike OCCT's,
# whose sample must import every module): this imports the three modules
# the sample calls plus their dependency closure.
find_package (OpenCV REQUIRED COMPONENTS core imgproc imgcodecs)

add_executable (hello_opencv hello_opencv.cpp)

target_link_libraries (hello_opencv PRIVATE ${OpenCV_LIBS})
