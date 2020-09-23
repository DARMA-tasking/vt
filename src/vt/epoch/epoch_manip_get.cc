/*
//@HEADER
// *****************************************************************************
//
//                              epoch_manip_get.cc
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
#include "vt/utils/bits/bits_common.h"
#include "vt/utils/bits/bits_packer.h"

namespace vt { namespace epoch {

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

}} /* end namespace vt::epoch */
