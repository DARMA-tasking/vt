FROM lifflander1/vt:alpine-final
MAINTAINER Jonathan Lifflander <jliffla@sandia.gov>

COPY . /usr/src/vt/

WORKDIR /usr/src

ARG LB_ENABLED
ARG TRACE_ENABLED

RUN /bin/bash -c 'source $HOME/.bashrc && \
 source /usr/share/spack/share/spack/setup-env.sh && \
 spack env activate clang-mpich && \
  ls /usr/src/vt && \
 export CC=clang && \
 export CXX=clang++ && \
 echo $HTTP_PROXY && \
 echo $HTTPS_PROXY && \
 echo $ALL_PROXY && \
 echo $http_proxy && \
 echo $https_proxy && \
 echo $all_proxy && \
 unset https_proxy &&  \
 unset http_proxy && \
 unset all_proxy && \
 unset HTTPS_PROXY && \
 unset HTTP_PROXY && \
 unset ALL_PROXY && \
 if [ -d "detector" ]; then rm -Rf detector; fi && \
 if [ -d "checkpoint" ]; then rm -Rf checkpoint; fi && \
 git clone -b develop --depth 1 https://github.com/DARMA-tasking/checkpoint.git && \
 export CHECKPOINT=$PWD/checkpoint && \
 export CHECKPOINT_BUILD=/usr/build/checkpoint && \
 git clone -b master --depth 1 https://github.com/DARMA-tasking/detector.git && \
 export DETECTOR=$PWD/detector && \
 export DETECTOR_BUILD=/usr/build/detector && \
 export VT=/usr/src/vt && \
 export VT_BUILD=/usr/build/vt && \
 echo $SOURCE_COMMIT && \
 cd $DETECTOR_BUILD && \
 mkdir build && \
 cd build && \
 cmake -DCMAKE_INSTALL_PREFIX=$DETECTOR_BUILD/install -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC $DETECTOR && \
 make && \
 make install && \
 cd $CHECKPOINT_BUILD && \
 mkdir build && \
 cd build && \
 cmake -DCMAKE_INSTALL_PREFIX=$CHECKPOINT_BUILD/install -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -Ddetector_DIR=$DETECTOR_BUILD/install $CHECKPOINT && \
 make && \
 make install && \
 cd $VT_BUILD && \
 mkdir build && \
 cd build && \
 cmake -GNinja -Dvt_lb_enabled=$LB_ENABLED -Dvt_trace_enabled=$TRACE_ENABLED -DCMAKE_INSTALL_PREFIX=$VT_BUILD/install -DCMAKE_EXE_LINKER_FLAGS=-lexecinfo -DCMAKE_BUILD_TYPE=release -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -Ddetector_DIR=$DETECTOR_BUILD/install -Dcheckpoint_DIR=$CHECKPOINT_BUILD/install $VT && \
 ninja && \
 ninja install && \
 ninja test || ctest -V'

COPY $DETECTOR_BUILD/ $DETECTOR_BUILD
COPY $CHECKPOINT_BUILD/ $CHECKPOINT_BUILD
COPY $VT_BUILD/ $VT_BUILD
