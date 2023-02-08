/*
//@HEADER
// *****************************************************************************
//
//                                 param_msg.h
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

#if !defined INCLUDED_VT_MESSAGING_PARAM_MSG_H
#define INCLUDED_VT_MESSAGING_PARAM_MSG_H

#include "vt/messaging/message/message_serialize.h"

namespace vt { namespace messaging {

template <typename Tuple, typename enabled = void>
struct ParamMsg;

template <typename Tuple>
struct ParamMsg<
  Tuple, std::enable_if_t<is_byte_copyable_t<Tuple>::value>
> : vt::Message
{
  ParamMsg() = default;

  template <typename... Params>
  explicit ParamMsg(Params&&... in_params)
    : params(std::forward<Params>(in_params)...)
  { }

  Tuple params;
  Tuple& getTuple() { return params; }
};

template <typename Tuple>
struct ParamMsg<
  Tuple, std::enable_if_t<not is_byte_copyable_t<Tuple>::value>
> : vt::Message
{
  using MessageParentType = vt::Message;
  vt_msg_serialize_if_needed_by_parent_or_type1(Tuple); // by tup

  ParamMsg() = default;

  template <typename... Params>
  explicit ParamMsg(Params&&... in_params)
    : params(std::make_unique<Tuple>(std::forward<Params>(in_params)...))
  { }

  std::unique_ptr<Tuple> params;

  Tuple& getTuple() { return *params.get(); }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | params;
  }
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_PARAM_MSG_H*/