/*
//@HEADER
// ************************************************************************
//
//                          group_manager_active_attorney.h
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

#if !defined INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H
#define INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/messaging/message/message.h"
#include "vt/messaging/message.h"
#include "vt/messaging/active.fwd.h"
#include "vt/messaging/message/smart_ptr.h"

namespace vt { namespace group {

struct GroupActiveAttorney {

  friend struct ::vt::messaging::ActiveMessenger;

private:
  static EventType groupHandler(
    MsgSharedPtr<BaseMsgType> const& msg, NodeType const& from,
    MsgSizeType const& msg_size, bool const is_root, ActionType new_action,
    bool* const deliver
  );
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_MANAGER_ACTIVE_ATTORNEY_H*/
