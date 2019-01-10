/*
//@HEADER
// ************************************************************************
//
//                          user_wrap.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
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
  using UserMsgType = UserMsgT;

  ColMsgWrap() = default;

  explicit ColMsgWrap(UserMsgT&& msg)
    : msg_(std::move(msg)),
      CollectionMessage<ColT,BaseMsgT>(ColMsgWrapTag)
  { }

  explicit ColMsgWrap(UserMsgT const& msg)
    : msg_(msg),
      CollectionMessage<ColT,BaseMsgT>(ColMsgWrapTag)
  { }

  UserMsgT& getMsg() { return msg_; }

  template <typename SerializerT>
  void serializeParent(SerializerT& s) {
    CollectionMessage<ColT,BaseMsgT>::serializeParent(s);
    CollectionMessage<ColT,BaseMsgT>::serializeThis(s);
  }

  template <typename SerializerT>
  void serializeThis(SerializerT& s) {
    s | msg_;
  }

  template <
    typename SerializerT,
    typename T=void,
    typename = typename std::enable_if<
      ::serdes::SerializableTraits<UserMsgT>::has_serialize_function, T
    >::type
  >
  void serialize(SerializerT& s) {
    serializeParent(s);
    serializeThis(s);
  }

private:
  UserMsgT msg_;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MESSAGES_USER_WRAP_H*/
