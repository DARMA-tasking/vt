/*
//@HEADER
// *****************************************************************************
//
//                                epoch_manip.cc
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

#include <fmt/ostream.h>

namespace vt { namespace epoch {

EpochManip::EpochManip()
  : live_scopes_(no_scope)
{ }

/*static*/ EpochType EpochManip::generateEpoch(
  EpochType const& seq, bool const& is_rooted, NodeType const& root_node,
  EpochScopeType const& scope, eEpochCategory const& category
) {
  EpochType new_epoch = 0;
  bool const& has_category = category != eEpochCategory::NoCategoryEpoch;
  EpochManip::setIsRooted(new_epoch, is_rooted);

  // Compose in the high bits of the sequence epoch ID a scope (only actually
  // impacts the value if not global scope). Use the \c scope_limit to
  // determine how many bits are reserved.
  EpochManip::setScope(new_epoch, scope);

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

/*static*/ EpochType EpochManip::generateRootedEpoch(
  EpochType const& seq, EpochScopeType const& scope,
  eEpochCategory const& category
) {
  auto const& root_node = theContext()->getNode();
  return EpochManip::generateEpoch(seq,true,root_node,scope,category);
}

EpochType EpochManip::getNextCollectiveEpoch(
  EpochScopeType const scope, eEpochCategory const& category
) {
  auto const new_epoch = generateEpoch(
    nextSeqCollective(scope),false,uninitialized_destination,scope,category
  );
  return new_epoch;
}

EpochType EpochManip::getNextRootedEpoch(
  eEpochCategory const& category, EpochScopeType const scope
) {
  auto const& root_node = theContext()->getNode();
  return getNextRootedEpoch(category, scope, root_node);
}

EpochType EpochManip::getNextRootedEpoch(
  eEpochCategory const& category, EpochScopeType const scope,
  NodeType const root_node
) {
  auto const& next_rooted_epoch = EpochManip::generateEpoch(
    nextSeqRooted(scope),true,root_node,scope,category
  );
  return next_rooted_epoch;
}

EpochType EpochManip::nextSeqRooted(EpochScopeType scope) {
  return nextSeq(scope, false);
}

EpochType EpochManip::nextSeqCollective(EpochScopeType scope) {
  return nextSeq(scope, true);
}

EpochType EpochManip::nextSeq(EpochScopeType scope, bool is_collective) {
  if (is_collective) {
    auto& scope_map = scope_collective_;
    if (scope_map.find(scope) == scope_map.end()) {
      scope_map[scope] = first_epoch;
    }
    auto const seq = scope_map[scope];
    scope_map[scope]++;
    return seq;
  } else {
    // for rooted, it's simple: get the next one and return it.
    auto const seq = next_rooted_;
    next_rooted_++;
    return seq;
  }
}

EpochCollectiveScope EpochManip::makeScopeCollective() {
  // We have \c scope_limit scopes available, not including the global scope
  vtAbortIf(live_scopes_.size() >= scope_limit, "Must have space for new scope");

  static constexpr EpochScopeType const first_scope = 1;

  // if empty, go with the first scope
  EpochScopeType next = first_scope;

  if (not live_scopes_.empty()) {
    if (live_scopes_.upper() < scope_limit) {
      next = live_scopes_.upper() + 1;
    } else if (live_scopes_.lower() > 1) {
      next = live_scopes_.lower() - 1;
    } else {
      // fall back to just searching the integral set for one that is not live
      EpochScopeType s = first_scope;
      do {
        if (not live_scopes_.exists(s)) {
          next = s;
          break;
        }
        s++;
      } while (s <= scope_limit);
    }
  }

  vtAssert(next >= 1, "Scope must be greater than 0");
  vtAssert(next < scope_limit + 1, "Scope must be less than the scope limit");
  vtAssert(not live_scopes_.exists(next), "Scope must not already exist");

  // insert the scope to track it
  live_scopes_.insert(next);

  return EpochCollectiveScope{next};
}

void EpochManip::destroyScope(EpochScopeType scope) {
  vtAssert(live_scopes_.exists(scope), "Scope must exist to destroy");
  live_scopes_.erase(scope);
  // Very important: explicitly don't clear the scope map (\c scope_collective_)
  // because if we did we would have to wait for termination of all epochs
  // within the scope before it could be destroyed. Thus, scopes can be quickly
  // created and destroyed by the user.
}

/*static*/ bool EpochManip::isRooted(EpochType const& epoch) {
  constexpr BitPackerType::FieldType field = eEpochLayout::EpochIsRooted;
  constexpr BitPackerType::FieldType size = 1;
  return BitPackerType::boolGetField<field,size,EpochType>(epoch);
}

/*static*/ bool EpochManip::hasCategory(EpochType const& epoch) {
  return BitPackerType::boolGetField<eEpochLayout::EpochHasCategory>(epoch);
}

/*static*/ eEpochCategory EpochManip::category(EpochType const& epoch) {
  return BitPackerType::getField<
    eEpochLayout::EpochCategory, epoch_category_num_bits, eEpochCategory
  >(epoch);
}

/*static*/ NodeType EpochManip::node(EpochType const& epoch) {
  return BitPackerType::getField<
    eEpochLayout::EpochNode, node_num_bits, NodeType
  >(epoch);
}

/*static*/ EpochType EpochManip::seq(EpochType const& epoch) {
  return BitPackerType::getField<
    eEpochLayout::EpochSequential, epoch_seq_num_bits, EpochType
  >(epoch);
}

/*static*/ EpochScopeType EpochManip::getScope(EpochType const& epoch) {
  // constexpr EpochScopeType offset = epoch_seq_num_bits - scope_limit;
  return BitPackerType::getField<
    eEpochLayout::EpochScope, scope_bits, EpochScopeType
  >(epoch);
}

/*static*/
void EpochManip::setIsRooted(EpochType& epoch, bool const is_rooted) {
  BitPackerType::boolSetField<eEpochLayout::EpochIsRooted,1,EpochType>(epoch,is_rooted);
}

/*static*/
void EpochManip::setHasCategory(EpochType& epoch, bool const has_cat) {
  BitPackerType::boolSetField<eEpochLayout::EpochHasCategory,1,EpochType>(
    epoch,has_cat
  );
}

/*static*/
void EpochManip::setScope(EpochType& epoch, EpochScopeType const scope) {
  BitPackerType::setField<eEpochLayout::EpochScope,scope_bits>(epoch,scope);
}

/*static*/
void EpochManip::setCategory(EpochType& epoch, eEpochCategory const cat) {
  BitPackerType::setField<
    eEpochLayout::EpochCategory, epoch_category_num_bits
  >(epoch,cat);
}

/*static*/
void EpochManip::setNode(EpochType& epoch, NodeType const node) {
  BitPackerType::setField<eEpochLayout::EpochNode, node_num_bits>(epoch,node);
}

/*static*/
void EpochManip::setSeq(EpochType& epoch, EpochType const seq) {
  BitPackerType::setField<
    eEpochLayout::EpochSequential, epoch_seq_num_bits
  >(epoch,seq);
}

}} /* end namespace vt::epoch */
