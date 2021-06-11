/*
//@HEADER
// *****************************************************************************
//
//                              async_op_wrapper.h
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

#if !defined INCLUDED_VT_MESSAGING_ASYNC_OP_WRAPPER_H
#define INCLUDED_VT_MESSAGING_ASYNC_OP_WRAPPER_H

#include "vt/messaging/async_op.h"

#include <memory>

namespace vt { namespace messaging {

/**
 * \internal
 * \struct AsyncOpWrapper
 *
 * \brief Wrapper for a general asynchronous operation that holds a pointer to
 * base class.
 */
struct AsyncOpWrapper {

  /**
   * \internal \brief Construct with unique pointer to operation
   *
   * \param[in] ptr the operation
   */
  explicit AsyncOpWrapper(std::unique_ptr<AsyncOp> ptr)
    : valid(true),
      op_(std::move(ptr))
  { }

  /**
   * \internal \brief Construct with unique pointer to operation and a thread ID
   * to resume when it completes
   *
   * \param[in] ptr the operation
   * \param[in] tid the thread ID to resume after completion
   */
  AsyncOpWrapper(std::unique_ptr<AsyncOp> ptr, ThreadIDType in_tid)
    : valid(true),
      op_(std::move(ptr)),
      tid_(in_tid)
  { }

  /**
   * \internal \brief Test completion of the operation
   *
   * \param[in] num_tests how many tests were executed (diagnostics)
   *
   * \return whether the operation is complete
   */
  bool test(int& num_tests);

  /**
   * \internal \brief Trigger continuation after operation completes
   */
  void done();

  /**
   * \brief Serializer for footprinting
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | valid | op_ | tid_;
  }

public:
  bool valid = false;                         /**< Whether op is valid  */

private:
  std::unique_ptr<AsyncOp> op_ = nullptr;     /**< The enclosed operation  */
  ThreadIDType tid_ = no_thread_id;           /**< The thread ID to resume */
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_ASYNC_OP_WRAPPER_H*/
