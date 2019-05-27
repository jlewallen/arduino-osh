include(Arduino)

enable_arduino_toolchain()

enable_language(ASM)

if(TARGET_M4)
  enable_m4()
endif()

if(TARGET_M0)
  enable_m0()
endif()

file(GLOB osh_sources ../src/*.cpp ../src/*.c ../src/*.s ../src/*.h ../src/segger/*.c)

file(GLOB three_sources three.cpp)
add_arduino_library(osh-three "${osh_sources};${three_sources}")
target_compile_options(osh-three PRIVATE -Wall -Werror)
target_include_directories(osh-three PUBLIC ./ ../src)
add_arduino_firmware(osh-three)

file(GLOB reuse_sources reuse.cpp)
add_arduino_library(osh-reuse "${osh_sources};${reuse_sources}")
target_compile_options(osh-reuse PRIVATE -Wall -Werror)
target_include_directories(osh-reuse PUBLIC ./ ../src)
add_arduino_firmware(osh-reuse)

file(GLOB extras_sources extras.cpp)
add_arduino_library(osh-extras "${osh_sources};${extras_sources}")
target_compile_options(osh-extras PRIVATE -Wall -Werror)
target_include_directories(osh-extras PUBLIC ./ ../src)
add_arduino_firmware(osh-extras)
