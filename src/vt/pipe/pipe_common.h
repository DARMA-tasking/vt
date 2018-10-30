
#if !defined INCLUDED_PIPE_PIPE_COMMON_H
#define INCLUDED_PIPE_PIPE_COMMON_H

#include "config.h"
#include "pipe/pipe_manager.fwd.h"

#include <cstdlib>

namespace vt { namespace pipe {

using PipeIDType       = uint32_t;
using PipeRemoteOpType = uint64_t;

static constexpr PipeIDType       const initial_pipe_id = 0;
static constexpr PipeIDType       const no_pipe_id      = 0xFFFFFFFF;
static constexpr PipeRemoteOpType const initial_pipe_op = 0;
static constexpr PipeRemoteOpType const no_pipe_op      = 0xFFFFFFFFFFFFFFFF;

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_COMMON_H*/
