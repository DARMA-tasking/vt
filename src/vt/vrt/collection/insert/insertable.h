/*
//@HEADER
// *****************************************************************************
//
//                                 insertable.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_INSERT_INSERTABLE_H
#define INCLUDED_VT_VRT_COLLECTION_INSERT_INSERTABLE_H

#include "vt/config.h"
#include "vt/vrt/collection/send/sendable.h"
#include "vt/vrt/collection/insert/modify_token.h"
#include "vt/messaging/message/smart_ptr.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct ElmInsertable : BaseProxyT {
  ElmInsertable() = default;
  ElmInsertable(
    typename BaseProxyT::ProxyType const& in_proxy,
    typename BaseProxyT::ElementProxyType const& in_elm
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  /**
   * \brief Insert a new collection element (calling the default
   * constructor). Collective must have dynamic membership to perform
   * insertions. The element will be inserted on the node where the initial map
   * puts it.
   *
   * \param[in] token the modifier token
   */
  void insert(ModifierToken& token) const;

  /**
   * \brief Insert a new collection element (calling the default
   * constructor). Collective must have dynamic membership to perform
   * insertions.
   *
   * \param[in] token the modifier token
   * \param[in] node the node to insert on
   */
  void insertAt(ModifierToken& token, NodeType node) const;

  /**
   * \brief Insert a new collection element, calling the constructor that takes
   * a message as an argument. Collection must have dynamic membership to
   * perform insertions.
   *
   * \param[in] token the modifier token
   * \param[in] msg the message
   */
  template <typename MsgT>
  void insertMsg(ModifierToken& token, messaging::MsgPtrThief<MsgT> msg) const;

  /**
   * \brief Insert a new collection element, calling the constructor that takes
   * a message as an argument. Collection must have dynamic membership to
   * perform insertions.
   *
   * \param[in] token the modifier token
   * \param[in] node the node to insert on
   * \param[in] msg the message
   */
  template <typename MsgT>
  void insertAtMsg(
    ModifierToken& token, NodeType node, messaging::MsgPtrThief<MsgT> msg
  ) const;

  /**
   * \brief Destroy the collection element
   *
   * \param[in] token the modifier token
   */
  void destroy(ModifierToken& token) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_INSERT_INSERTABLE_H*/
