
ARG arch=amd64
FROM ${arch}/ubuntu:18.04 as base

ARG proxy=""
ARG compiler=gcc-7
ARG token

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    ca-certificates \
    curl \
    cmake \
    git \
    mpich \
    libmpich-dev \
    wget \
    ${compiler} \
    zlib1g \
    zlib1g-dev \
    ninja-build \
    doxygen \
    python3 \
    python3-jinja2 \
    python3-pygments \
    texlive-font-utils \
    ghostscript \
    ccache && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV MPI_EXTRA_FLAGS="" \
    CMAKE_PREFIX_PATH="/lib/x86_64-linux-gnu/" \
    CC=mpicc \
    CXX=mpicxx \
    PATH=/usr/lib/ccache/:$PATH

FROM base as build
COPY . /vt

ARG token
ARG VT_LB_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_ASAN_ENABLED
ARG VT_WERROR_ENABLED
ARG VT_POOL_ENABLED
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
    VT_DIAGNOSTICS_ENABLED=${VT_DIAGNOSTICS_ENABLED} \
    VT_DIAGNOSTICS_RUNTIME_ENABLED=${VT_DIAGNOSTICS_RUNTIME_ENABLED} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build "${token}"
