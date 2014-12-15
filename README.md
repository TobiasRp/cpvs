cpvs
====

Implementation of compact precomputed voxelized shadows


/// Building CPVS ///

Use CMake!

Needed libraries:
	GLEW, GLFW3, Assimp

CMake should find them

For unit testing GTest is included in the repository

/// Running CPVS ///
3D-models/scene files are not included in the repo!

Simply specify the scene to use as an argument.
(Default scene is "../scenes/sibenik/sibenik.obj", which can be changed in src/main.cpp)
