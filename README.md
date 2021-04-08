# *vt* => virtual transport

[![Codacy Badge](https://api.codacy.com/project/badge/Grade/e21fba68df8947ecb9a9c51b5e159e56)](https://www.codacy.com/gh/DARMA-tasking/vt?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=DARMA-tasking/vt&amp;utm_campaign=Badge_Grade)
[![codecov](https://codecov.io/gh/DARMA-tasking/vt/branch/develop/graph/badge.svg)](https://codecov.io/gh/DARMA-tasking/vt)
[![License](https://img.shields.io/badge/License-BSD%203--Clause-orange.svg)](https://opensource.org/licenses/BSD-3-Clause)
![](https://github.com/DARMA-tasking/vt/workflows/Docker%20Image%20CI/badge.svg)
[![gcc-5, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(gcc-5%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(gcc-5%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=2&branchName=develop)
[![gcc-6, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(gcc-6%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(gcc-6%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=7&branchName=develop)
[![gcc-7, ubuntu, mpich, trace runtime, LB](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(gcc-7%2C%20ubuntu%2C%20mpich%2C%20trace%20runtime%2C%20LB)?branchName=develop&Label=(gcc-7%2C%20ubuntu%2C%20mpich%2C%20trace%20runtime%2C%20LB))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=8&branchName=develop)
[![gcc-8, ubuntu, mpich, address sanitizer](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(gcc-8%2C%20ubuntu%2C%20mpich%2C%20address%20sanitizer)?branchName=develop&Label=(gcc-8%2C%20ubuntu%2C%20mpich%2C%20address%20sanitizer))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=9&branchName=develop)
[![gcc-9, ubuntu, mpich, zoltan](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(gcc-9%2C%20ubuntu%2C%20mpich%2C%20zoltan)?branchName=develop&Label=(gcc-9%2C%20ubuntu%2C%20mpich%2C%20zoltan))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=10&branchName=develop)
[![gcc-10, ubuntu, openmpi, no LB](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(gcc-10%2C%20ubuntu%2C%20openmpi%2C%20no%20LB)?branchName=develop&Label=(gcc-10%2C%20ubuntu%2C%20openmpi%2C%20no%20LB))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=4&branchName=develop)
[![clang-3.9, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(clang-3.9%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(clang-3.9%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=3&branchName=develop)
[![clang-5, ubuntu, mpich, trace](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(clang-5.0%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(clang-5.0%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=5&branchName=develop)
[![clang-9, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(clang-9%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(clang-9.0%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build?definitionId=22&branchName=develop)
[![clang-10, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(clang-10%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(clang-10.0%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build?definitionId=21&branchName=develop)
[![intel 18.03, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(intel%2018.03%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(intel-18.03%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=13&branchName=develop)
[![intel 19, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(intel%2019%2C%20ubuntu-mpich)?branchName=develop&Label=(intel-19%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build?definitionId=18&branchName=develop)
[![nvidia cuda 10.1, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(nvidia%20cuda%2010.1%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(nvidia%20cuda%2010.1%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=11&branchName=develop)
[![nvidia cuda 11.0, ubuntu, mpich](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20(nvidia%20cuda%2011.0%2C%20ubuntu%2C%20mpich)?branchName=develop&Label=(nvidia%20cuda%2011.0%2C%20ubuntu%2C%20mpich))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=12&branchName=develop)
[![gcc-10, ubuntu, openmpi, no LB, spack-package](https://dev.azure.com/DARMA-tasking/DARMA/_apis/build/status/PR%20tests%20spack-package%20(gcc-10%2C%20ubuntu%2C%20openmpi%2C%20no%20LB%2C%20spack-package)?branchName=develop&Label=(gcc-10%2C%20ubuntu%2C%20openmpi%2C%20no%20LB%2C%20spack-package))](https://dev.azure.com/DARMA-tasking/DARMA/_build/latest?definitionId=20&branchName=develop)
![apple clang, macosx, mpich](https://github.com/DARMA-tasking/vt/workflows/PR%20tests%20(clang-8,%20macosx,%20mpich)/badge.svg?branch=develop)
![Build Documentation](https://github.com/DARMA-tasking/vt/workflows/Build%20Documentation/badge.svg?branch=develop)
[![](https://github.com/DARMA-tasking/vt/wiki/build_time_badge.svg)](https://github.com/DARMA-tasking/vt/wiki/build_times.png)

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
