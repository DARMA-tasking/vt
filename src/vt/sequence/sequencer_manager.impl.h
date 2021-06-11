/*
//@HEADER
// *****************************************************************************
//
//                           sequencer_manager.impl.h
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

#if !defined INCLUDED_SEQUENCE_SEQUENCER_MANAGER_IMPL_H
#define INCLUDED_SEQUENCE_SEQUENCER_MANAGER_IMPL_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
typename SeqManager<SeqTag, SeqTrigger>::SeqType
SeqManager<SeqTag, SeqTrigger>::nextSeqID(
  bool const is_virtual_seq
) {
  if (is_virtual_seq) {
    return next_vrt_id++;
  } else {
    return next_nrm_id++;
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
bool SeqManager<SeqTag, SeqTrigger>::isVirtual(SeqType const& id) const {
  if (id >= start_vrt_seq) {
    return true;
  } else {
    return false;
  }
}

}} // end namespace vt::seq

#endif /*INCLUDED_SEQUENCE_SEQUENCER_MANAGER_IMPL_H*/
