
ARG arch=amd64
FROM alpine:3.16 as base

ARG proxy=""

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

RUN apk add --no-cache \
        alpine-sdk \
        autoconf \
        automake \
        bash \
        binutils-dev \
        ccache \
        clang \
        clang-dev \
        cmake \
        dpkg \
        libdwarf-dev \
        libunwind-dev \
        libtool \
        linux-headers \
        m4 \
        make \
        ninja \
        wget \
        zlib \
        zlib-dev

RUN ln -sf /usr/bin/clang /usr/bin/cc && \
    ln -sf /usr/bin/clang++ /usr/bin/c++ && \
    update-alternatives --install /usr/bin/cc cc /usr/bin/clang 10 && \
    update-alternatives --install /usr/bin/c++ c++ /usr/bin/clang++ 10 && \
    update-alternatives --auto cc && \
    update-alternatives --auto c++ && \
    update-alternatives --display cc && \
    update-alternatives --display c++ && \
    cc --version && \
    c++ --version

ENV CC=clang \
    CXX=clang++

COPY ./ci/deps/mpich.sh mpich.sh
RUN ./mpich.sh 3.3.2 -j4

ENV CC=mpicc \
    CXX=mpicxx \
    PATH=/usr/lib/ccache/:$PATH

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
ARG VT_FCONTEXT_ENABLED
ARG CMAKE_BUILD_TYPE
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
    VT_PRODUCTION_BUILD_ENABLED=${VT_PRODUCTION_BUILD_ENABLED} \
    VT_FCONTEXT_ENABLED=${VT_FCONTEXT_ENABLED} \
    VT_NO_COLOR_ENABLED=${VT_NO_COLOR_ENABLED} \
    CMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD} \
    BUILD_SHARED_LIBS=${BUILD_SHARED_LIBS} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
