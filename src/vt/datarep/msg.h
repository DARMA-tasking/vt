/*
//@HEADER
// *****************************************************************************
//
//                                    msg.h
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

#if !defined INCLUDED_VT_DATAREP_MSG_H
#define INCLUDED_VT_DATAREP_MSG_H

#include "vt/topos/location/message/msg.h"

namespace vt { namespace datarep { namespace detail {

template <typename T>
struct DataRequestMsg : LocationRoutedMsg<DataRepIDType, vt::Message> {
  using MessageParentType = vt::Message;
  vt_msg_serialize_prohibited();

  DataRequestMsg(NodeType in_requestor, DataRepIDType in_handle_id)
    : requestor_(in_requestor),
      handle_id_(in_handle_id)
  { }

  NodeType requestor_ = uninitialized_destination;
  DataRepIDType handle_id_ = no_datarep;
};

template <typename T>
struct DataResponseMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_if_needed_by_parent_or_type1(T);

  DataResponseMsg() = default; // for serializer
  DataResponseMsg(DataRepIDType in_handle_id, T const& data)
    : handle_id_(in_handle_id),
      data_(std::make_unique<T>(data))
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | handle_id_;
    s | data_;
  }

  DataRepIDType handle_id_ = no_datarep;
  std::unique_ptr<T> data_;
};


}}} /* end namespace vt::datarep::detail */

#endif /*INCLUDED_VT_DATAREP_MSG_H*/
