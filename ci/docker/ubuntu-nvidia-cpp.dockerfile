
ARG compiler=11.0.3
ARG arch=amd64
# Works with 20.04 and 22.04
ARG ubuntu=20.04
FROM --platform=${arch} nvidia/cuda:${compiler}-devel-ubuntu${ubuntu} as base

ARG host_compiler=gcc-9
ARG proxy=""

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y && \
    apt-get install -y software-properties-common --no-install-recommends && \
    add-apt-repository -y ppa:ubuntu-toolchain-r/test && \
    apt remove -y software-properties-common && \
    apt-get install -y --no-install-recommends \
    ca-certificates \
    g++-$(echo ${host_compiler} | cut -d- -f2) \
    curl \
    less \
    git \
    wget \
    zlib1g \
    zlib1g-dev \
    ninja-build \
    gnupg \
    make-guile \
    libomp5 \
    libunwind-dev \
    valgrind \
    ccache && \
    apt-get autoremove -y && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV CC=gcc \
    CXX=g++

ARG arch

COPY ./ci/deps/cmake.sh cmake.sh
RUN ./cmake.sh 3.23.4 ${arch}

ENV PATH=/cmake/bin/:$PATH
ENV LESSCHARSET=utf-8

COPY ./ci/deps/mpich.sh mpich.sh
RUN ./mpich.sh 3.3.2 -j4

ARG external_fmt
COPY ./ci/deps/fmt.sh fmt.sh

RUN if test ${external_fmt} -eq 1; then \
      chmod +x ./fmt.sh && \
      ./fmt.sh 7.1.3 -j4; \
    fi

RUN mkdir -p /nvcc_wrapper/build && \
    wget https://raw.githubusercontent.com/kokkos/kokkos/master/bin/nvcc_wrapper -P /nvcc_wrapper/build && \
    chmod +x /nvcc_wrapper/build/nvcc_wrapper

ENV MPI_EXTRA_FLAGS="" \
    HOST_COMPILER=${host_compiler} \
    PATH=/usr/lib/ccache/:/nvcc_wrapper/build:$PATH \
    CXX=nvcc_wrapper

FROM base as build
COPY . /vt
RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build

RUN /vt/ci/build_vt_sample.sh /vt /build
