
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager.h"
#include "pipe/state/pipe_state.h"
#include "pipe/id/pipe_id.h"
#include "context/context.h"

namespace vt { namespace pipe {

PipeType PipeManager::makePipeID(bool const persist, bool const send_back) {
  auto const& this_node = theContext()->getNode();
  auto const next_id = cur_pipe_id_++;
  auto const pipe_id = PipeIDBuilder::createPipeID(
    next_id,this_node,send_back,persist
  );
  return pipe_id;
}

}} /* end namespace vt::pipe */
