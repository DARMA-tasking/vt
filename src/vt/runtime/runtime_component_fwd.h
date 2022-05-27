/*
//@HEADER
// *****************************************************************************
//
//                           runtime_component_fwd.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_RUNTIME_RUNTIME_COMPONENT_FWD_H
#define INCLUDED_VT_RUNTIME_RUNTIME_COMPONENT_FWD_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/sequence/sequencer_fwd.h"

namespace vt {

namespace arguments {
struct ArgConfig;
}
namespace registry {
struct Registry;
}
namespace messaging {
struct ActiveMessenger;
}
namespace ctx {
struct Context;
}
namespace event {
struct AsyncEvent;
}
namespace collective {
struct CollectiveAlg;
}
namespace pool {
struct Pool;
}
namespace rdma {
struct RDMAManager;
struct Manager;
}
namespace param {
struct Param;
}
namespace sched {
struct Scheduler;
}
namespace location {
struct LocationManager;
}
namespace vrt {
struct VirtualContextManager;
}
namespace vrt { namespace collection {
struct CollectionManager;
}}
namespace vrt { namespace collection { namespace balance {
struct NodeLBData;
struct LBDataRestartReader;
struct LBManager;
}}}
namespace group {
struct GroupManager;
}
namespace pipe {
struct PipeManager;
}
namespace objgroup {
struct ObjGroupManager;
}
namespace util { namespace memory {
struct MemoryUsage;
}}
namespace timetrigger {
struct TimeTriggerManager;
}
namespace phase {
struct PhaseManager;
}
namespace epoch {
struct EpochManip;
}

#if vt_check_enabled(trace_enabled)
namespace trace {
struct Trace;
}
#endif
#if vt_check_enabled(mpi_access_guards)
namespace pmpi {
struct PMPIComponent;
}
#endif

} /* end namespace vt */

#endif /*INCLUDED_VT_RUNTIME_RUNTIME_COMPONENT_FWD_H*/
