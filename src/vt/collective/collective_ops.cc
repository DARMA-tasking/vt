/*
//@HEADER
// *****************************************************************************
//
//                              collective_ops.cc
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

#include "vt/collective/collective_ops.h"
#include "vt/context/context.h"
#include "vt/runtime/runtime.h"
#include "vt/scheduler/scheduler.h"
#include "vt/runtime/runtime_inst.h"
#include "vt/utils/tls/tls.h"

#include <memory>
#include <cstdlib>
#include <mpi.h>

namespace vt {

namespace {

#define printIfOverwritten(opt)                                                \
  do {                                                                         \
    if (cliConfig.opt != appConfig.opt) {                                      \
      ++overwrittens;                                                          \
      fmt::print(                                                              \
        "{}\t{}--" #opt "{}\n",                                                \
        vt_pre, magenta, reset                                                 \
      );                                                                       \
    }                                                                          \
  } while (0)

void printOverwrittens(
  vt::arguments::AppConfig const &cliConfig,
  vt::arguments::AppConfig const &appConfig
) {
  auto const green   = debug::green();
  auto const reset   = debug::reset();
  auto const magenta = debug::magenta();
  auto const vt_pre  = debug::vtPre();

  fmt::print(
    "{}{}Predefined options overwritten by CLI arguments:{}\n",
    vt_pre, green, reset
  );

  int overwrittens = 0;

  printIfOverwritten(vt_color);
  printIfOverwritten(vt_no_color);
  printIfOverwritten(vt_quiet);
  printIfOverwritten(vt_sched_progress_han);
  printIfOverwritten(vt_sched_progress_sec);
  printIfOverwritten(vt_no_sigint);
  printIfOverwritten(vt_no_sigsegv);
  printIfOverwritten(vt_no_sigbus);
  printIfOverwritten(vt_no_terminate);
  printIfOverwritten(vt_memory_reporters);
  printIfOverwritten(vt_print_memory_each_phase);
  printIfOverwritten(vt_print_memory_node);
  printIfOverwritten(vt_allow_memory_report_with_ps);
  printIfOverwritten(vt_print_memory_at_threshold);
  printIfOverwritten(vt_print_memory_threshold);
  printIfOverwritten(vt_print_memory_sched_poll);
  printIfOverwritten(vt_print_memory_footprint);
  printIfOverwritten(vt_no_warn_stack);
  printIfOverwritten(vt_no_assert_stack);
  printIfOverwritten(vt_no_abort_stack);
  printIfOverwritten(vt_no_stack);
  printIfOverwritten(vt_stack_file);
  printIfOverwritten(vt_stack_dir);
  printIfOverwritten(vt_stack_mod);
  printIfOverwritten(vt_trace);
  printIfOverwritten(vt_trace_mpi);
  printIfOverwritten(vt_trace_pmpi);
  printIfOverwritten(vt_trace_sys_all);
  printIfOverwritten(vt_trace_sys_term);
  printIfOverwritten(vt_trace_sys_location);
  printIfOverwritten(vt_trace_sys_collection);
  printIfOverwritten(vt_trace_sys_serial_msg);
  printIfOverwritten(vt_trace_file);
  printIfOverwritten(vt_trace_dir);
  printIfOverwritten(vt_trace_mod);
  printIfOverwritten(vt_trace_flush_size);
  printIfOverwritten(vt_trace_spec);
  printIfOverwritten(vt_trace_spec_file);
  printIfOverwritten(vt_trace_memory_usage);
  printIfOverwritten(vt_trace_event_polling);
  printIfOverwritten(vt_trace_irecv_polling);
  printIfOverwritten(vt_lb);
  printIfOverwritten(vt_lb_show_spec);
  printIfOverwritten(vt_lb_quiet);
  printIfOverwritten(vt_lb_file_name);
  printIfOverwritten(vt_lb_name);
  printIfOverwritten(vt_lb_args);
  printIfOverwritten(vt_lb_interval);
  printIfOverwritten(vt_lb_stats);
  printIfOverwritten(vt_lb_stats_compress);
  printIfOverwritten(vt_lb_stats_dir);
  printIfOverwritten(vt_lb_stats_file);
  printIfOverwritten(vt_lb_stats_dir_in);
  printIfOverwritten(vt_lb_stats_file_in);
  printIfOverwritten(vt_help_lb_args);
  printIfOverwritten(vt_no_detect_hang);
  printIfOverwritten(vt_print_no_progress);
  printIfOverwritten(vt_epoch_graph_on_hang);
  printIfOverwritten(vt_epoch_graph_terse);
  printIfOverwritten(vt_term_rooted_use_ds);
  printIfOverwritten(vt_term_rooted_use_wave);
  printIfOverwritten(vt_hang_freq);
  printIfOverwritten(vt_diag_enable);
  printIfOverwritten(vt_diag_print_summary);
  printIfOverwritten(vt_diag_summary_csv_file);
  printIfOverwritten(vt_diag_summary_file);
  printIfOverwritten(vt_diag_csv_base_units);
  printIfOverwritten(vt_pause);
  printIfOverwritten(vt_no_assert_fail);
  printIfOverwritten(vt_throw_on_abort);
  printIfOverwritten(vt_max_mpi_send_size);
  printIfOverwritten(vt_debug_level);
  printIfOverwritten(vt_debug_all);
  printIfOverwritten(vt_debug_none);
  printIfOverwritten(vt_debug_gen);
  printIfOverwritten(vt_debug_runtime);
  printIfOverwritten(vt_debug_active);
  printIfOverwritten(vt_debug_term);
  printIfOverwritten(vt_debug_termds);
  printIfOverwritten(vt_debug_barrier);
  printIfOverwritten(vt_debug_event);
  printIfOverwritten(vt_debug_pipe);
  printIfOverwritten(vt_debug_pool);
  printIfOverwritten(vt_debug_reduce);
  printIfOverwritten(vt_debug_rdma);
  printIfOverwritten(vt_debug_rdma_channel);
  printIfOverwritten(vt_debug_rdma_state);
  printIfOverwritten(vt_debug_param);
  printIfOverwritten(vt_debug_handler);
  printIfOverwritten(vt_debug_hierlb);
  printIfOverwritten(vt_debug_temperedlb);
  printIfOverwritten(vt_debug_scatter);
  printIfOverwritten(vt_debug_sequence);
  printIfOverwritten(vt_debug_sequence_vrt);
  printIfOverwritten(vt_debug_serial_msg);
  printIfOverwritten(vt_debug_trace);
  printIfOverwritten(vt_debug_location);
  printIfOverwritten(vt_debug_lb);
  printIfOverwritten(vt_debug_vrt);
  printIfOverwritten(vt_debug_vrt_coll);
  printIfOverwritten(vt_debug_worker);
  printIfOverwritten(vt_debug_group);
  printIfOverwritten(vt_debug_broadcast);
  printIfOverwritten(vt_debug_objgroup);
  printIfOverwritten(vt_debug_phase);
  printIfOverwritten(vt_debug_context);
  printIfOverwritten(vt_debug_epoch);
  printIfOverwritten(vt_debug_print_flush);
  printIfOverwritten(vt_user_1);
  printIfOverwritten(vt_user_2);
  printIfOverwritten(vt_user_3);
  printIfOverwritten(vt_user_int_1);
  printIfOverwritten(vt_user_int_2);
  printIfOverwritten(vt_user_int_3);
  printIfOverwritten(vt_user_str_1);
  printIfOverwritten(vt_user_str_2);
  printIfOverwritten(vt_user_str_3);
  printIfOverwritten(vt_output_config);
  printIfOverwritten(vt_output_config_file);

  if (overwrittens == 0) {
    fmt::print("{}\tNone.\n", vt_pre);
  }
}

} /* end anon namespace */

