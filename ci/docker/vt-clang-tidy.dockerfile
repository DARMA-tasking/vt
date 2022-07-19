FROM jstrz/vt-clang-tidy:pr-1-merge AS base

ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /

RUN apt-get remove cmake -y -q
RUN apt-get update -y -q
RUN apt-get install -y -q --no-install-recommends \
    ca-certificates \
    ccache \
    curl \
    git \
    less \
    libomp-dev \
    libomp5 \
    make-guile \
    ninja-build \
    python \
    python3 \
    valgrind \
    wget \
    zlib1g \
    zlib1g-dev && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV CC=clang \
    CXX=clang++

COPY ./ci/deps/libunwind.sh libunwind.sh
RUN ./libunwind.sh 1.6.2

COPY ./ci/deps/cmake.sh cmake.sh
RUN ./cmake.sh 3.18.4

ENV PATH=/cmake/bin/:$PATH
ENV LESSCHARSET=utf-8

COPY ./ci/deps/mpich.sh mpich.sh
RUN ./mpich.sh 4.0.2 -j"$(nproc)"

ENV MPI_EXTRA_FLAGS="" \
    CMAKE_PREFIX_PATH="/lib/x86_64-linux-gnu/" \
    PATH=/usr/lib/ccache/:$PATH \
    CMAKE_EXE_LINKER_FLAGS="-pthread"

FROM base AS build
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
ARG VT_USE_OPENMP
ARG VT_USE_STD_THREAD
ARG VT_WERROR_ENABLED

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
    VT_USE_OPENMP=${VT_USE_OPENMP} \
    VT_USE_STD_THREAD=${VT_USE_STD_THREAD} \
    VT_WERROR_ENABLED=${VT_WERROR_ENABLED}

# Without build there is no `vt_git_revision.cc` which causes clang-tidy to exit with error
RUN /vt/ci/build_cpp.sh /vt /build
RUN /vt/ci/clang_tidy_cpp.sh /build
