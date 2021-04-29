
ARG arch=amd64
FROM alpine:3.13 as base

ARG proxy=""
ARG compiler=clang

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

RUN apk update && \
    apk upgrade && \
    apk add --no-cache \
        alpine-sdk \
        autoconf \
        automake \
        bash \
        binutils-dev \
        boost-dev \
        ccache \
        clang \
        clang-dev \
        cmake \
        dpkg \
        libdwarf-dev \
        libexecinfo-dev \
        libtool \
        linux-headers \
        m4 \
        make \
        ninja \
        perl \
        # python3 \
        wget \
        zlib \
        zlib-dev

RUN ls -l /usr/bin/cc /usr/bin/c++ /usr/bin/clang /usr/bin/clang++ && \
    ln -sf /usr/bin/clang /usr/bin/cc && \
    ln -sf /usr/bin/clang++ /usr/bin/c++

RUN update-alternatives --install /usr/bin/cc cc /usr/bin/clang 10 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 10 && \
    update-alternatives --auto cc && \
    update-alternatives --auto c++ && \
    update-alternatives --display cc && \
    update-alternatives --display c++

RUN ls -l /usr/bin/cc /usr/bin/c++ && \
    cc --version && \
    c++ --version

ENV CC=clang \
    CXX=clang++

COPY ./ci/deps/mpich.sh mpich.sh
RUN ./mpich.sh 3.3.2 -j4

ENV CMAKE_EXE_LINKER_FLAGS="-lexecinfo" \
    CC=mpicc \
    CXX=mpicxx \
    PATH=/usr/lib/ccache/:$PATH

RUN ls -l /usr/bin/cc /usr/bin/c++ && \
    cc --version && \
    c++ --version

FROM base as build
COPY . /vt

ARG VT_LB_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_ASAN_ENABLED
ARG VT_WERROR_ENABLED
ARG VT_POOL_ENABLED
ARG VT_PRODUCTION_BUILD_ENABLED
ARG VT_FCONTEXT_ENABLED
ARG VT_USE_OPENMP
ARG VT_USE_STD_THREAD
ARG CMAKE_BUILD_TYPE

ENV VT_LB_ENABLED=${VT_LB_ENABLED} \
    VT_TRACE_ENABLED=${VT_TRACE_ENABLED} \
    VT_MIMALLOC_ENABLED=${VT_MIMALLOC_ENABLED} \
    VT_DOXYGEN_ENABLED=${VT_DOXYGEN_ENABLED} \
    VT_TRACE_RUNTIME_ENABLED=${VT_TRACE_RUNTIME} \
    VT_ASAN_ENABLED=${VT_ASAN_ENABLED} \
    VT_WERROR_ENABLED=${VT_WERROR_ENABLED} \
    VT_POOL_ENABLED=${VT_POOL_ENABLED} \
    VT_PRODUCTION_BUILD_ENABLED=${VT_PRODUCTION_BUILD_ENABLED} \
    VT_FCONTEXT_ENABLED=${VT_FCONTEXT_ENABLED} \
    VT_USE_OPENMP=${VT_USE_OPENMP} \
    VT_USE_STD_THREAD=${VT_USE_STD_THREAD} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build
COPY /build/ /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
