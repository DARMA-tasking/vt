ARG REPO=lifflander1/vt
ARG ARCH=amd64
ARG DISTRO=ubuntu
ARG DISTRO_VERSION=22.04
ARG COMPILER=clang-13

ARG BASE=${REPO}:wf-${ARCH}-${DISTRO}-${DISTRO_VERSION}-${COMPILER}-cpp

FROM --platform=${ARCH} ${BASE} AS build

COPY . /vt
RUN /vt/ci/build_cpp.sh /vt /build

FROM build AS test
RUN /vt/ci/test_cpp.sh /vt /build

FROM test AS test_sample
RUN /vt/ci/build_vt_sample.sh /vt /build