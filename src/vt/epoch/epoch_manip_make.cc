/*
//@HEADER
// *****************************************************************************
//
//                              epoch_manip_make.cc
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

#include "vt/config.h"
#include "vt/epoch/epoch.h"
#include "vt/epoch/epoch_manip.h"
#include "vt/context/context.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/utils/bits/bits_packer.h"

namespace vt { namespace epoch {

/*static*/ EpochType EpochManip::makeEpoch(
  EpochType const& seq, bool const& is_rooted, NodeType const& root_node,
  bool const& is_user, eEpochCategory const& category
) {
  EpochType new_epoch = 0;
  bool const& has_category = category != eEpochCategory::NoCategoryEpoch;
  EpochManip::setIsRooted(new_epoch, is_rooted);
  EpochManip::setIsUser(new_epoch, is_user);
  EpochManip::setHasCategory(new_epoch, has_category);
  if (is_rooted) {
    vtAssertExpr(root_node != uninitialized_destination);
    EpochManip::setNode(new_epoch, root_node);
  }
  if (has_category) {
    EpochManip::setCategory(new_epoch, category);
  }
  EpochManip::setSeq(new_epoch, seq);
  return new_epoch;
}

EpochType EpochManip::makeNewEpoch(
  bool const& is_rooted, NodeType const& root_node,
  bool const& is_user, eEpochCategory const& category
) {
  if (is_rooted) {
    return makeNewRootedEpoch(is_user,category);
  } else {
    if (cur_non_rooted_ == no_epoch) {
      cur_non_rooted_ = first_epoch;
    }
    auto const new_epoch = makeEpoch(
      cur_non_rooted_,false,uninitialized_destination,is_user,category
    );
    cur_non_rooted_++;
    return new_epoch;
  }
}

/*static*/ EpochType EpochManip::makeRootedEpoch(
  EpochType const& seq, bool const& is_user, eEpochCategory const& category
) {
  auto const& root_node = theContext()->getNode();
  return EpochManip::makeEpoch(seq,true,root_node,is_user,category);
}

EpochType EpochManip::makeNewRootedEpoch(
  bool const& is_user, eEpochCategory const& category
) {
  auto const& root_node = theContext()->getNode();
  if (cur_rooted_ == no_epoch) {
    cur_rooted_ = first_epoch;
  }
  auto const& next_rooted_epoch = EpochManip::makeEpoch(
    cur_rooted_,true,root_node,is_user,category
  );
  cur_rooted_++;
  return next_rooted_epoch;
}

}} /* end namespace vt::epoch */
