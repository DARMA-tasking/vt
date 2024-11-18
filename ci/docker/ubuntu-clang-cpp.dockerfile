ARG arch=amd64
ARG ubuntu=20.04
FROM ${arch}/ubuntu:${ubuntu} AS base

ARG proxy=""
ARG compiler=clang-11
ARG ubsan_enabled
ARG ubuntu

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
        ${compiler} \
        ${ubsan_enabled:+llvm-$(echo ${compiler} | cut -d- -f2)} \
        brotli \
        ca-certificates \
        ccache \
        curl \
        git \
        less \
        libomp-dev \
        libomp5 \
        make-guile \
        ninja-build \
        python3 \
        python3-brotli \
        python3-deepdiff \
        python3-pip \
        python3-schema \
        valgrind \
        wget \
        zlib1g \
        zlib1g-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s \
    "$(which $(echo ${compiler}  | cut -d- -f1)++-$(echo ${compiler}  | cut -d- -f2))" \
    /usr/bin/clang++

ENV CC=${compiler} \
    CXX=clang++

COPY ./ci/deps/libunwind.sh libunwind.sh
RUN ./libunwind.sh 1.6.2

ARG arch

COPY ./ci/deps/cmake.sh cmake.sh
RUN ./cmake.sh 3.23.4 ${arch}

ENV PATH=/cmake/bin/:$PATH
ENV LESSCHARSET=utf-8

COPY ./ci/deps/mpich.sh mpich.sh
RUN ./mpich.sh 4.0.2 -j4

ENV MPI_EXTRA_FLAGS="" \
    CMAKE_PREFIX_PATH="/lib/x86_64-linux-gnu/" \
    PATH=/usr/lib/ccache/:$PATH \
    CMAKE_EXE_LINKER_FLAGS="-pthread"

FROM base as build
COPY . /vt

ARG BUILD_SHARED_LIBS
ARG CMAKE_BUILD_TYPE
ARG VT_ASAN_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_EXTENDED_TESTS_ENABLED
ARG VT_FCONTEXT_ENABLED
ARG VT_LB_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_NO_COLOR_ENABLED
ARG VT_POOL_ENABLED
ARG VT_PRODUCTION_BUILD_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_UBSAN_ENABLED
ARG VT_WERROR_ENABLED
ARG CMAKE_CXX_STANDARD
ARG VT_DEBUG_VERBOSE
ARG VT_CI_BUILD

ENV BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} \
    VT_ASAN_ENABLED=${VT_ASAN_ENABLED} \
    VT_DIAGNOSTICS_ENABLED=${VT_DIAGNOSTICS_ENABLED} \
    VT_DIAGNOSTICS_RUNTIME_ENABLED=${VT_DIAGNOSTICS_RUNTIME_ENABLED} \
    VT_DOXYGEN_ENABLED=${VT_DOXYGEN_ENABLED} \
    VT_EXTENDED_TESTS_ENABLED=${VT_EXTENDED_TESTS_ENABLED} \
    VT_FCONTEXT_ENABLED=${VT_FCONTEXT_ENABLED} \
    VT_LB_ENABLED=${VT_LB_ENABLED} \
    VT_MIMALLOC_ENABLED=${VT_MIMALLOC_ENABLED} \
    VT_MPI_GUARD_ENABLED=${VT_MPI_GUARD_ENABLED} \
    VT_NO_COLOR_ENABLED=${VT_NO_COLOR_ENABLED} \
    VT_POOL_ENABLED=${VT_POOL_ENABLED} \
    VT_PRODUCTION_BUILD_ENABLED=${VT_PRODUCTION_BUILD_ENABLED} \
    VT_TRACE_ENABLED=${VT_TRACE_ENABLED} \
    VT_TRACE_RUNTIME_ENABLED=${VT_TRACE_RUNTIME} \
    VT_UBSAN_ENABLED=${VT_UBSAN_ENABLED} \
    VT_UNITY_BUILD_ENABLED=${VT_UNITY_BUILD_ENABLED} \
    VT_WERROR_ENABLED=${VT_WERROR_ENABLED} \
    CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
    VT_DEBUG_VERBOSE=${VT_DEBUG_VERBOSE} \
    VT_CI_BUILD=${VT_CI_BUILD}

RUN /vt/ci/build_cpp.sh /vt /build

FROM build AS test
RUN /vt/ci/test_cpp.sh /vt /build

RUN /vt/ci/build_vt_sample.sh /vt /build
