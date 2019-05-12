include(Arduino)

enable_arduino_toolchain()

enable_language(ASM)

enable_m4()

file(GLOB osh_sources ../src/*.cpp ../src/*.c ../src/*.h)

file(GLOB three_sources *.cpp)
add_arduino_library(osh-three "${osh_sources};${three_sources}")
target_include_directories(osh-three PUBLIC ./ ../src)
add_arduino_firmware(osh-three)
