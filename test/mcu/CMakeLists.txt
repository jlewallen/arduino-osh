include(Arduino)

enable_arduino_toolchain()

enable_language(ASM)

enable_m4()

file(GLOB osh_sources ../../src/*.cpp ../../src/*.c ../../src/*.h)

file(GLOB test_sources *.cpp)
add_arduino_library(osh-test "${test_sources};${three_sources}")
target_include_directories(osh-test PUBLIC ./ ../../src)
add_arduino_firmware(osh-test)
