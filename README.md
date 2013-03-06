miWorleyNoise
=============

Worley noise / Voronoi noise shader for mental ray.

`Voronoi` contains a prototype implementation in Haskell. It is somewhat slow.

`shader` contains the mental ray implementation, including a README on how to compile and use the shader and a makefile for OS X.

Known Issues (all with the mentalray implementation)
-----------
- Specifying a color for the gap between cells does not currently work (it is disabled due to wrong behavior)
- For unknown reasons, there are sometimes artifacts in the mentalray shader. 
	Need to check whether f2 and f3 contain values that actually make sense. That could at least explain weirdness wrt gaps.

To-Do
-----
- Optional noise in the distances, to allow for a more natural look for e.g cracked earth
- Recursive/multi-level implementation

Contributors
----------
Code: Marcel Ruegenberg
Icons and some improvement suggestions by Andrew Hazelden

See also [appliedcuriosity.eu/portfolio/project/miworleynoise](http://appliedcuriosity.eu/portfolio/project/miworleynoise).