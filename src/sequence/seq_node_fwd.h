
#if ! defined __RUNTIME_TRANSPORT_SEQ_NODE_FWD__
#define __RUNTIME_TRANSPORT_SEQ_NODE_FWD__

#include "config.h"
#include "seq_common.h"
#include "seq_types.h"

namespace vt { namespace seq {

struct SeqNode;
using SeqNodePtrType = std::shared_ptr<SeqNode>;

template <typename Fn>
bool executeSeqExpandContext(SeqType const& id, SeqNodePtrType node, Fn&& fn);

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_NODE_FWD__*/

