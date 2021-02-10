/*
//@HEADER
// *****************************************************************************
//
//                               subphase_msgs.h
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

#if !defined INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_MSGS_H
#define INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_MSGS_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_sentinels.h"
#include "vt/messaging/message.h"

#include <string>

namespace vt { namespace phase { namespace subphase {

/**
 * \internal
 * \struct SubphaseIDMsg
 *
 * \brief Message sent back with the resolved subphase ID
 */
struct SubphaseIDMsg : Message {
  explicit SubphaseIDMsg(SubphaseType in_id)
    : id_(in_id)
  { }

  SubphaseType id_ = no_lb_phase; /**< The resolved ID */
};

/**
 * \internal
 * \struct RootedStringMsg
 *
 * \brief Request message to broker to resolve the subphase ID for a
 * label. Contains a callback that is triggered once resolution occurs.
 */
struct RootedStringMsg : Message {
  using MessageParentType = Message;

  vt_msg_serialize_required(); // std::string

  RootedStringMsg() = default;

  /**
   * \brief Construct the label message for broker
   *
   * \param[in] in_subphase the label
   * \param[in] in_cb the callback to trigger after resolution
   */
  RootedStringMsg(
    std::string const& in_subphase, vt::Callback<SubphaseIDMsg> in_cb
  ) : subphase_(in_subphase),
      cb_(in_cb)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | subphase_ | cb_;
  }

  std::string subphase_ = "";       /**< the label */
  vt::Callback<SubphaseIDMsg> cb_;  /**< the callback to trigger */
};

}}} /* end namespace vt::phase::subphase */

#endif /*INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_MSGS_H*/