template <runtime::RuntimeInstType instance>
RuntimePtrType CollectiveAnyOps<instance>::initialize(
  int& argc, char**& argv, WorkerCountType const num_workers,
  bool is_interop, MPI_Comm* comm, arguments::AppConfig const* appConfig
) {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

  MPI_Comm resolved_comm = comm not_eq nullptr ? *comm : MPI_COMM_WORLD;

#pragma sst global rt
  RuntimeInst<instance>::rt = std::make_unique<Runtime>(
    argc, argv, num_workers, is_interop, resolved_comm,
    eRuntimeInstance::DefaultInstance, appConfig
  );

#pragma sst global rt
  auto rt_ptr = RuntimeInst<instance>::rt.get();
  if (instance == runtime::RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = rt_ptr;
    curRT = rt_ptr;
  }
#pragma sst global rt
  RuntimeInst<instance>::rt->initialize();

  if (appConfig && theContext()->getNode() == 0) {
    printOverwrittens(*rt->getAppConfig(), *appConfig);
  }

  return runtime::makeRuntimePtr(rt_ptr);
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::setCurrentRuntimeTLS(RuntimeUnsafePtrType in) {
  bool const has_rt = in != nullptr;
  auto rt_use = has_rt ? in : ::vt::rt;
  curRT = rt_use;
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::scheduleThenFinalize(
  RuntimePtrType in_rt, WorkerCountType const workers
) {
  bool const has_rt = in_rt != nullptr;
  auto rt_use = has_rt ? in_rt.unsafe() : curRT;

  auto sched_fn = [=]{
    theSched()->runSchedulerWhile([rt_use]{ return not rt_use->isTerminated(); });
  };

  if (workers == no_workers) {
    sched_fn();
  } else {
    #if vt_threading_enabled
    theWorkerGrp()->spawnWorkersBlock(sched_fn);
    #else
    sched_fn();
    #endif
  }

  CollectiveAnyOps<instance>::finalize(has_rt ? std::move(in_rt) : nullptr);
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::finalize(RuntimePtrType in_rt) {
  using vt::runtime::RuntimeInst;
  using vt::runtime::Runtime;
  using vt::runtime::eRuntimeInstance;

#pragma sst global rt
  RuntimeInst<instance>::rt = nullptr;

  if (instance == runtime::RuntimeInstType::DefaultInstance) {
    // Set global variable for default instance for backward compatibility
    ::vt::rt = nullptr;
    curRT = nullptr;
  }

  if (in_rt) {
    in_rt = nullptr;
  }
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::abort(
  std::string const str, ErrorCodeType const code
) {
  auto tls_rt = curRT;
  auto myrt = tls_rt ? tls_rt : ::vt::rt;
  if (myrt) {
#if vt_check_enabled(trace_enabled)
    //--- Try to flush most of the traces before aborting
    myrt->theTrace->cleanupTracesFile();
#endif
    myrt->abort(str, code);
  } else if (vt::debug::preConfig()->vt_throw_on_abort) {
    // Special case when preConfig has 'vt_throw_on_abort' option set
    // This is meant to be used by nompi unit tests
    throw std::runtime_error(str);
  } else {
    std::_Exit(code);
  }
}

template <runtime::RuntimeInstType instance>
void CollectiveAnyOps<instance>::output(
  std::string const str, ErrorCodeType const code, bool error, bool decorate,
  bool formatted, bool abort_out
) {
  auto tls_rt = curRT;
  auto myrt = tls_rt ? tls_rt : ::vt::rt;
  if (myrt) {
    myrt->output(str,code,error,decorate,formatted);
  } else {
    ::fmt::print(str.c_str());
  }
  if (error and abort_out) {
    vt::abort("Assertion Failed", 129);
  }
}

template <runtime::RuntimeInstType instance>
HandlerType CollectiveAnyOps<instance>::registerHandler(ActiveClosureFnType fn) {
  return theRegistry()->registerActiveHandler(fn);
}

template struct CollectiveAnyOps<collective_default_inst>;

} /* end namespace vt */
