/*
//@HEADER
// *****************************************************************************
//
//                               async_op_cuda.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_MESSAGING_ASYNC_OP_CUDA_H
#define INCLUDED_VT_MESSAGING_ASYNC_OP_CUDA_H

#include "vt/messaging/async_op.h"

namespace vt { namespace messaging {

#if __CUDACC__

/**
 * \struct AsyncOpCUDA
 *
 * \brief A asynchronous CUDA event that VT can poll completion for.
 */
struct AsyncOpCUDA : AsyncOp {

  /**
   * \brief Construct with stream
   *
   * \param[in] in_stream the CUDA stream to generate an event from
   * \param[in] in_cont the action to execute when event completes
   */
  AsyncOpCUDA(cudaStream_t in_stream, ActionType in_cont = nullptr)
    : cont_(in_cont)
  {
    cudaEventCreate(&event_);
    cudaEventRecord(event_, in_stream);
  }

  /**
   * \brief Construct with an event
   *
   * \param[in] in_event the CUDA event to poll
   * \param[in] in_cont the action to execute when event completes
   */
  AsyncOpCUDA(cudaEvent_t in_event, ActionType in_cont = nullptr)
    : event_(in_event),
      cont_(in_cont)
  { }

  /**
   * \brief Poll completion of the CUDA event
   *
   * \return whether the CUDA event is complete
   */
  bool poll() override {
    return cudaEventQuery(event_) == cudaSuccess;
  }

  /**
   * \brief Trigger continuation after completion
   */
  void done() override {
    if (cont_) {
      cont_();
    }
  }

private:
  cudaEvent_t event_ = {};        /**< The CUDA event being tested */
  ActionType cont_ = nullptr;     /**< The continuation after event completes */
};

#endif /*__CUDACC__*/

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_ASYNC_OP_CUDA_H*/
