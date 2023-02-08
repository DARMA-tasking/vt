/*
//@HEADER
// *****************************************************************************
//
//                                async_op_hip.h
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

#if !defined INCLUDED_VT_MESSAGING_ASYNC_OP_HIP_H
#define INCLUDED_VT_MESSAGING_ASYNC_OP_HIP_H

#include "vt/messaging/async_op.h"

#if __HIPCC__
#include <hip/hip_runtime.h>
#endif

namespace vt { namespace messaging {

#if __HIPCC__

/**
 * \struct AsyncOpHIP
 *
 * \brief An asynchronous HIP event on which VT can poll completion.
 */
struct AsyncOpHIP : AsyncOp {

  /**
   * \brief Construct with stream
   *
   * \param[in] in_stream the HIP stream to generate an event from
   * \param[in] in_cont the action to execute when event completes
   */
  AsyncOpHIP(hipStream_t in_stream, ActionType in_cont = nullptr)
    : cont_(in_cont)
  {
    vtAbortIf(hipSuccess != hipEventCreate(&event_), "Failed to create event");
    vtAbortIf(hipSuccess != hipEventRecord(event_, in_stream), "Failed to record event");
  }

  /**
   * \brief Construct with an event
   *
   * \param[in] in_event the HIP event to poll
   * \param[in] in_cont the action to execute when event completes
   */
  AsyncOpHIP(hipEvent_t in_event, ActionType in_cont = nullptr)
    : event_(in_event),
      cont_(in_cont)
  { }

  /**
   * \brief Poll completion of the HIP event
   *
   * \return whether the HIP event is complete
   */
  bool poll() override {
    auto ret = hipEventQuery(event_);
    if (hipSuccess != ret && hipErrorNotReady != ret) {
      vtAbort(fmt::format("Failure on stream event: {}: {}", hipGetErrorName(ret), hipGetErrorString(ret)));
    }
    return ret == hipSuccess;
  }

  /**
   * \brief Trigger continuation after completion
   */
  void done() override {
    vtAbortIf(hipSuccess != hipEventDestroy(event_), "Failed to destroy event");
    if (cont_) {
      cont_();
    }
  }

private:
  hipEvent_t event_ = {};        /**< The HIP event being tested */
  ActionType cont_ = nullptr;     /**< The continuation after event completes */
};

#endif /* __HIPCC__ */

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_ASYNC_OP_HIP_H*/
