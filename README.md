cpvs
====

Implementation of compact precomputed voxelized shadows

## General requirements ##

The code has only been tested on Linux using OpenGL 4.4 and gcc 4.9.1 / clang 3.5.

In general at least OpenGL 4.2 (allthough shader version specifiers must be changed) and C++14 support are needed.

## Building CPVS ##

Use CMake!

Needed libraries:
 * OpenGL
 * GLEW
 * GLFW3
 * Assimp
 * AntTweakBar

-> CMake should find them!
(A custom cmake module for AntTweakBar is included which should work as expected)

 * AntTweakBar also needs it's shared library in 'LD_LIBRARY_PATH'

 * GLM is included in the repository (it is header-only!)

 * For unit testing GTest is included in the repository

## Running CPVS ##

Run cpvs with --help to see a full overview of the possible command line arguments.

Except for the default scene file 3D-models/scene files are not included in the repo!

Simply specify the scene to use as an argument.
(Default scene is "../scenes/plane.obj", which can be changed in src/main.cpp)


## Feature overview ##

 * SVO is created from a shadow map
 * The shadow SVO is transformed to a DAG and compressed
 * 64-bit 'flat' leafmasks can be used, i.e. level 3 stores 1x1x8 nodes and the last levels encode 8x8x1 voxels
 * Light space transformation is calculated from the scene boundaries to reduce aliasing and artefacts
 * The precomputed shadow uses a top-level grid to store large shadows
