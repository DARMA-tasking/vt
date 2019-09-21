/*
//@HEADER
// *****************************************************************************
//
//                                 count_msg.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_RMA_COUNT_MSG_H
#define INCLUDED_VT_VRT_COLLECTION_RMA_COUNT_MSG_H

#include "vt/config.h"
#include "vt/collective/collective_alg.h"

namespace vt { namespace vrt { namespace collection { namespace rma {

template <typename ColT>
struct CountMsg : collective::ReduceTMsg<int> {
  CountMsg(HandleType in_handle, int val)
    : collective::ReduceTMsg<int>(val), handle_(in_handle)
  { }

  HandleType handle() const { return handle_; }

private:
  HandleType handle_ = 0;
};

template <typename ColT, typename T>
struct RankCountMsg : collective::ReduceTMsg<T> {
  RankCountMsg() = default;
  RankCountMsg(HandleType in_handle, T const& val)
    : collective::ReduceTMsg<T>(val), handle_(in_handle)
  { }

  HandleType handle() const { return handle_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    collective::ReduceTMsg<T>::invokeSerialize(s);
  }

private:
  HandleType handle_ = 0;
};

}}}} /* end namespace vt::vrt::collection::rma */

#endif /*INCLUDED_VT_VRT_COLLECTION_RMA_COUNT_MSG_H*/
