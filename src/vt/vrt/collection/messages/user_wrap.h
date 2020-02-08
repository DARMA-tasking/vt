/*
//@HEADER
// *****************************************************************************
//
//                                 user_wrap.h
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

#if !defined INCLUDED_VRT_COLLECTION_MESSAGES_USER_WRAP_H
#define INCLUDED_VRT_COLLECTION_MESSAGES_USER_WRAP_H

#include "vt/config.h"
#include "vt/topos/location/message/msg.h"
#include "vt/messaging/message.h"
#include "vt/vrt/collection/messages/user.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/vrt_common.h"

#if HAS_SERIALIZATION_LIBRARY
  #define HAS_DETECTION_COMPONENT 1
  #include "serialization_library_headers.h"
  #include "traits/serializable_traits.h"
#endif

#include <type_traits>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename UserMsgT, typename BaseMsgT = ::vt::Message>
struct ColMsgWrap : CollectionMessage<ColT,BaseMsgT> {
  using MessageParentType = CollectionMessage<ColT,BaseMsgT>;
  vt_msg_serialize_if_needed_by_parent_or_type1(UserMsgT);

  using UserMsgType = UserMsgT;

  ColMsgWrap() = default;

  explicit ColMsgWrap(UserMsgT&& msg)
    : CollectionMessage<ColT,BaseMsgT>(ColMsgWrapTag),
      msg_(std::move(msg))
  { }

  explicit ColMsgWrap(UserMsgT const& msg)
    : CollectionMessage<ColT,BaseMsgT>(ColMsgWrapTag),
      msg_(msg)
  { }

  UserMsgT& getMsg() { return msg_; }

  // TODO - does not align with 'serialize if needed' above.
  template <
    typename SerializerT,
    typename T=void,
    typename = typename std::enable_if<
      ::serdes::SerializableTraits<UserMsgT>::has_serialize_function, T
    >::type
  >
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | msg_;
  }

private:
  UserMsgT msg_;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_WRAP_H*/
