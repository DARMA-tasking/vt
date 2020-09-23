/*
//@HEADER
// *****************************************************************************
//
//                              epoch_manip_set.cc
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
