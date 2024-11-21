ARG compiler=gcc-12
ARG arch=amd64
ARG ubuntu=22.04

FROM lifflander1/vt:${arch}-ubuntu-${ubuntu}-${compiler}-cpp

# All ARGs are invalidated after FROM instruction, so it has to be redefined
ARG compiler=gcc-12

ARG proxy=""
ENV https_proxy=${proxy} \
    http_proxy=${proxy}

ENV DEBIAN_FRONTEND=noninteractive

ARG zoltan_enabled=0
ARG ZOLTAN_INSTALL_DIR=/trilinos-install
ENV ZOLTAN_DIR=${ZOLTAN_INSTALL_DIR}

RUN if test ${zoltan_enabled} -eq 1; then \
      apt-get update -y -q && \
      apt-get install -y -q --no-install-recommends \
      gfortran-$(echo ${compiler} | cut -d- -f2) && \
      apt-get clean && \
      rm -rf /var/lib/apt/lists/*; \
      ln -s \
      "$(which gfortran-$(echo ${compiler}  | cut -d- -f2))" \
      /usr/bin/gfortran; \
      ./zoltan.sh -j4 ${ZOLTAN_INSTALL_DIR}; \
    fi

COPY . /vt

RUN /vt/ci/build_cpp.sh /vt /build
RUN /vt/ci/test_cpp.sh /vt /build
RUN /vt/ci/build_vt_sample.sh /vt /build
