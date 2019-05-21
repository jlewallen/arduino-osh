include(Arduino)

enable_arduino_toolchain()

enable_language(ASM)

if(TARGET_M4)
  enable_m4()
endif()

if(TARGET_M0)
  enable_m0()
endif()

# add_definitions(-DOSH_MTB)
# add_compile_options("-fstack-usage")

file(GLOB osh_sources ../../src/*.cpp ../../src/*.s ../../src/*.c ../../src/*.h)

file(GLOB test_sources *.cpp)
add_arduino_library(osh-test "${test_sources};${osh_sources}")
target_include_directories(osh-test PUBLIC ./ ../../src)
add_arduino_firmware(osh-test)
