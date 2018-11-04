
#if !defined INCLUDED_SEQUENCE_SEQ_LIST_H
#define INCLUDED_SEQUENCE_SEQ_LIST_H

#include <list>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_node.h"

namespace vt { namespace seq {

struct SeqList {
  using SeqFunType = SystemSeqFunType;
  using SeqNodeType = SeqNode;
  using SeqNodeStateEnumType = eSeqNodeState;

  explicit SeqList(SeqType const& seq_id_in);

  void addAction(SeqFunType const& fn);
  void addNode(SeqNodePtrType node);
  void expandNextNode();
  void makeReady();
  bool isReady() const;
  SeqType getSeq() const;

private:
  SeqType seq_id_ = no_seq;

  bool ready_ = true;

  SeqNodePtrType node_ = nullptr;
};

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_LIST_H*/
