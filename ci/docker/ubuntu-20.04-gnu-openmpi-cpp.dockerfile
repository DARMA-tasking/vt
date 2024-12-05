
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
RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build

RUN /vt/ci/build_vt_sample.sh /vt /build
