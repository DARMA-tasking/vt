/*
//@HEADER
// ************************************************************************
//
//                          dispatch_functor.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_FUNCTOR_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_FUNCTOR_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/serialization/serialize_interface.h"
#include "vt/utils/static_checks/functor.h"

#if HAS_SERIALIZATION_LIBRARY
  #define HAS_DETECTION_COMPONENT 1
  #include "serialization_library_headers.h"
  #include "traits/serializable_traits.h"
#endif

namespace vt { namespace serialization { namespace auto_dispatch {

template <typename FunctorT, typename MsgT>
struct SenderFunctor {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
  );
};

template <typename FunctorT, typename MsgT>
struct SenderSerializeFunctor {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
  );
  static EventType sendMsgParserdes(
    NodeType const& node, MsgT* msg, TagType const& tag, ActionType action
  );
};

template <typename FunctorT, typename MsgT>
struct BroadcasterFunctor {
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag, ActionType action
  );
};

template <typename FunctorT, typename MsgT>
struct BroadcasterSerializeFunctor {
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag, ActionType action
  );
  static EventType broadcastMsgParserdes(
    MsgT* msg, TagType const& tag, ActionType action
  );
};


template <typename FunctorT, typename MsgT, typename=void>
struct RequiredSerializationFunctor {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return SenderFunctor<FunctorT,MsgT>::sendMsg(node,msg,tag,action);
  }
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return BroadcasterFunctor<FunctorT,MsgT>::broadcastMsg(msg,tag,action);
  }
};

#if HAS_SERIALIZATION_LIBRARY

template <typename FunctorT, typename MsgT>
struct RequiredSerializationFunctor<
  FunctorT, MsgT,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::has_serialize_function
  >
> {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return SenderSerializeFunctor<FunctorT,MsgT>::sendMsg(node,msg,tag,action);
  }
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return BroadcasterSerializeFunctor<FunctorT,MsgT>::broadcastMsg(
      msg,tag,action
    );
  }
};

template <typename FunctorT, typename MsgT>
struct RequiredSerializationFunctor<
  FunctorT, MsgT,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::is_parserdes
  >
> {
  static EventType sendMsg(
    NodeType const& node, MsgT* msg, TagType const& tag = no_tag,
    ActionType action = nullptr
  ) {
    return SenderSerializeFunctor<FunctorT,MsgT>::sendMsgParserdes(
      node,msg,tag,action
    );
  }
  static EventType broadcastMsg(
    MsgT* msg, TagType const& tag = no_tag, ActionType action = nullptr
  ) {
    return BroadcasterSerializeFunctor<FunctorT,MsgT>::broadcastMsgParserdes(
      msg,tag,action
    );
  }
};

#endif

}}} /* end namespace vt::serialization::auto_dispatch */

namespace vt {

template <
  typename FunctorT,
  typename MsgT = typename util::FunctorExtractor<FunctorT>::MessageType
>
using ActiveSendFunctor =
  serialization::auto_dispatch::RequiredSerializationFunctor<FunctorT,MsgT>;

} /* end namespace vt */

#include "vt/serialization/auto_dispatch/dispatch_handler.impl.h"

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_FUNCTOR_H*/
