
ARG arch=amd64
FROM ${arch}/ubuntu:18.04 as base

ARG proxy=""
ARG compiler=gcc-7

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
    PATH=/usr/lib/ccache/:$PATH

RUN if test ${zoltan_enabled} -eq 1; then \
      if test -d Trilinos; then \
         echo "Found Trilinos already"; \
      else \
        git clone https://github.com/trilinos/Trilinos.git --depth=1 && \
        mkdir -p trilinos-build && \
        mkdir -p trilinos-install; \
      fi && \
      cd trilinos-build && \
      export FC=/usr/bin/gfortran && \
      cmake \
        -D CMAKE_INSTALL_PREFIX:FILEPATH="/trilinos-install/" \
        -D TPL_ENABLE_MPI:BOOL=ON \
        -D CMAKE_C_FLAGS:STRING="-m64 -g" \
        -D CMAKE_CXX_FLAGS:STRING="-m64 -g" \
        -D Trilinos_ENABLE_ALL_PACKAGES:BOOL=OFF \
        -D Trilinos_ENABLE_Zoltan:BOOL=ON \
        -D Zoltan_ENABLE_ULLONG_IDS:Bool=ON \
        ../Trilinos && \
      make -j4 && \
      make install && \
      cd - && \
      rm -rf Trilinos; \
    fi

ENV ZOLTAN_DIR=/trilinos-install

FROM base as build
COPY . /vt

ARG VT_LB_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_ASAN_ENABLED
ARG VT_POOL_ENABLED
ARG VT_ZOLTAN_ENABLED
ARG CMAKE_BUILD_TYPE
ARG VT_FCONTEXT_ENABLED
ARG VT_USE_OPENMP
ARG VT_USE_STD_THREAD

ENV VT_LB_ENABLED=${VT_LB_ENABLED} \
    VT_TRACE_ENABLED=${VT_TRACE_ENABLED} \
    VT_MIMALLOC_ENABLED=${VT_MIMALLOC_ENABLED} \
    VT_DOXYGEN_ENABLED=${VT_DOXYGEN_ENABLED} \
    VT_TRACE_RUNTIME_ENABLED=${VT_TRACE_RUNTIME} \
    VT_ASAN_ENABLED=${VT_ASAN_ENABLED} \
    VT_POOL_ENABLED=${VT_POOL_ENABLED} \
    VT_ZOLTAN_ENABLED=${VT_ZOLTAN_ENABLED} \
    VT_MPI_GUARD_ENABLED=${VT_MPI_GUARD_ENABLED} \
    VT_FCONTEXT_ENABLED=${VT_FCONTEXT_ENABLED} \
    VT_USE_OPENMP=${VT_USE_OPENMP} \
    VT_USE_STD_THREAD=${VT_USE_STD_THREAD} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
