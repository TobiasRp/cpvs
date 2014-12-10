cmake_minimum_required (VERSION 2.6)
project (cpvs)

set (CPVS_VERSION_MAJOR_0)
set (CPVS_VERSION_MINOR_1)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(Assimp REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_search_module(GLFW REQUIRED glfw3)

include_directories(${OPENGL_INCLUDE_DIRS})
include_directories(${GLEW_INCLUDE_DIRS})
include_directories(${GLFW_INCLUDE_DIRS})
include_directories(${ASSIMP_INCLUDE_DIRS})

# GLM is assumed to be above in the path
include_directories("../glm/")

file(GLOB SRC_DIR_FILES "src/*.h" "src/*.cpp")

add_executable(cpvs ${SRC_DIR_FILES})

target_link_libraries(cpvs ${OPENGL_LIBRARIES})
target_link_libraries(cpvs ${GLEW_LIBRARIES})
target_link_libraries(cpvs ${GLFW_STATIC_LIBRARIES})
target_link_libraries(cpvs ${ASSIMP_LIBRARIES})
