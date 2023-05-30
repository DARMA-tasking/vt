ARG arch=amd64
ARG ubuntu=20.04
FROM ${arch}/ubuntu:${ubuntu} as base

ARG proxy=""
ARG compiler=gcc-9
ARG ubuntu

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

ARG zoltan_enabled

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    ${compiler} \
    g++-$(echo ${compiler} | cut -d- -f2) \
    ${zoltan_enabled:+gfortran-$(echo ${compiler} | cut -d- -f2)} \
    ca-certificates \
    ccache \
    curl \
    git \
    less \
    libomp5 \
    libunwind-dev \
    make-guile \
    ninja-build \
    python3 \
    valgrind \
    wget \
    zlib1g \
    zlib1g-dev \
    brotli \
    python3 \
    python3-brotli \
    python3-pip && \
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

COPY ./ci/deps/mpich.sh mpich.sh
RUN if [ "$ubuntu" = "18.04" ]; then \
      ./mpich.sh 3.3.2 -j4; else \
      ./mpich.sh 4.0.2 -j4; \
    fi

ENV MPI_EXTRA_FLAGS="" \
    PATH=/usr/lib/ccache/:$PATH

ARG ZOLTAN_INSTALL_DIR=/trilinos-install
ENV ZOLTAN_DIR=${ZOLTAN_INSTALL_DIR}

COPY ./ci/deps/zoltan.sh zoltan.sh
RUN if test ${zoltan_enabled} -eq 1; then \
      ./zoltan.sh -j4 ${ZOLTAN_INSTALL_DIR}; \
    fi

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    lcov && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN pip3 install schema

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
ARG VT_ZOLTAN_ENABLED
ARG CMAKE_CXX_STANDARD

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
    VT_ZOLTAN_ENABLED=${VT_ZOLTAN_ENABLED} \
    CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}

RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build

RUN /vt/ci/build_vt_sample.sh /vt /build
