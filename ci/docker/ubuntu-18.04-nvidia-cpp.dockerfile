
ARG cuda=10.1
ARG arch=amd64
FROM ${arch}/ubuntu:18.04 as base

ARG proxy=""
ARG compiler=nvcc-10

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    ca-certificates \
    g++-7 \
    curl \
    cmake \
    less \
    git \
    wget \
    zlib1g \
    zlib1g-dev \
    ninja-build \
    gnupg \
    make-guile \
    libomp5 \
    valgrind \
    ccache && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN if test ${compiler} = "nvcc-10"; then \
      wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-ubuntu1804.pin && \
      mv cuda-ubuntu1804.pin /etc/apt/preferences.d/cuda-repository-pin-600 && \
      wget http://developer.download.nvidia.com/compute/cuda/10.1/Prod/local_installers/cuda-repo-ubuntu1804-10-1-local-10.1.243-418.87.00_1.0-1_amd64.deb && \
      dpkg -i cuda-repo-ubuntu1804-10-1-local-10.1.243-418.87.00_1.0-1_amd64.deb && \
      apt-key add /var/cuda-repo-10-1-local-10.1.243-418.87.00/7fa2af80.pub && \
      apt-get update && \
      apt-get -y install cuda-nvcc-10-1 cuda-cudart-dev-10-1 && \
      apt-get clean && \
      rm -rf /var/lib/apt/lists/* && \
      rm -rf cuda-repo-ubuntu1804-10-1-local-10.1.243-418.87.00_1.0-1_amd64.deb && \
      ln -s /usr/local/cuda-10.1 /usr/local/cuda-versioned; \
    else \
      wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-ubuntu1804.pin && \
      mv cuda-ubuntu1804.pin /etc/apt/preferences.d/cuda-repository-pin-600 && \
      wget http://developer.download.nvidia.com/compute/cuda/11.0.1/local_installers/cuda-repo-ubuntu1804-11-0-local_11.0.1-450.36.06-1_amd64.deb && \
      dpkg -i cuda-repo-ubuntu1804-11-0-local_11.0.1-450.36.06-1_amd64.deb && \
      apt-key add /var/cuda-repo-ubuntu1804-11-0-local/7fa2af80.pub && \
      apt-get update && \
      apt-get -y install cuda-nvcc-11-0 && \
      apt-get clean && \
      rm -rf /var/lib/apt/lists/* && \
      rm -rf cuda-repo-ubuntu1804-11-0-local_11.0.1-450.36.06-1_amd64.deb && \
      ln -s /usr/local/cuda-11.0 /usr/local/cuda-versioned; \
    fi

ENV CC=gcc \
    CXX=g++

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

ENV CUDACXX=/usr/local/cuda-versioned/bin/nvcc
ENV PATH=/usr/local/cuda-versioned/bin/:$PATH

RUN git clone https://github.com/kokkos/nvcc_wrapper.git && \
    cd nvcc_wrapper && \
    mkdir build && \
    cd build && \
    cmake ../

ENV MPI_EXTRA_FLAGS="" \
    CXX=/nvcc_wrapper/build/nvcc_wrapper \
    PATH=/usr/lib/ccache/:$PATH

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
    VT_EXTENDED_TESTS_ENABLED=${VT_EXTENDED_TESTS_ENABLED} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
