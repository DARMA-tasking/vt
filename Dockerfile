FROM lifflander1/vt:alpine-final
MAINTAINER Jonathan Lifflander <jliffla@sandia.gov>

WORKDIR /usr/src

RUN /bin/bash -c 'source $HOME/.bashrc && \
 source /usr/share/spack/share/spack/setup-env.sh && \
 spack env activate clang-mpich && \
 export CC=clang && \
 export CXX=clang++ && \
 echo $HTTP_PROXY && \
 echo $HTTPS_PROXY && \
 echo $ALL_PROXY && \
 echo $http_proxy && \
 echo $https_proxy && \
 echo $all_proxy && \
 if [ -d "detector" ]; then rm -Rf detector; fi && \
 if [ -d "checkpoint" ]; then rm -Rf checkpoint; fi && \
 if [ -d "vt" ]; then rm -Rf vt; fi && \
 git clone -b develop --depth 1 https://github.com/DARMA-tasking/checkpoint.git && \
 export CHECKPOINT=$PWD/checkpoint && \
 git clone -b master --depth 1 https://github.com/DARMA-tasking/detector.git && \
 export DETECTOR=$PWD/detector && \
 git clone -b develop https://github.com/DARMA-tasking/vt.git && \
 export VT=$PWD/vt && \
 echo $SOURCE_COMMIT && \
 cd $VT && git checkout $SOURCE_COMMIT && \
 cd $DETECTOR && \
 mkdir build && \
 cd build && \
 cmake -DCMAKE_INSTALL_PREFIX=$DETECTOR/install -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC   .. && \
 make && \
 make install && \
 cd $CHECKPOINT && \
 mkdir build && \
 cd build && \
 cmake -DCMAKE_INSTALL_PREFIX=$CHECKPOINT/install -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -Ddetector_DIR=$DETECTOR/install  .. && \
 make && \
 make install && \
 cd $VT && \
 mkdir build && \
 cd build && \
 cmake -DCMAKE_EXE_LINKER_FLAGS=-lexecinfo -DCMAKE_BUILD_TYPE=release -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -Ddetector_DIR=$DETECTOR/install -Dcheckpoint_DIR=$CHECKPOINT/install  ../ && \
 make -j1 && \
 make test'
