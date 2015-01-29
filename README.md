cpvs
====

Implementation of compact precomputed voxelized shadows

## General requirements ##

OpenGL 4.4 is needed



## Building CPVS ##

Use CMake!

Needed libraries:
	OpenGL
	GLEW, GLFW3, Assimp, AntTweakBar

-> CMake should find them!
AntTweakBar also needs it's shared library in 'LD_LIBRARY_PATH'

GLM is included in the repository (it is header-only!)

For unit testing GTest is included in the repository

## Running CPVS ##

Except for the default scene file 3D-models/scene files are not included in the repo!

Simply specify the scene to use as an argument.
(Default scene is "../scenes/plane.obj", which can be changed in src/main.cpp)
