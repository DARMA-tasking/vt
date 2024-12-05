ARG arch=amd64
ARG ubuntu=22.04
FROM ${arch}/ubuntu:${ubuntu} as base

ARG proxy=""
ARG compiler=gcc-9
ARG token

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
        ${compiler} \
        ca-certificates \
        ccache \
        curl \
        ghostscript \
        git \
        libmpich-dev \
        mpich \
        ninja-build \
        python3 \
        python3-jinja2 \
        python3-pygments \
        texlive-font-utils \
        wget \
        zlib1g \
        zlib1g-dev &&\
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV MPI_EXTRA_FLAGS="" \
    CMAKE_PREFIX_PATH="/lib/x86_64-linux-gnu/" \
    CC=mpicc \
    CXX=mpicxx \
    PATH=/usr/lib/ccache/:$PATH

ARG arch

COPY ./ci/deps/cmake.sh cmake.sh
RUN ./cmake.sh 3.23.4 ${arch}

ENV PATH=/cmake/bin/:$PATH

COPY ./ci/deps/doxygen.sh doxygen.sh
RUN ./doxygen.sh 1.8.16

ENV PATH=/doxygen/bin/:$PATH

FROM base as build
COPY . /vt

ARG token

RUN /vt/ci/build_cpp.sh /vt /build "${token}"
