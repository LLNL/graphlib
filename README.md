## GraphLib, Version 3.0.

Copyright (c) 2007-2016, Lawrence Livermore National Security, LLC.
Written by Martin Schulz, Dorian Arnold, and Gregory L. Lee
Contact email: schulzm@llnl.gov, LLNL-CODE-624053
All rights reserved - please read information in [LICENCSE](https://github.com/LLNL/graphlib/blob/master/LICENSE)


### Overview
Library to create, manipulate, and export graphs

Martin Schulz, LLNL, 2005-2007
Additions made by Dorian Arnold (summer 2006)
Additions make by Todd Gamblin (summer 2011)
Modifications make by Gregory L. Lee, LLNL

This library is used in several LLNL projects, including STAT
(the Stack Trace Analysis Tool for scalable debugging) and some
modules in P^nMPI (a tool MPI tool infrastructure). It can
also be used standalone for creating and manipulating graphs,
but its API is primarily tuned to support these other
projects.

### Compilation

You will need CMake to build Graphlib.  To build Graphlib on
BlueGene/P, you will need CMake version 2.8.3 or later.

The recommended way to build is in an architecture-specific
subdirectory of the source directory, e.g.:

  mkdir x86_64 && cd x86_64
  cmake -D CMAKE_INSTALL_PREFIX=/path/to/install/location ..
  make

You should define CMAKE_INSTALL_PREFIX as above to be the location
where you want libraries and headers to be installed.  '..' above
is the path to the graphlib source.  You can actually build in *any*
directory, but we use '..' here because we're building in a
subdirectory.

To build on BlueGene/P, you will need to be a little more specific.
You have to give CMake a toolchain file that tells it which
cross-compilers to use.  This is included with the distribution,
in the toolchain-files subdirectory:

  mkdir bgp && cd bgp
  cmake \
    -D CMAKE_INSTALL_PREFIX=/path/to/install/location \
    -D CMAKE_TOOLCHAIN_FILE=../toolchain-files/BlueGeneP-gnu.cmake \
    ..
  make

This will create liblnlgraph.a, a demo program called
"graphdemo", and small utility called "grmerge".

Running graphdemo will test the most common routines and
create a set of graphs in DOT and GML format that can then
be viewed using external tools (e.g., dotty or yed).

grmerge merges an arbitrary number of graphs stored in GraphLib's
internal format and then either saves the merged graph or exports
it as GML or DOT files. For more information run "grmerge" without
arguments.

### INSTALLATION (optional)

To install graphlib, simply run 'make install' in your build
directory after you have successfully compiled the binaries.


### Finding graphlib from other CMake projects

If graphlib is in a system directory, no environment variables need
to be set.  If not, set the graphlib_DIR environment variable to the
location where you installed graphlib.

Put this in your CMakeLists.txt:

  find_package(graphlib)

If graphlib is found successfully, you can use this variables to
refer to the autodetected graphlib include directory:

  include_directories(${graphlib_INCLUDE_PATH})

Then you can link graphlib with your executable like this:

  target_link_libraries(mytarget lnlgraph)

That's all!


### General Usage

- include "graphlib.h" into source code
- link with -llnlgraph (or as above if using CMake)

The API is described in the header file.


