
#if !defined INCLUDED_SEQUENCE_SEQ_NODE_FWD_H
#define INCLUDED_SEQUENCE_SEQ_NODE_FWD_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_types.h"

namespace vt { namespace seq {

struct SeqNode;
using SeqNodePtrType = std::shared_ptr<SeqNode>;

template <typename Fn>
bool executeSeqExpandContext(SeqType const& id, SeqNodePtrType node, Fn&& fn);

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_NODE_FWD_H*/

