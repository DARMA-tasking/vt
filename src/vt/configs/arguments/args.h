/*
//@HEADER
// *****************************************************************************
//
//                                    args.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H

// Do not pull in any other VT dependencies here
#include "vt/runtime/component/component.h"

#include <functional>
#include <string>
#include <vector>
#include <tuple>

namespace CLI {
class App;
}

namespace vt { namespace arguments {

struct ArgConfig : runtime::component::Component<ArgConfig> {

  /// Parse the arguments into ArgConfig.
  /// Re-assigns argc/argv to remove consumed arguments.
  /// On success the tuple will be {-1, ""}. Otherwise the exit code
  /// (which may be 0 if help was requested) will be returned along
  /// with an appropriate display message.
  std::tuple<int, std::string> parse(int& argc, char**& argv);
  std::tuple<int, std::string> parseArguments(CLI::App& app, int& argc, char**& argv);

  std::string name() override { return "ArgConfig"; }

public:
  void addColorArgs(CLI::App& app);
  void addSignalArgs(CLI::App& app);
  void addMemUsageArgs(CLI::App& app);
  void addStackDumpArgs(CLI::App& app);
  void addTraceArgs(CLI::App& app);
  void addDebugPrintArgs(CLI::App& app);
  void addLbArgs(CLI::App& app);
  void addTerminationArgs(CLI::App& app);
  void addDebuggerArgs(CLI::App& app);
  void addUserArgs(CLI::App& app);
  void addSchedulerArgs(CLI::App& app);
  void addConfigFileArgs(CLI::App& app);

  inline bool user1() { return vt_user_1; }
  inline bool user2() { return vt_user_2; }
  inline bool user3() { return vt_user_3; }

  inline bool traceTerm() {
    return vt_trace_sys_term or vt_trace_sys_all;
  }
  inline bool traceLocation() {
    return vt_trace_sys_location or vt_trace_sys_all;
  }
  inline bool traceCollection() {
    return vt_trace_sys_collection or vt_trace_sys_all;
  }
  inline bool traceSerialMsg() {
    return vt_trace_sys_serial_msg or vt_trace_sys_all;
  }

  inline bool alwaysFlush() {
    return vt_debug_print_flush;
  }

  bool vt_color      = true;
  bool vt_no_color   = false;
  bool vt_auto_color = false;
  bool vt_quiet      = false;
  // Derived from vt_*_color arguments after parsing.
  bool colorize_output;

  int32_t vt_sched_num_progress = 2;
  int32_t vt_sched_progress_han = 0;
  double vt_sched_progress_sec  = 0.0;
  bool vt_no_sigint    = false;
  bool vt_no_sigsegv   = false;
  bool vt_no_terminate = false;
  std::string vt_memory_reporters =
  # if vt_check_enabled(mimalloc)
  "mimalloc,"
# endif
  "mstats,machinfo,selfstat,selfstatm,sbrk,mallinfo,getrusage,ps";
  bool vt_print_memory_each_phase  = false;
  std::string vt_print_memory_node = "0";
  bool vt_allow_memory_report_with_ps = false;
  bool vt_print_memory_at_threshold   = false;
  std::string vt_print_memory_threshold = "1 GiB";
  int32_t vt_print_memory_sched_poll    = 100;

  bool vt_no_warn_stack     = false;
  bool vt_no_assert_stack   = false;
  bool vt_no_abort_stack    = false;
  bool vt_no_stack          = false;
  std::string vt_stack_file = "";
  std::string vt_stack_dir  = "";
  int32_t vt_stack_mod      = 0;

  bool vt_trace                  = false;
  bool vt_trace_mpi              = false;
  bool vt_trace_pmpi             = false;
  bool vt_trace_sys_all          = false;
  bool vt_trace_sys_term         = false;
  bool vt_trace_sys_location     = false;
  bool vt_trace_sys_collection   = false;
  bool vt_trace_sys_serial_msg   = false;
  std::string vt_trace_file      = "";
  std::string vt_trace_dir       = "";
  int32_t vt_trace_mod           = 0;
  int32_t vt_trace_flush_size    = 0;
  bool vt_trace_spec             = false;
  std::string vt_trace_spec_file = "";
  bool vt_trace_memory_usage     = false;
  bool vt_trace_event_polling    = false;
  bool vt_trace_irecv_polling    = false;

  bool vt_lb                  = false;
  bool vt_lb_file             = false;
  bool vt_lb_quiet            = false;
  std::string vt_lb_file_name = "";
  std::string vt_lb_name      = "NoLB";
  std::string vt_lb_args      = "";
  int32_t vt_lb_interval      = 1;
  bool vt_lb_stats            = false;
  std::string vt_lb_stats_dir     = "vt_lb_stats";
  std::string vt_lb_stats_file    = "stats";
  std::string vt_lb_stats_dir_in  = "vt_lb_stats_in";
  std::string vt_lb_stats_file_in = "stats_in";

  bool vt_no_detect_hang       = false;
  bool vt_print_no_progress    = true;
  bool vt_epoch_graph_on_hang  = true;
  bool vt_epoch_graph_terse    = false;
  bool vt_term_rooted_use_ds   = false;
  bool vt_term_rooted_use_wave = false;
  int64_t vt_hang_freq         = 1024;

  bool vt_pause = false;

  bool vt_debug_all          = false;
  bool vt_debug_verbose      = false;
  bool vt_debug_none         = false;
  bool vt_debug_gen          = false;
  bool vt_debug_runtime      = false;
  bool vt_debug_active       = false;
  bool vt_debug_term         = false;
  bool vt_debug_termds       = false;
  bool vt_debug_barrier      = false;
  bool vt_debug_event        = false;
  bool vt_debug_pipe         = false;
  bool vt_debug_pool         = false;
  bool vt_debug_reduce       = false;
  bool vt_debug_rdma         = false;
  bool vt_debug_rdma_channel = false;
  bool vt_debug_rdma_state   = false;
  bool vt_debug_param        = false;
  bool vt_debug_handler      = false;
  bool vt_debug_hierlb       = false;
  bool vt_debug_gossiplb     = false;
  bool vt_debug_scatter      = false;
  bool vt_debug_sequence     = false;
  bool vt_debug_sequence_vrt = false;
  bool vt_debug_serial_msg   = false;
  bool vt_debug_trace        = false;
  bool vt_debug_location     = false;
  bool vt_debug_lb           = false;
  bool vt_debug_vrt          = false;
  bool vt_debug_vrt_coll     = false;
  bool vt_debug_worker       = false;
  bool vt_debug_group        = false;
  bool vt_debug_broadcast    = false;
  bool vt_debug_objgroup     = false;

  bool vt_debug_print_flush = false;

  bool vt_user_1 = false;
  bool vt_user_2 = false;
  bool vt_user_3 = false;
  int32_t vt_user_int_1 = 0;
  int32_t vt_user_int_2 = 0;
  int32_t vt_user_int_3 = 0;
  std::string vt_user_str_1 = "";
  std::string vt_user_str_2 = "";
  std::string vt_user_str_3 = "";

  bool vt_output_config   = false;
  std::string vt_output_config_file = "vt_config.ini";
  std::string vt_output_config_str  = "";

  /// Name of the program launched (excluding any path!)
  std::string prog_name {"vt_unknown"};

  /// Name of the program launched, aka argv[0].
  /// Original char* object.
  char* argv_prog_name {const_cast<char*>("vt_unknown")};

  /// Arguments to pass to MPI Init.
  /// Does not include argv[0]. Original char* objects.
  std::vector<char*> mpi_init_args;
  /// Arguments are being ref-returend as the result of parse(..).
  /// Does not include argv[0]. Original char* objects.
  std::vector<char*> passthru_args;

private:
  void postParseTransform();

  bool parsed_ = false;
};

}} /* end namespace vt::arguments */

namespace vt {

extern arguments::ArgConfig* theArgConfig();

} /* end namespace vt */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
