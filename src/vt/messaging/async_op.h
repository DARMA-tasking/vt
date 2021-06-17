/*
//@HEADER
// *****************************************************************************
//
//                                  async_op.h
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

#if !defined INCLUDED_VT_MESSAGING_ASYNC_OP_H
#define INCLUDED_VT_MESSAGING_ASYNC_OP_H

#include "vt/configs/types/types_headers.h"

namespace vt { namespace messaging {

/**
 * \struct AsyncOp
 *
 * \brief A general asynchronous operation with a pure virtual polling function
 * and an associated trigger to execute when it completes.
 */
struct AsyncOp {

  /**
   * \brief Construct an \c AsyncOp tracking the current epoch
   */
  AsyncOp();
  AsyncOp(AsyncOp&&);
  AsyncOp(AsyncOp const&) = delete;
  AsyncOp& operator=(AsyncOp&&) = delete;
  AsyncOp& operator=(AsyncOp const&) = delete;

  virtual ~AsyncOp();

  /**
   * \brief Function to override for polling completion of this asynchronous
   * operation.
   *
   * \return whether the operation is completed
   */
  virtual bool poll() = 0;

  /**
   * \brief Function to override that is triggered when the operation completes
   */
  virtual void done() = 0;

  /**
   * \brief Serialize for footprinting
   *
   * \note Virtual serialization is not used here so some memory might be
   * uncounted in inherited classes
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | cur_epoch_;
  }

protected:
  EpochType cur_epoch_ = no_epoch; /**< Enclosing epoch for the operation */
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_ASYNC_OP_H*/
