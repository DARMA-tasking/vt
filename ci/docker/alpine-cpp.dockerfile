
ARG arch=amd64
FROM lifflander1/vt:alpine-final as base

ARG proxy=""
ARG compiler=clang

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

RUN source "$HOME/.bashrc" && \
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
ARG VT_WERROR_ENABLED
ARG VT_POOL_ENABLED
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
    VT_FCONTEXT_ENABLED=${VT_FCONTEXT_ENABLED} \
    VT_USE_OPENMP=${VT_USE_OPENMP} \
    VT_USE_STD_THREAD=${VT_USE_STD_THREAD} \
    CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}

RUN /vt/ci/build_cpp.sh /vt /build
COPY /build/ /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build
