cmake_minimum_required(VERSION 3.16)
project(pixelmatch-cpp17 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PIXELMATCH_BUILD_TESTS "Enable building tests" OFF)

# stb libraries for tests and image_utils. Not used in pixelmatch-cpp17 itself.
add_library(pixelmatch_third_party_stb_image STATIC third_party/stb/stb_image.cpp)
target_include_directories(pixelmatch_third_party_stb_image PUBLIC third_party)
target_compile_options(pixelmatch_third_party_stb_image PRIVATE -Wno-unused-function -Wno-self-assign)

add_library(pixelmatch_third_party_stb_image_write STATIC third_party/stb/stb_image_write.cpp)
target_include_directories(pixelmatch_third_party_stb_image_write PUBLIC third_party)
target_compile_options(pixelmatch_third_party_stb_image_write PRIVATE -Wno-unused-function -Wno-self-assign)

# Main library
add_library(pixelmatch-cpp17 src/pixelmatch/pixelmatch.cc)
target_include_directories(pixelmatch-cpp17 PUBLIC src)

# image_utils helper library (uses stb to load and save images)
add_library(image_utils src/pixelmatch/image_utils.cc)
target_include_directories(image_utils PUBLIC src)
target_link_libraries(image_utils PUBLIC pixelmatch-cpp17 pixelmatch_third_party_stb_image pixelmatch_third_party_stb_image_write)

if(PIXELMATCH_BUILD_TESTS)
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()
add_library(test_base INTERFACE)
target_link_libraries(test_base INTERFACE GTest::gtest_main GTest::gmock_main m)

add_executable(pixelmatch_tests tests/pixelmatch_tests.cc)
target_link_libraries(pixelmatch_tests PRIVATE test_base pixelmatch-cpp17 image_utils)
add_test(NAME pixelmatch_tests COMMAND pixelmatch_tests)
set_tests_properties(pixelmatch_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

add_executable(image_utils_tests tests/image_utils_tests.cc)
target_link_libraries(image_utils_tests PRIVATE test_base image_utils)
add_test(NAME image_utils_tests COMMAND image_utils_tests)
set_tests_properties(image_utils_tests PROPERTIES WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

endif() # PIXELMATCH_BUILD_TESTS
