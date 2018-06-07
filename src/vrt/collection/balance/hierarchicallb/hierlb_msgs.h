
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_MSGS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_MSGS_H

#include "config.h"
#include "vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vrt/collection/balance/proc_stats.h"
#include "messaging/message.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

struct LBTreeUpMsg : HierLBTypes, ::vt::Message {
  using LoadType = double;

  LBTreeUpMsg() = default;
  LBTreeUpMsg(
    LoadType const in_child_load, NodeType const in_child,
    ObjSampleType in_load, NodeType const in_child_size
  ) : child_load_(in_child_load), child_(in_child), load_(in_load),
      child_size_(in_child_size)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | child_load_ | child_ | load_ | child_size_;
  }

  LoadType getChildLoad() const { return child_load_; }
  NodeType getChild() const { return child_; }
  ObjSampleType const& getLoad() const { return load_; }
  ObjSampleType&& getLoadMove() { return std::move(load_); }
  NodeType getChildSize() const { return child_size_; }

private:
  LoadType child_load_ = 0.0f;
  NodeType child_ = uninitialized_destination;
  ObjSampleType load_;
  NodeType child_size_ = 0;
};

struct LBTreeDownMsg : HierLBTypes, ::vt::Message {
  using LoadType = double;

  LBTreeDownMsg() = default;
  LBTreeDownMsg(
    NodeType const in_from, ObjSampleType in_excess, bool const in_final_child
  ) : from_(in_from), excess_(in_excess), final_child_(in_final_child)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | from_ | excess_ | final_child_;
  }

  NodeType getFrom() const { return from_; }
  ObjSampleType const& getExcess() const { return excess_; }
  ObjSampleType&& getExcessMove() { return std::move(excess_); }
  bool getFinalChild() const { return final_child_; }

private:
  NodeType from_ = uninitialized_destination;
  ObjSampleType excess_;
  bool final_child_ = 0;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_MSGS_H*/
