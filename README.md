jot-lib
=======

C++ libraries for 3D graphics, especially NPR

What is this?
-------------

This is a fork of the jot-lib library developed at Brown University, Princeton
University, and the University of Michigan and released on Google code [1].

The aim is to get things running on modern compilers, in a 64-bit world, and
to remove the cruft of code written long ago. I'm no expert on the theory, so
don't expect me to go fixing bugs in any of the algorithms any time soon.

I am currently targeting Linux distributions, but I'm also aiming to make the
code more portable where possible.

Dependencies
------------

You will need a C++ compiler, cmake, and development headers for some external
libraries:

 * OpenGL
 * GLUT
 * GLEW
 * libpng
 * Coin3D (optional; used by iv2sm for conversions. Another implementation ofi
           Open Inventor may also work.)

How to build
------------

First run `cmake` to check for dependencies and set up compilation flags. Then
run `make` to build everything.

    $ mkdir build
    $ cmake ..
    $ make -j$NUM_PROCS

[1] http://code.google.com/p/jot-lib/

