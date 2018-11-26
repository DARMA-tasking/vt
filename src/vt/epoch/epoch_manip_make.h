/*
//@HEADER
// ************************************************************************
//
//                          epoch_manip_make.h
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

#if !defined INCLUDED_EPOCH_EPOCH_MANIP_MAKE_H
#define INCLUDED_EPOCH_EPOCH_MANIP_MAKE_H

#include "vt/config.h"
#include "vt/epoch/epoch.h"
#include "vt/epoch/epoch_manip.h"
#include "vt/context/context.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/utils/bits/bits_packer.h"

namespace vt { namespace epoch {

/*static*/ inline EpochType EpochManip::makeEpoch(
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

/*static*/ inline EpochType EpochManip::makeNewEpoch(
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

/*static*/ inline EpochType EpochManip::makeRootedEpoch(
  EpochType const& seq, bool const& is_user, eEpochCategory const& category
) {
  auto const& root_node = theContext()->getNode();
  return EpochManip::makeEpoch(seq,true,root_node,is_user,category);
}

/*static*/ inline EpochType EpochManip::makeNewRootedEpoch(
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

/*static*/ inline EpochType EpochManip::next(EpochType const& epoch) {
  return EpochManip::nextFast(epoch);
}

/*static*/ inline EpochType EpochManip::nextSlow(EpochType const& epoch) {
  EpochType new_epoch = epoch;
  auto const& this_seq = EpochManip::seq(epoch);
  EpochManip::setSeq(new_epoch, this_seq + 1);
  return new_epoch;
}

/*static*/ inline EpochType EpochManip::nextFast(EpochType const& epoch) {
  return epoch + 1;
}

}} /* end namespace vt::epoch */

#endif /*INCLUDED_EPOCH_EPOCH_MANIP_MAKE_H*/
