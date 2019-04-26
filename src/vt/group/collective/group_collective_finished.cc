/*
//@HEADER
// ************************************************************************
//
//                          group_collective_finished.cc
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

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/collective/group_info_collective.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/group/group_manager.h"

namespace vt { namespace group {

void InfoColl::CollSetupFinished::operator()(FinishedReduceMsg* msg) {
  debug_print_verbose(
    group, node,
    "CollSetupFinished: group={:x}\n", msg->getGroup()
  );
  auto iter = theGroup()->local_collective_group_info_.find(msg->getGroup());
  vtAssert(
    iter != theGroup()->local_collective_group_info_.end(), "Must exist"
  );
  auto const& this_node = theContext()->getNode();
  auto info = iter->second.get();
  if (info->known_root_node_ != this_node) {
    auto nmsg = makeSharedMessage<GroupOnlyMsg>(
      msg->getGroup(),info->new_tree_cont_
    );
    theMsg()->sendMsg<GroupOnlyMsg,InfoColl::newTreeHan>(
      info->known_root_node_,nmsg
    );
  } else {
    info->newTree(-1);
  }
}

}} /* end namespace vt::group */
