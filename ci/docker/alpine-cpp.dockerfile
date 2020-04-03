
ARG arch=amd64
FROM lifflander1/vt:alpine-final as base

ARG proxy=""
ARG compiler=clang

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

RUN source $HOME/.bashrc && \
    source /usr/share/spack/share/spack/setup-env.sh && \
    spack env activate clang-mpich

ENV CMAKE_EXE_LINKER_FLAGS="-lexecinfo" \
    CC=mpicc \
    CXX=mpicxx \
    PATH=/usr/lib/ccache/:$PATH \
    PATH=/usr/share/spack/var/spack/environments/clang-mpich/.spack-env/view/bin/:$PATH

FROM base as build
COPY . /vt

ARG VT_LB_ENABLED
ARG VT_TRACE_ENABLED
ARG VT_TRACE_RUNTIME_ENABLED
ARG VT_MIMALLOC_ENABLED
ARG VT_DOXYGEN_ENABLED
ARG VT_ASAN_ENABLED
ARG CMAKE_BUILD_TYPE

ENV VT_LB_ENABLED=${VT_LB_ENABLED} \
    VT_TRACE_ENABLED=${VT_TRACE_ENABLED} \
    VT_MIMALLOC_ENABLED=${VT_MIMALLOC_ENABLED} \
    VT_DOXYGEN_ENABLED=${VT_DOXYGEN_ENABLED} \
    VT_TRACE_RUNTIME_ENABLED=${VT_TRACE_RUNTIME} \
    VT_ASAN_ENABLED=${VT_ASAN_ENABLED} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build
COPY /build/ /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
