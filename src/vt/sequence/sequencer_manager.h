
#if !defined INCLUDED_SEQUENCE_SEQUENCER_MANAGER_H
#define INCLUDED_SEQUENCE_SEQUENCER_MANAGER_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/sequencer_fwd.h"

#include <limits>

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct SeqManager {
  using SeqType = SeqTag;

  SeqType nextSeqID(bool const is_virtual_seq);
  bool isVirtual(SeqType const& id) const;

private:
  static constexpr SeqType const start_nrm_seq = initial_seq;
  static constexpr SeqType const start_vrt_seq = std::numeric_limits<SeqType>::max() / 2;

  SeqType next_nrm_id = start_nrm_seq;
  SeqType next_vrt_id = start_vrt_seq;
};

}} // end namespace vt::seq

#include "vt/sequence/sequencer_manager.impl.h"

#endif /*INCLUDED_SEQUENCE_SEQUENCER_MANAGER_H*/
