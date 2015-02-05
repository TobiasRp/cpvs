cmake_minimum_required (VERSION 2.6)
project (cpvs)

set (CPVS_VERSION_MAJOR_0)
set (CPVS_VERSION_MINOR_1)

# Enable C++14
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake-modules)

add_definitions(-DPRINT_PROGRESS)
add_definitions(-DPRINT_CPVS_SIZE)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Assimp REQUIRED)
find_package(AntTweakBar REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})
include_directories(${ANT_TWEAK_BAR_INCLUDE_PATH})

include_directories("glm/")

file(GLOB SRC_DIR_FILES "src/*.h" "src/*.cpp")
file(GLOB TEST_CPP_FILES "test/*.cpp")

add_executable(cpvs ${SRC_DIR_FILES})

# create a static library for unit testing
add_library(cpvs_lib STATIC ${SRC_DIR_FILES})

target_link_libraries(cpvs ${OPENGL_LIBRARIES})
target_link_libraries(cpvs ${GLEW_LIBRARIES})
target_link_libraries(cpvs ${GLFW_STATIC_LIBRARIES})
target_link_libraries(cpvs ${ASSIMP_LIBRARIES})
target_link_libraries(cpvs ${ANT_TWEAK_BAR_LIBRARY})

# Testing with GTest
ADD_SUBDIRECTORY(gtest-1.7.0)
enable_testing()
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})
include_directories("src/")

# Unit Tests
add_executable(runUnitTests ${TEST_CPP_FILES})
target_link_libraries(runUnitTests gtest gtest_main cpvs_lib)
target_link_libraries(runUnitTests ${OPENGL_LIBRARIES})
target_link_libraries(runUnitTests ${GLEW_LIBRARIES})
target_link_libraries(runUnitTests ${GLFW_STATIC_LIBRARIES})
add_test(runUnitTests runUnitTests)

