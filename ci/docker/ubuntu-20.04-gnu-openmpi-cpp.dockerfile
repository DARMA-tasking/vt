
ARG arch=amd64
FROM ${arch}/ubuntu:20.04 as base

ARG proxy=""
ARG compiler=gcc-9

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

ARG zoltan_enabled

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    g++-$(echo ${compiler} | cut -d- -f2) \
    ca-certificates \
    less \
    curl \
    ${zoltan_enabled:+gfortran-$(echo ${compiler} | cut -d- -f2)} \
    git \
    wget \
    ${compiler} \
    zlib1g \
    zlib1g-dev \
    ninja-build \
    valgrind \
    make-guile \
    libomp5 \
    ccache \
    python3 \
    ssh && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN ln -s \
    "$(which g++-$(echo ${compiler}  | cut -d- -f2))" \
    /usr/bin/g++

RUN ln -s \
    "$(which gcc-$(echo ${compiler}  | cut -d- -f2))" \
    /usr/bin/gcc

RUN if test ${zoltan_enabled} -eq 1; then \
      ln -s \
      "$(which gfortran-$(echo ${compiler}  | cut -d- -f2))" \
      /usr/bin/gfortran; \
    fi

ENV CC=gcc \
    CXX=g++

ARG arch

COPY ./ci/deps/cmake.sh cmake.sh
RUN ./cmake.sh 3.23.4 ${arch}

ENV PATH=/cmake/bin/:$PATH
ENV LESSCHARSET=utf-8

COPY ./ci/deps/openmpi.sh openmpi.sh
RUN ./openmpi.sh v4.0 4.0.4 -j4

ENV MPI_EXTRA_FLAGS="--allow-run-as-root --oversubscribe" \
    PATH=/usr/lib/ccache/:$PATH

ARG ZOLTAN_INSTALL_DIR=/trilinos-install
ENV ZOLTAN_DIR=${ZOLTAN_INSTALL_DIR}

COPY ./ci/deps/zoltan.sh zoltan.sh
RUN if test ${zoltan_enabled} -eq 1; then \
      ./zoltan.sh -j4 ${ZOLTAN_INSTALL_DIR}; \
    fi

ENV OMPI_ALLOW_RUN_AS_ROOT=1 \
    OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

FROM base as build
COPY . /vt

ARG VT_LB_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_ASAN_ENABLED
ARG VT_UBSAN_ENABLED
ARG VT_WERROR_ENABLED
ARG VT_POOL_ENABLED
ARG VT_PRODUCTION_BUILD_ENABLED
ARG VT_ZOLTAN_ENABLED
ARG VT_FCONTEXT_ENABLED
ARG CMAKE_BUILD_TYPE
ARG VT_EXTENDED_TESTS_ENABLED
ARG VT_NO_COLOR_ENABLED
ARG BUILD_SHARED_LIBS
ARG CMAKE_CXX_STANDARD

ENV VT_LB_ENABLED=${VT_LB_ENABLED} \
    VT_TRACE_ENABLED=${VT_TRACE_ENABLED} \
    VT_MIMALLOC_ENABLED=${VT_MIMALLOC_ENABLED} \
    VT_DOXYGEN_ENABLED=${VT_DOXYGEN_ENABLED} \
    VT_TRACE_RUNTIME_ENABLED=${VT_TRACE_RUNTIME} \
    VT_ASAN_ENABLED=${VT_ASAN_ENABLED} \
    VT_UBSAN_ENABLED=${VT_UBSAN_ENABLED} \
    VT_WERROR_ENABLED=${VT_WERROR_ENABLED} \
    VT_POOL_ENABLED=${VT_POOL_ENABLED} \
    VT_MPI_GUARD_ENABLED=${VT_MPI_GUARD_ENABLED} \
    VT_ZOLTAN_ENABLED=${VT_ZOLTAN_ENABLED} \
    VT_EXTENDED_TESTS_ENABLED=${VT_EXTENDED_TESTS_ENABLED} \
    VT_UNITY_BUILD_ENABLED=${VT_UNITY_BUILD_ENABLED} \
    VT_PRODUCTION_BUILD_ENABLED=${VT_PRODUCTION_BUILD_ENABLED} \
    VT_FCONTEXT_ENABLED=${VT_FCONTEXT_ENABLED} \
    VT_DIAGNOSTICS_ENABLED=${VT_DIAGNOSTICS_ENABLED} \
    VT_DIAGNOSTICS_RUNTIME_ENABLED=${VT_DIAGNOSTICS_RUNTIME_ENABLED} \
    VT_NO_COLOR_ENABLED=${VT_NO_COLOR_ENABLED} \
    CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
    BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build

RUN /vt/ci/build_vt_sample.sh /vt /build
