
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE_MANAGER_IMPL__
#define __RUNTIME_TRANSPORT_SEQUENCE_MANAGER_IMPL__

#include "config.h"
#include "seq_common.h"

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

#endif /*__RUNTIME_TRANSPORT_SEQUENCE_MANAGER_IMPL__*/
