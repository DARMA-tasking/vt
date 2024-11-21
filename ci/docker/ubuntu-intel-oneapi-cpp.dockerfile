
FROM intel/oneapi:os-tools-ubuntu20.04 as base

ARG proxy=""
ARG compiler=icpx

ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -y -q && \
    apt-get install -y -q --no-install-recommends \
    intel-oneapi-compiler-dpcpp-cpp-and-cpp-classic-2022.2.1 \
    ca-certificates \
    less \
    curl \
    git \
    wget \
    zlib1g \
    zlib1g-dev \
    ninja-build \
    valgrind \
    make-guile \
    libomp5 \
    libunwind-dev \
    ccache && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

ENV CMAKE_PREFIX_PATH='/opt/intel/oneapi/tbb/latest/env/..' \
    CMPLR_ROOT=/opt/intel/oneapi/compiler/latest \
    CPATH='/opt/intel/oneapi/tbb/latest/env/../include:/opt/intel/oneapi/dev-utilities/latest/include:/opt/intel/oneapi/compiler/latest/linux/include' \
    INFOPATH=/opt/intel/oneapi/debugger/10.1.2/gdb/intel64/lib \
    INTEL_LICENSE_FILE='/opt/intel/licenses:/root/intel/licenses:/opt/intel/licenses:/root/intel/licenses:/Users/Shared/Library/Application Support/Intel/Licenses' \
    LD_LIBRARY_PATH='/opt/intel/oneapi/tbb/latest/env/../lib/intel64/gcc4.8:/opt/intel/oneapi/debugger/10.1.1/dep/lib:/opt/intel/oneapi/debugger/10.1.1/libipt/intel64/lib:/opt/intel/oneapi/debugger/10.1.1/gdb/intel64/lib:/opt/intel/oneapi/compiler/latest/linux/lib:/opt/intel/oneapi/compiler/latest/linux/lib/x64:/opt/intel/oneapi/compiler/latest/linux/lib/emu:/opt/intel/oneapi/compiler/latest/linux/lib/oclfpga/host/linux64/lib:/opt/intel/oneapi/compiler/latest/linux/lib/oclfpga/linux64/lib:/opt/intel/oneapi/compiler/latest/linux/compiler/lib/intel64_lin:/opt/intel/oneapi/compiler/latest/linux/compiler/lib' \
    LIBRARY_PATH='/opt/intel/oneapi/tbb/latest/env/../lib/intel64/gcc4.8:/opt/intel/oneapi/compiler/latest/linux/compiler/lib/intel64_lin:/opt/intel/oneapi/compiler/latest/linux/lib' \
    ONEAPI_ROOT='/opt/intel/oneapi' \
    PATH='/opt/intel/oneapi/dev-utilities/latest/bin:/opt/intel/oneapi/debugger/10.1.1/gdb/intel64/bin:/opt/intel/oneapi/compiler/latest/linux/lib/oclfpga/llvm/aocl-bin:/opt/intel/oneapi/compiler/latest/linux/lib/oclfpga/bin:/opt/intel/oneapi/compiler/latest/linux/bin/intel64:/opt/intel/oneapi/compiler/latest/linux/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin' \
    TBBROOT='/opt/intel/oneapi/tbb/latest/env/..'

RUN ln -s \
    "$(which $(echo $compiler | sed 's/p//'))" \
    /usr/bin/intel-cc

ENV CC=intel-cc \
    CXX=${compiler}

COPY ./ci/deps/mpich.sh mpich.sh
RUN ./mpich.sh 4.0.2 -j4

ENV CC=mpicc \
    CXX=mpicxx \
    MPICH_CC=intel-cc \
    MPICH_CXX=${compiler} \
    MPI_EXTRA_FLAGS="" \
    LESSCHARSET=utf-8 \
    PATH=/usr/lib/ccache/:$PATH

FROM base as build
COPY . /vt
RUN /vt/ci/build_cpp.sh /vt /build

FROM build as test
RUN /vt/ci/test_cpp.sh /vt /build

RUN /vt/ci/build_vt_sample.sh /vt /build
