/*
//@HEADER
// *****************************************************************************
//
//                                runtime_get.cc
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

#include "vt/config.h"
#include "vt/configs/arguments/args.h"
#include "vt/context/context.h"
#include "vt/runtime/runtime.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/utils/tls/tls.h"
#include "vt/vrt/context/context_vrtmanager.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"
#include "vt/event/event.h"
#include "vt/termination/term_headers.h"
#include "vt/collective/collective_alg.h"
#include "vt/pool/pool.h"
#include "vt/rdma/rdma.h"
#include "vt/parameterization/parameterization.h"
#include "vt/trace/trace.h"
#include "vt/scheduler/scheduler.h"
#include "vt/topos/location/location_headers.h"
#include "vt/vrt/collection/collection_headers.h"
#include "vt/group/group_headers.h"
#include "vt/pipe/pipe_headers.h"
#include "vt/objgroup/headers.h"
#include "vt/timetrigger/time_trigger_manager.h"
#include "vt/phase/phase_manager.h"
#include "vt/epoch/epoch_manip.h"

#include <cassert>

namespace vt {

/*
 *         === Access functions for the runtime component singletons ===
 *
 * Macro is used for convenience so these can be changed easily.
 *
 * Currently, there is only one runtime instance that is actually used, which is
 * accessed through the TLS variable `curRT'. Using a TLS variable enables
 * switching the pointer for other non-communication threads that should not
 * have directly access to runtime components that are not thread-safe.
 *
 * The use of a TLS variable for every access to the singletons might slow down
 * performance. If so, we may need to redesign this.
 *
 */

static runtime::Runtime* no_rt = nullptr;

#define IS_COMM_THREAD curRT->theContext->getWorker() == worker_id_comm_thread
#define CUR_RT_SAFE (IS_COMM_THREAD ? curRT : no_rt)
#define CUR_RT_TS curRT
#define CUR_RT CUR_RT_SAFE

#define CHECK_THD                                                       \
  do {                                                                  \
    bool const check = IS_COMM_THREAD;                                  \
    std::string str("Only comm thread can access this component");      \
    vtAssertExpr(check && str.c_str());                                       \
    if (!check) { CUR_RT->abort(str, 29); }                             \
  } while (0);

using CollectionManagerType = vrt::collection::CollectionManager;

// Thread-safe runtime components
ctx::Context*               theContext()            { return CUR_RT_TS->theContext;        }
pool::Pool*                 thePool()               { return CUR_RT_TS->thePool;           }
vrt::VirtualContextManager* theVirtualManager()     { return CUR_RT_TS->theVirtualManager; }

// Non thread-safe runtime components
collective::CollectiveAlg*  theCollective()         { return CUR_RT->theCollective;     }
event::AsyncEvent*          theEvent()              { return CUR_RT->theEvent;          }
messaging::ActiveMessenger* theMsg()                { return CUR_RT->theMsg;            }
param::Param*               theParam()              { return CUR_RT->theParam;          }
rdma::RDMAManager*          theRDMA()               { return CUR_RT->theRDMA;           }
sched::Scheduler*           theSched()              { return CUR_RT->theSched;          }
term::TerminationDetector*  theTerm()               { return CUR_RT->theTerm;           }
location::LocationManager*  theLocMan()             { return CUR_RT->theLocMan;         }
CollectionManagerType*      theCollection()         { return CUR_RT->theCollection;     }
group::GroupManager*        theGroup()              { return CUR_RT->theGroup;          }
pipe::PipeManager*          theCB()                 { return CUR_RT->theCB;             }
objgroup::ObjGroupManager*  theObjGroup()           { return CUR_RT->theObjGroup;       }
rdma::Manager*              theHandleRDMA()         { return CUR_RT->theHandleRDMA;     }
util::memory::MemoryUsage*  theMemUsage()           { return CUR_RT->theMemUsage;       }
vrt::collection::balance::NodeLBData* theNodeLBData() { return CUR_RT->theNodeLBData;      }
vrt::collection::balance::LBDataRestartReader* theLBDataReader() { return CUR_RT->theLBDataReader;      }
vrt::collection::balance::LBManager* theLBManager() { return CUR_RT->theLBManager;      }
timetrigger::TimeTriggerManager* theTimeTrigger()   { return CUR_RT->theTimeTrigger;    }
vt::arguments::AppConfig*   theConfig()             { return &CUR_RT->theArgConfig->config_;      }
vt::phase::PhaseManager*   thePhase()               { return CUR_RT->thePhase;          }
epoch::EpochManip*          theEpoch()              { return CUR_RT->theEpoch;          }

#if vt_check_enabled(trace_enabled)
trace::Trace*               theTrace()              { return CUR_RT->theTrace;          }
#endif
#if vt_check_enabled(mpi_access_guards)
pmpi::PMPIComponent*        thePMPI()               { return CUR_RT->thePMPI;           }
#endif

#undef CUR_RT
#undef CUR_RT_SAFE
#undef IS_COMM_THREAD

} /* end namespace vt */

namespace vt { namespace debug {

// Dummy config that applies outside of RT initialization, much like preNode.
static arguments::AppConfig preInitAppConfig{};


/**
 * \internal
 * \brief Returns the preConfig, accessible OUTSIDE of VT initialization.
 *
 * This non-const version is used by 'nompi' tests, in order to customize
 * the app config (mostly vt_throw_on_abort)
 *
 * \return A modifiable configuration
 */
arguments::AppConfig* preConfigRef() {
  return &preInitAppConfig;
}

/**
 * \internal
 * \brief Returns the config, accessible OUTSIDE of VT initialization.
 *
 * Much as preNode, this can be accessed safely in debug* methods.
 * This allows such methods to be used in code that is unit-test OK.
 *
 * \return A configuration; possible a default configuration.
 */
arguments::AppConfig const* preConfig() {
  auto const runtime = curRT;
  if (not runtime)
    return &preInitAppConfig;
  auto const* config = runtime->theArgConfig;
  return config not_eq nullptr ? &config->config_ : runtime->getAppConfig();
}

}} /* end namespace vt::debug */
