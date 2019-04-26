project(ARDUINO_PROJECTS)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH $ENV{CMAKE_MODULE_PATH})

include(Arduino)

enable_arduino_toolchain()

enable_language(ASM)

file(GLOB sources *.cpp)

enable_m4()
add_arduino_library(osh-test "${sources}")
add_arduino_firmware(osh-test)
