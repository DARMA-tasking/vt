
FROM intel/oneapi:os-tools-ubuntu18.04 as base

ARG proxy=""

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    intel-oneapi-compiler-dpcpp-cpp-and-cpp-classic \
    intel-oneapi-mpi-devel \
    ca-certificates \
    less \
    curl \
    git \
    wget \
    zlib1g \
    zlib1g-dev \
    ninja-build \
    valgrind \
    make-guile \
    libomp5 \
    ccache && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV LESSCHARSET=utf-8

ENV MPICH_CC=icc \
    MPICH_CXX=icpc \
    CC=mpicc \
    CXX=mpicxx

ENV MPI_EXTRA_FLAGS="" \
    PATH=/usr/lib/ccache/:$PATH


FROM base as build
COPY . /vt

ARG VT_LB_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_ASAN_ENABLED
ARG VT_WERROR_ENABLED
ARG VT_POOL_ENABLED
ARG VT_PRODUCTION_BUILD_ENABLED
ARG VT_FCONTEXT_ENABLED
ARG VT_USE_OPENMP
ARG VT_USE_STD_THREAD
ARG CMAKE_BUILD_TYPE
ARG VT_EXTENDED_TESTS_ENABLED

ENV VT_LB_ENABLED=${VT_LB_ENABLED} \
    VT_TRACE_ENABLED=${VT_TRACE_ENABLED} \
    VT_MIMALLOC_ENABLED=${VT_MIMALLOC_ENABLED} \
    VT_DOXYGEN_ENABLED=${VT_DOXYGEN_ENABLED} \
    VT_TRACE_RUNTIME_ENABLED=${VT_TRACE_RUNTIME} \
    VT_ASAN_ENABLED=${VT_ASAN_ENABLED} \
    VT_WERROR_ENABLED=${VT_WERROR_ENABLED} \
    VT_POOL_ENABLED=${VT_POOL_ENABLED} \
    VT_MPI_GUARD_ENABLED=${VT_MPI_GUARD_ENABLED} \
    VT_EXTENDED_TESTS_ENABLED=${VT_EXTENDED_TESTS_ENABLED} \
    VT_UNITY_BUILD_ENABLED=${VT_UNITY_BUILD_ENABLED} \
    VT_PRODUCTION_BUILD_ENABLED=${VT_PRODUCTION_BUILD_ENABLED} \
    VT_FCONTEXT_ENABLED=${VT_FCONTEXT_ENABLED} \
    VT_USE_OPENMP=${VT_USE_OPENMP} \
    VT_USE_STD_THREAD=${VT_USE_STD_THREAD} \
    VT_DIAGNOSTICS_ENABLED=${VT_DIAGNOSTICS_ENABLED} \
    VT_DIAGNOSTICS_RUNTIME_ENABLED=${VT_DIAGNOSTICS_RUNTIME_ENABLED} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN . /opt/intel/oneapi/setvars.sh && \
    /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
