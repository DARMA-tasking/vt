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
#include "vt/termination/term_common.h"

#include <fmt/ostream.h>

namespace vt { namespace epoch {

static EpochType const arch_epoch_coll = 0ull;

EpochManip::EpochManip()
  : terminated_collective_epochs_(
      std::make_unique<EpochWindow>(arch_epoch_coll)
    )
{ }

/*static*/ EpochType EpochManip::generateEpoch(
  bool const& is_rooted, NodeType const& root_node,
  eEpochCategory const& category
) {
  EpochType new_epoch = 0;
  bool const& has_category = category != eEpochCategory::NoCategoryEpoch;
  EpochManip::setIsRooted(new_epoch, is_rooted);

  if (has_category) {
    EpochManip::setCategory(new_epoch, category);
  }

  if (is_rooted) {
    vtAssertExpr(root_node != uninitialized_destination);
    EpochManip::setNode(new_epoch, root_node);
  }

  // Set sequence ID to 0--this is the archetypical epoch with just control bits
  // initialized
  EpochManip::setSeq(new_epoch, 0);

  return new_epoch;
}

/*static*/ EpochType EpochManip::generateRootedEpoch(
  eEpochCategory const& category
) {
  auto const root_node = theContext()->getNode();
  return generateEpoch(true,root_node,category);
}

EpochType EpochManip::getNextCollectiveEpoch(
  eEpochCategory const& category
) {
  auto const no_dest = uninitialized_destination;
  auto const arch_epoch = generateEpoch(false,no_dest,category);
  return getTerminatedWindow(arch_epoch)->allocateNewEpoch();
}

EpochType EpochManip::getNextRootedEpoch(
  eEpochCategory const& category
) {
  auto const root_node = theContext()->getNode();
  auto const arch_epoch = getNextRootedEpoch(category, root_node);
  return getTerminatedWindow(arch_epoch)->allocateNewEpoch();
}

EpochType EpochManip::getNextRootedEpoch(
  eEpochCategory const& category, NodeType const root_node
) {
  auto const arch_epoch = generateEpoch(true,root_node,category);
  return getTerminatedWindow(arch_epoch)->allocateNewEpoch();
}

EpochType EpochManip::getArchetype(EpochType epoch) const {
  auto epoch_arch = epoch;
  epoch::EpochManip::setSeq(epoch_arch,0);
  return epoch_arch;
}

EpochWindow* EpochManip::getTerminatedWindow(EpochType epoch) {
  auto const is_rooted = isRooted(epoch);
  if (is_rooted and epoch != term::any_epoch_sentinel) {
    auto const& arch_epoch = getArchetype(epoch);
    auto iter = terminated_epochs_.find(arch_epoch);
    if (iter == terminated_epochs_.end()) {
      terminated_epochs_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(arch_epoch),
        std::forward_as_tuple(std::make_unique<EpochWindow>(arch_epoch))
      );
      iter = terminated_epochs_.find(arch_epoch);
    }
    return iter->second.get();
  } else {
    vtAssertExpr(terminated_collective_epochs_ != nullptr);
    return terminated_collective_epochs_.get();
  }
}

/*static*/ bool EpochManip::isRooted(EpochType const& epoch) {
  constexpr BitPackerType::FieldType field = eEpochRoot::rEpochIsRooted;
  constexpr BitPackerType::FieldType size = 1;
  return BitPackerType::boolGetField<field,size,EpochType>(epoch);
}

/*static*/ eEpochCategory EpochManip::category(EpochType const& epoch) {
  return BitPackerType::getField<
    eEpochRoot::rEpochCategory, epoch_category_num_bits, eEpochCategory
  >(epoch);
}

/*static*/ NodeType EpochManip::node(EpochType const& epoch) {
  vtAssert(isRooted(epoch), "Must be rooted to manipulate the node");

  return BitPackerType::getField<
    eEpochRoot::rEpochNode, node_num_bits, NodeType
  >(epoch);
}

/*static*/ EpochType EpochManip::seq(EpochType const& epoch) {
  if (isRooted(epoch)) {
    return BitPackerType::getField<
      eEpochRoot::rEpochSequential, epoch_seq_root_num_bits, EpochType
    >(epoch);
  } else {
    return BitPackerType::getField<
      eEpochColl::cEpochSequential, epoch_seq_coll_num_bits, EpochType
    >(epoch);
  }
}

/*static*/
void EpochManip::setIsRooted(EpochType& epoch, bool const is_rooted) {
  BitPackerType::boolSetField<eEpochRoot::rEpochIsRooted,1,EpochType>(
    epoch, is_rooted
  );
}

/*static*/
void EpochManip::setCategory(EpochType& epoch, eEpochCategory const cat) {
  BitPackerType::setField<
    eEpochRoot::rEpochCategory, epoch_category_num_bits
  >(epoch,cat);
}

/*static*/
void EpochManip::setNode(EpochType& epoch, NodeType const node) {
  vtAssert(isRooted(epoch), "Must be rooted to manipulate the node");

  BitPackerType::setField<eEpochRoot::rEpochNode, node_num_bits>(epoch,node);
}

/*static*/
void EpochManip::setSeq(EpochType& epoch, EpochType const seq) {
  if (isRooted(epoch)) {
    BitPackerType::setField<
      eEpochRoot::rEpochSequential, epoch_seq_root_num_bits
    >(epoch,seq);
  } else {
    BitPackerType::setField<
      eEpochColl::cEpochSequential, epoch_seq_coll_num_bits
    >(epoch,seq);
  }
}

}} /* end namespace vt::epoch */
