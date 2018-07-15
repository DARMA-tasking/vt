
#if !defined INCLUDED_GROUP_GROUP_COLLECTIVE_H
#define INCLUDED_GROUP_GROUP_COLLECTIVE_H

#include "config.h"
#include "group/group_common.h"
#include "group/group_manager.fwd.h"
#include "group/group_info.fwd.h"
#include "group/group_info.fwd.h"
#include "collective/tree/tree.h"
#include "collective/reduce/reduce.h"

#include <memory>

namespace vt { namespace group {

struct GroupCollective {
  using TreeType = collective::tree::Tree;
  using TreePtrType = std::unique_ptr<TreeType>;
  using NodeListType = TreeType::NodeListType;
  using ReduceType = collective::reduce::Reduce;
  using ReducePtrType = std::unique_ptr<ReduceType>;

  explicit GroupCollective()
    : init_span_(
        std::make_unique<TreeType>(collective::tree::tree_cons_tag_t)
      )
  { }

  friend struct InfoColl;

protected:
  NodeType getInitialParent() const { return init_span_->getParent(); }
  NodeType getInitialChildren() const { return init_span_->getNumChildren();  }
  bool isInitialRoot() const { return init_span_->isRoot();  }
  NodeListType const& getChildren() const { return init_span_->getChildren(); }

private:
  TreePtrType span_           = nullptr;
  TreePtrType init_span_      = nullptr;
  NodeListType span_children_ = {};
  NodeType parent_            = uninitialized_destination;
  ReducePtrType reduce_       = nullptr;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_COLLECTIVE_H*/
