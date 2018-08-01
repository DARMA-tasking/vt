
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager.h"
#include "group/group_common.h"
#include "group/group_manager.h"

namespace vt { namespace pipe {

PipeManager::PipeManager() {
  auto constexpr tag = group::GroupCollectiveLabelTag;
  group_id_ = theGroup()->newGroupCollectiveLabel(tag);
}

}} /* end namespace vt::pipe */
