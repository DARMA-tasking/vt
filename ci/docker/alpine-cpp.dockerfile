
ARG arch=amd64
FROM alpine:3.16 AS base

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

FROM base AS build
COPY . /vt
RUN /vt/ci/build_cpp.sh /vt /build

FROM build AS test
RUN /vt/ci/test_cpp.sh /vt /build
