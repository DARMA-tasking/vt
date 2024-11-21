ARG arch=amd64
ARG ubuntu=20.04
FROM ${arch}/ubuntu:${ubuntu} AS base

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
        brotli \
        ca-certificates \
        ccache \
        curl \
        git \
        less \
        libgl1-mesa-dev \
        libglu1-mesa-dev \
        libncurses5-dev \
        libomp5 \
        libunwind-dev \
        m4 \
        make-guile \
        mesa-common-dev \
        ninja-build \
        python3 \
        python3-brotli \
        python3-deepdiff \
        python3-numpy \
        python3-pip \
        python3-schema \
        valgrind \
        wget \
        zlib1g \
        zlib1g-dev && \
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
RUN ./mpich.sh 4.0.2 -j4

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

FROM base as build
COPY . /vt
RUN /vt/ci/build_cpp.sh /vt /build

FROM build AS test
RUN /vt/ci/test_cpp.sh /vt /build

RUN /vt/ci/build_vt_sample.sh /vt /build
