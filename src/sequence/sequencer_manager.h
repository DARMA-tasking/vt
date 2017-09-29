
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE_MANAGER__
#define __RUNTIME_TRANSPORT_SEQUENCE_MANAGER__

#include "config.h"
#include "seq_common.h"
#include "sequencer_fwd.h"

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

#include "sequencer_manager.impl.h"

#endif /*__RUNTIME_TRANSPORT_SEQUENCE_MANAGER__*/
