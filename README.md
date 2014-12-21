cpvs
====

Implementation of compact precomputed voxelized shadows

/// General requirements ///

OpenGL 4.4 is needed


/// Building CPVS ///

Use CMake!

Needed libraries:
	GLEW, GLFW3, Assimp

-> CMake should find them

GLM is included in the repository (it is header-only!)

For unit testing GTest is included in the repository

/// Running CPVS ///

3D-models/scene files are not included in the repo!

Simply specify the scene to use as an argument.
(Default scene is "../scenes/plane.obj", which can be changed in src/main.cpp)
