
ARG arch=amd64
FROM ${arch}/ubuntu:20.04 as base

ARG proxy=""
ARG compiler=clang-10

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    ca-certificates \
    curl \
    cmake \
    git \
    wget \
    ${compiler} \
    zlib1g \
    zlib1g-dev \
    ninja-build \
    valgrind \
    make-guile \
    libomp5 \
    ccache && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s \
    "$(which $(echo ${compiler}  | cut -d- -f1)++-$(echo ${compiler}  | cut -d- -f2))" \
    /usr/bin/clang++

ENV CC=${compiler} \
    CXX=clang++

RUN wget http://www.mpich.org/static/downloads/3.3.2/mpich-3.3.2.tar.gz && \
    tar xzf mpich-3.3.2.tar.gz && \
    rm mpich-3.3.2.tar.gz && \
    cd mpich-3.3.2 && \
    ./configure \
        --enable-static=false \
        --enable-alloca=true \
        --disable-long-double \
        --enable-threads=single \
        --enable-fortran=no \
        --enable-fast=all \
        --enable-g=none \
        --enable-timing=none && \
    make -j4 && \
    make install && \
    cd - && \
    rm -rf mpich-3.3.2

ENV MPI_EXTRA_FLAGS="" \
    CMAKE_PREFIX_PATH="/lib/x86_64-linux-gnu/" \
    PATH=/usr/lib/ccache/:$PATH \
    CMAKE_EXE_LINKER_FLAGS="-pthread"

FROM base as build
COPY . /vt

ARG VT_LB_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_ASAN_ENABLED
ARG VT_POOL_ENABLED
ARG CMAKE_BUILD_TYPE
ARG VT_EXTENDED_TESTS_ENABLED

ENV VT_LB_ENABLED=${VT_LB_ENABLED} \
    VT_TRACE_ENABLED=${VT_TRACE_ENABLED} \
    VT_MIMALLOC_ENABLED=${VT_MIMALLOC_ENABLED} \
    VT_DOXYGEN_ENABLED=${VT_DOXYGEN_ENABLED} \
    VT_TRACE_RUNTIME_ENABLED=${VT_TRACE_RUNTIME} \
    VT_ASAN_ENABLED=${VT_ASAN_ENABLED} \
    VT_POOL_ENABLED=${VT_POOL_ENABLED} \
    VT_MPI_GUARD_ENABLED=${VT_MPI_GUARD_ENABLED} \
    VT_EXTENDED_TESTS_ENABLED=${VT_EXTENDED_TESTS_ENABLED} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
