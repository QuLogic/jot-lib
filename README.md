jot-lib
=======

C++ libraries for 3D graphics, especially NPR

What is this?
-------------

This is a fork of the jot-lib library developed at Brown University, Princeton
University, and the University of Michigan and released on Google code [1].

The aim is to get things running on modern compilers, in a 64-bit world, and
to remove the cruft of code written long ago. I'm no expert on the theory, so
don't expect me to go fixing bugs in any of the algorithms.

I am currently targetting Linux distributions, but I'm also aiming to make the
code more portable where possible.

How to build
------------

First run `cmake` to check for dependencies and set up compilation flags. Then
run `make` to build everything.

    $ mkdir build
    $ cmake ..
    $ make -j$NUM_PROCS

**Note:** The code does not presently compile in a 64-bit world. Make sure to
install 32-bit development libraries, as `cmake` will force a 32-bit compile.

[1] http://code.google.com/p/jot-lib/

