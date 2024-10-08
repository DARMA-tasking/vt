/*
//@HEADER
// *****************************************************************************
//
//                            construct_params_msg.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PARAMS_MSG_H
#define INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PARAMS_MSG_H

#include "vt/vrt/collection/param/construct_params.h"

namespace vt { namespace vrt { namespace collection { namespace param {

/**
 * \struct ConstructParamMsg
 *
 * \brief Construct PO configuration message for distributed construction
 */
template <typename ColT>
struct ConstructParamMsg : vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_required(); // po

  ConstructParamMsg() = default;
  explicit ConstructParamMsg(param::ConstructParams<ColT>& in_po)
    : po(std::make_unique<param::ConstructParams<ColT>>(in_po))
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | po;
  }

  /// Must use \c std::unique_ptr here because without the indirection,
  /// AppleClang generates invalid alignment that causes a segfault when \c new
  /// is called on this message type. The only other work around is some
  /// seemingly arbitrary value to alignas (alignas(1024) seems to do the
  /// trick).
  std::unique_ptr<param::ConstructParams<ColT>> po = nullptr;
};

}}}} /* end namespace vt::vrt::collection::param */

#endif /*INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PARAMS_MSG_H*/
