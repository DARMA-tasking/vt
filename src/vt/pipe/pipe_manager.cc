
#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/group/group_common.h"
#include "vt/group/group_manager.h"

namespace vt { namespace pipe {

PipeManager::PipeManager() {
  group_id_ = theGroup()->newGroupCollectiveLabel(
    group::GroupCollectiveLabelTag
  );
}

Callback<PipeManager::Void> PipeManager::makeFunc(FuncVoidType fn) {
  return makeCallbackSingleAnonVoid<Callback<Void>>(fn);
}

}} /* end namespace vt::pipe */
