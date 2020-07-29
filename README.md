# *vt* => virtual transport

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/e21fba68df8947ecb9a9c51b5e159e56)](https://www.codacy.com/gh/DARMA-tasking/vt?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=DARMA-tasking/vt&amp;utm_campaign=Badge_Grade)
[![codecov](https://codecov.io/gh/DARMA-tasking/vt/branch/develop/graph/badge.svg)](https://codecov.io/gh/DARMA-tasking/vt)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-orange.svg)](https://opensource.org/licenses/BSD-3-Clause)
![](https://github.com/DARMA-tasking/vt/workflows/Docker%20Image%20CI/badge.svg)
![gcc-5, ubuntu, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(gcc-5,%20ubuntu,%20mpich)/badge.svg?branch=develop)
![gcc-6, ubuntu, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(gcc-6,%20ubuntu,%20mpich)/badge.svg?branch=develop)
![gcc-7, ubuntu, mpich, trace runtime, LB](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(gcc-7,%20ubuntu,%20mpich,%20trace%20runtime,%20LB)/badge.svg?branch=develop)
![gcc-8, ubuntu, mpich, address sanitizer](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(gcc-8,%20ubuntu,%20mpich,%20address%20sanitizer)/badge.svg?branch=develop)
![gcc-9, ubuntu, mpich, zoltan](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(gcc-9,%20ubuntu,%20mpich,%20zoltan)/badge.svg?branch=develop)
![gcc-10, ubuntu, openmpi, no LB](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(gcc-10,%20ubuntu,%20openmpi,%20no%20LB)/badge.svg?branch=develop)
![clang-3.9, ubuntu, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(clang-3.9,%20ubuntu,%20mpich)/badge.svg?branch=develop)
![clang-5, ubuntu, mpich, trace](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(clang-5.0,%20ubuntu,%20mpich,%20trace)/badge.svg?branch=develop)
![apple clang, macosx, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(clang-8,%20macosx,%20mpich)/badge.svg?branch=develop)
![intel 18.03, ubuntu, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(intel%2018.03,%20ubuntu,%20mpich)/badge.svg?branch=develop)
![nvidia cuda 10.1, ubuntu, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(nvidia%20cuda%2010.1,%20ubuntu,%20mpich)/badge.svg?branch=develop)
![nvidia cuda 11.0, ubuntu, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(nvidia%20cuda%2011.0,%20ubuntu,%20mpich)/badge.svg?branch=develop)
![Build Documentation](https://github.com/DARMA-tasking/vt/workflows/Build%20Documentation/badge.svg?branch=develop)

## Introduction : What is *vt*?

*vt* is an active messaging layer that utilizes C++ object virtualization to
manage virtual endpoints with automatic location management. *vt* is directly
built on top of MPI to provide efficient portability across different machine
architectures. Empowered with virtualization, **vt** can automatically perform
dynamic load balancing to schedule scientific applications across diverse
platforms with minimal user input.

*vt* abstracts the concept of a `node`/`rank`/`worker`/`thread` so a program can
be written in terms of virtual entities that are location independent. Thus,
they can be automatically migrated and thereby executed on varying hardware
resources without explicit programmer mapping, location, and communication
management.

## Read the documentation

To learn *vt*, read the [full
documentation](https://darma-tasking.github.io/docs/html/index.html) that is
automatically generated whenever a push occurs to "develop". It includes a
walk-though of the tutorial and a overview of the components that make up a *vt*
runtime.

## Building

[Learn how to build](https://darma-tasking.github.io/docs/html/vt-build.html)
*vt* with `cmake` or `docker`.
