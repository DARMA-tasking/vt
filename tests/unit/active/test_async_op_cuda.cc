/*
//@HEADER
// *****************************************************************************
//
//                            test_async_op_cuda.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "test_parallel_harness.h"

#include <vt/messaging/async_op_cuda.h>
#include <vt/objgroup/manager.h>

#include <gtest/gtest.h>


namespace vt { namespace tests { namespace unit {

#if __CUDACC__

using TestAsyncOp = TestParallelHarness;

using MyMsg = Message;

inline void checkCudaErrors(
  cudaError_t err, std::string const& additionalInfo,
  bool skipOnFailure = false
) {
  auto errorMsg = fmt::format(
    "{} failed with error -> {}\n", additionalInfo, cudaGetErrorString(err));

  if (cudaSuccess != err and skipOnFailure) {
    GTEST_SKIP() << errorMsg;
  }

  vtAbortIf(cudaSuccess != err, errorMsg);
}

__global__ void kernel(double* dst, double setVal) {
  int row = blockIdx.y * blockDim.y + threadIdx.y;
  int col = blockIdx.x * blockDim.x + threadIdx.x;
  int tID = row * blockDim.y + col;

  dst[tID] = setVal;
}

struct CUDAGroup {
  CUDAGroup() {
    auto const nBytes = dataSize_ * sizeof(double);

    // If first malloc fails, there's probably something wrong with CUDA env
    // so call GTEST_SKIP instead of failing the test
    checkCudaErrors(
      cudaMalloc((void**)&dataDevicePointer1_, nBytes),
      "cudaMalloc(dataDevicePointer1_)", true
    );

    checkCudaErrors(
      cudaMallocHost((void**)&dataHostPointer1_, nBytes),
      "cudaMallocHost(dataHostPointer1_)"
    );

    checkCudaErrors(
      cudaMalloc((void**)&dataDevicePointer2_, nBytes),
      "cudaMalloc(dataDevicePointer2_)"
    );

    checkCudaErrors(
      cudaMallocHost((void**)&dataHostPointer2_, nBytes),
      "cudaMallocHost(dataHostPointer2_)"
    );
  }

  ~CUDAGroup() {
    checkCudaErrors(
      cudaFree(dataDevicePointer1_), "cudaFree(dataDevicePointer1_)"
    );
    checkCudaErrors(
      cudaFree(dataDevicePointer2_), "cudaFree(dataDevicePointer1_)"
    );

    checkCudaErrors(
      cudaFreeHost(dataHostPointer1_), "cudaFreeHost(dataHostPointer1_)"
    );
    checkCudaErrors(
      cudaFreeHost(dataHostPointer2_), "cudaFreeHost(dataHostPointer2_)"
    );

    checkCudaErrors(cudaStreamDestroy(stream1_), "cudaStreamDestroy(stream1_)");
    checkCudaErrors(cudaStreamDestroy(stream2_), "cudaStreamDestroy(stream2_)");
  }

  void cudaHandler(MyMsg* msg) {
    auto const nBytes = dataSize_ * sizeof(double);

    checkCudaErrors(cudaStreamCreate(&stream1_), "cudaStreamCreate (stream1_)");
    kernel<<<1, dataSize_, 0, stream1_>>>(dataDevicePointer1_, 4.0);
    checkCudaErrors(
      cudaMemcpyAsync(
        dataHostPointer1_, dataDevicePointer1_, nBytes, cudaMemcpyDeviceToHost,
        stream1_
      ),
      "cudaMemcpyAsync (stream1_)"
    );

    auto op1 = std::make_unique<messaging::AsyncOpCUDA>(stream1_);

    checkCudaErrors(cudaStreamCreate(&stream2_), "cudaStreamCreate (stream2_)");
    kernel<<<1, dataSize_, 0, stream2_>>>(dataDevicePointer2_, 8.0);
    checkCudaErrors(
      cudaMemcpyAsync(
        dataHostPointer2_, dataDevicePointer2_, nBytes, cudaMemcpyDeviceToHost,
        stream2_
      ),
      "cudaMemcpyAsync (stream2_)"
    );

    auto op2 = std::make_unique<messaging::AsyncOpCUDA>(
      stream2_, [this] { done_ = true; }
    );

    // Register these async operations for polling; since these operations are
    // enclosed in an epoch, they should inhibit the current epoch from
    // terminating before they are done.
    theMsg()->registerAsyncOp(std::move(op1));
    theMsg()->registerAsyncOp(std::move(op2));
  }

  void check() {
    for (size_t i = 0; i < dataSize_; ++i) {
      EXPECT_DOUBLE_EQ(dataHostPointer1_[i], 4.0);
      EXPECT_DOUBLE_EQ(dataHostPointer2_[i], 8.0);
    }

    EXPECT_TRUE(done_);
  }

private:
  cudaStream_t stream1_;
  cudaStream_t stream2_;

  double* dataDevicePointer1_;
  double* dataHostPointer1_;

  double* dataDevicePointer2_;
  double* dataHostPointer2_;

  bool done_ = false;

  size_t const dataSize_ = 32;
};

TEST_F(TestAsyncOp, test_async_op_cuda) {
  int driverVer;
  if (
    (cudaDriverGetVersion(&driverVer) == cudaErrorInvalidValue) or
    (driverVer == 0)) {
    GTEST_SKIP()
      << "Trying to run test_async_op_cuda but CUDA driver is not present!\n";
  }

  auto const this_node = theContext()->getNode();
  auto p = theObjGroup()->makeCollective<CUDAGroup>("test_async_op_cuda");
  auto ep = theTerm()->makeEpochRooted(term::UseDS{true});

  // When this returns all the CUDA streams should be done
  runInEpoch(ep, [p, this_node] {
    p[this_node].send<MyMsg, &CUDAGroup::cudaHandler>();
  });

  p[this_node].get()->check();
}
#endif // __CUDACC__

}}} // end namespace vt::tests::unit
