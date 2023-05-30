/*
//@HEADER
// *****************************************************************************
//
//                             test_async_op_hip.cc
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

#include <vt/messaging/async_op_hip.h>
#include <vt/objgroup/manager.h>

#include <gtest/gtest.h>


namespace vt { namespace tests { namespace unit {

#if __HIPCC__

using TestAsyncOp = TestParallelHarness;

using MyMsg = Message;


inline void checkHipErrors(
  hipError_t err, std::string const& additionalInfo,
  bool skipOnFailure = false
) {
  auto errorMsg = fmt::format(
    "{} failed with error -> {}\n", additionalInfo, hipGetErrorString(err));

  if (hipSuccess != err and skipOnFailure) {
    GTEST_SKIP() << errorMsg;
  }

  vtAbortIf(hipSuccess != err, errorMsg);
}

__global__ void kernel(double* dst, double setVal) {
  int row = blockIdx.y * blockDim.y + threadIdx.y;
  int col = blockIdx.x * blockDim.x + threadIdx.x;
  int tID = row * blockDim.y + col;

  dst[tID] = setVal;
}

struct hipGroup {
  hipGroup() {
    auto const nBytes = dataSize_ * sizeof(double);

    // If first malloc fails, there's probably something wrong with HIP env
    // so call GTEST_SKIP instead of failing the test
    checkHipErrors(
      hipMalloc((void**)&dataDevicePointer1_, nBytes),
      "hipMalloc(dataDevicePointer1_)", true
    );

    checkHipErrors(
      hipHostMalloc((void**)&dataHostPointer1_, nBytes),
      "hipHostMalloc(dataHostPointer1_)"
    );

    checkHipErrors(
      hipMalloc((void**)&dataDevicePointer2_, nBytes),
      "hipMalloc(dataDevicePointer2_)"
    );

    checkHipErrors(
      hipHostMalloc((void**)&dataHostPointer2_, nBytes),
      "hipHostMalloc(dataHostPointer2_)"
    );
  }

  ~hipGroup() {
    checkHipErrors(
      hipFree(dataDevicePointer1_), "hipFree(dataDevicePointer1_)"
    );
    checkHipErrors(
      hipFree(dataDevicePointer2_), "hipFree(dataDevicePointer1_)"
    );

    checkHipErrors(
      hipHostFree(dataHostPointer1_), "hipHostFree(dataHostPointer1_)"
    );
    checkHipErrors(
      hipHostFree(dataHostPointer2_), "hipHostFree(dataHostPointer2_)"
    );

    checkHipErrors(hipStreamDestroy(stream1_), "hipStreamDestroy(stream1_)");
    checkHipErrors(hipStreamDestroy(stream2_), "hipStreamDestroy(stream2_)");
  }

  void hipHandler(MyMsg* msg) {
    auto const nBytes = dataSize_ * sizeof(double);

    checkHipErrors(hipStreamCreate(&stream1_), "hipStreamCreate (stream1_)");
    kernel<<<1, dataSize_, 0, stream1_>>>(dataDevicePointer1_, 4.0);
    checkHipErrors(
      hipMemcpyAsync(
        dataHostPointer1_, dataDevicePointer1_, nBytes, hipMemcpyDeviceToHost,
        stream1_
      ),
      "hipMemcpyAsync (stream1_)"
    );

    auto op1 = std::make_unique<messaging::AsyncOpHIP>(stream1_);

    checkHipErrors(hipStreamCreate(&stream2_), "hipStreamCreate (stream2_)");
    kernel<<<1, dataSize_, 0, stream2_>>>(dataDevicePointer2_, 8.0);
    checkHipErrors(
      hipMemcpyAsync(
        dataHostPointer2_, dataDevicePointer2_, nBytes, hipMemcpyDeviceToHost,
        stream2_
      ),
      "hipMemcpyAsync (stream2_)"
    );

    auto op2 = std::make_unique<messaging::AsyncOpHIP>(
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
  hipStream_t stream1_;
  hipStream_t stream2_;

  double* dataDevicePointer1_;
  double* dataHostPointer1_;

  double* dataDevicePointer2_;
  double* dataHostPointer2_;

  bool done_ = false;

  size_t const dataSize_ = 32;
};

TEST_F(TestAsyncOp, test_async_op_hip) {
  int driverVer;
  if (
    (hipDriverGetVersion(&driverVer) == hipErrorInvalidValue) or
    (driverVer == 0)) {
    GTEST_SKIP()
      << "Trying to run test_async_op_hip but HIP driver is not present!\n";
  }

  auto const this_node = theContext()->getNode();
  auto p = theObjGroup()->makeCollective<hipGroup>("test_async_op_hip");
  auto ep = theTerm()->makeEpochRooted(term::UseDS{true});

  // When this returns all the hip streams should be done
  runInEpoch(ep, [p, this_node] {
    p[this_node].send<MyMsg, &hipGroup::hipHandler>();
  });

  p[this_node].get()->check();
}
#endif // __HIPCC__

}}} // end namespace vt::tests::unit
