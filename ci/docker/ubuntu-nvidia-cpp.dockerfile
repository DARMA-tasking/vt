
ARG arch=amd64
FROM ${arch}/ubuntu:22.04 as base

ARG proxy=""
ARG compiler=nvcc-11
ARG host_compiler=gcc-11

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
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
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

RUN if test ${compiler} = "nvcc-11"; then \
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
    else \
      wget http://developer.download.nvidia.com/compute/cuda/repos/ubuntu1804/x86_64/cuda-ubuntu1804.pin && \
      mv cuda-ubuntu1804.pin /etc/apt/preferences.d/cuda-repository-pin-600 && \
      wget https://developer.download.nvidia.com/compute/cuda/11.2.0/local_installers/cuda-repo-ubuntu1804-11-2-local_11.2.0-460.27.04-1_amd64.deb && \
      dpkg -i cuda-repo-ubuntu1804-11-2-local_11.2.0-460.27.04-1_amd64.deb && \
      apt-key add /var/cuda-repo-ubuntu1804-11-2-local/7fa2af80.pub && \
      apt-get update && \
      apt-get -y install cuda-nvcc-11-2 && \
      apt-get clean && \
      rm -rf /var/lib/apt/lists/* && \
      rm -rf cuda-repo-ubuntu1804-11-2-local_11.2.0-460.27.04-1_amd64.deb && \
      ln -s /usr/local/cuda-11.2 /usr/local/cuda-versioned; \
    fi

ENV CC=gcc \
    CXX=g++

COPY ./ci/deps/cmake.sh cmake.sh
RUN ./cmake.sh 3.18.4

ENV PATH=/cmake/bin/:$PATH
ENV LESSCHARSET=utf-8

COPY ./ci/deps/mpich.sh mpich.sh
RUN ./mpich.sh 3.3.2 -j4

ENV CUDACXX=/usr/local/cuda-versioned/bin/nvcc
ENV PATH=/usr/local/cuda-versioned/bin/:$PATH

RUN mkdir -p /nvcc_wrapper/build && \
    wget https://raw.githubusercontent.com/kokkos/kokkos/master/bin/nvcc_wrapper -P /nvcc_wrapper/build && \
    chmod +x /nvcc_wrapper/build/nvcc_wrapper

ENV MPI_EXTRA_FLAGS="" \
    CXX=/nvcc_wrapper/build/nvcc_wrapper \
    PATH=/usr/lib/ccache/:$PATH

FROM base as build
COPY . /vt

ARG HOST_COMPILER
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
ARG CMAKE_BUILD_TYPE
ARG VT_EXTENDED_TESTS_ENABLED
ARG VT_FCONTEXT_ENABLED
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
