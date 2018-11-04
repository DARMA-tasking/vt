
#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/collective/group_info_collective.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/group/group_manager.h"

namespace vt { namespace group {

void InfoColl::CollSetupFinished::operator()(FinishedReduceMsg* msg) {
  debug_print(
    verbose, group, node,
    "CollSetupFinished: group={:x}\n", msg->getGroup()
  );
  auto iter = theGroup()->local_collective_group_info_.find(msg->getGroup());
  assert(
    iter != theGroup()->local_collective_group_info_.end() && "Must exist"
  );
  auto const& this_node = theContext()->getNode();
  auto info = iter->second.get();
  if (info->known_root_node_ != this_node) {
    auto nmsg = makeSharedMessage<GroupOnlyMsg>(
      msg->getGroup(),info->new_tree_cont_
    );
    theMsg()->sendMsg<GroupOnlyMsg,InfoColl::newTreeHan>(
      info->known_root_node_,nmsg
    );
  } else {
    info->newTree(-1);
  }
}

}} /* end namespace vt::group */
