FROM ubuntu:18.04
MAINTAINER Jonathan Lifflander <jliffla@sandia.gov>

RUN apt-get update && apt-get install -y \
  curl \
  cmake \
  git \
  googletest \
  libmpich-dev \
  wget \
  gcc-7 \
  zlib1g \
  zlib1g-dev \
  libopenmpi-dev

WORKDIR /usr/src

RUN \
 dpkg -L zlib1g && \
 export CC=mpicc && \
 export CXX=mpicxx && \
 if [ -d "googletest" ]; then rm -Rf googletest; fi && \
 if [ -d "detector" ]; then rm -Rf detector; fi && \
 if [ -d "checkpoint" ]; then rm -Rf checkpoint; fi && \
 if [ -d "vt" ]; then rm -Rf vt; fi && \
 git clone -b release-1.8.1 --depth 1 https://github.com/google/googletest.git && \
 export GTEST=$PWD/googletest && \
 git clone -b develop --depth 1 https://github.com/DARMA-tasking/checkpoint.git && \
 export CHECKPOINT=$PWD/checkpoint && \
 git clone -b master --depth 1 https://github.com/DARMA-tasking/detector.git && \
 export DETECTOR=$PWD/detector && \
 git clone -b develop --depth 1 https://github.com/DARMA-tasking/vt.git && \
 export VT=$PWD/vt && \
 echo $SOURCE_COMMIT && \
 cd $VT && git checkout $SOURCE_COMMIT && \
 cd $GTEST && \
 mkdir build && \
 cd build && \
 cmake -DCMAKE_INSTALL_PREFIX=$GTEST/install -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC .. && \
 make && \
 make install && \
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
 uname -p && \
 uname  && \
 cmake -DVT_NO_BUILD_EXAMPLES=1 -Dgtest_DIR=$GTEST/install -DGTEST_ROOT=$GTEST/install -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=$CXX -DCMAKE_C_COMPILER=$CC -DMPI_C_COMPILER=mpicc -DMPI_CXX_COMPILER=mpicxx -Ddetector_DIR=$DETECTOR/install -DCMAKE_PREFIX_PATH="$GTEST/install;/lib/x86_64-linux-gnu/" -Dcheckpoint_DIR=$CHECKPOINT/install  ../ && \
 make -j4 && \
 make test
