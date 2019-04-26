project(ARDUINO_PROJECTS)
cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH $ENV{CMAKE_MODULE_PATH})

include(Arduino)

enable_arduino_toolchain()

# enable_m4()

enable_language(ASM)

enable_small_bootloader()

file(GLOB sources *.cpp)

add_arduino_library(osh-test "${sources}")

add_arduino_firmware(osh-test)
