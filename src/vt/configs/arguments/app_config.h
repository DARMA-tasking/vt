/*
//@HEADER
// *****************************************************************************
//
//                                 app_config.h
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

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_APP_CONFIG_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_APP_CONFIG_H

// do not pull in any VT dependencies here
#include <string>
#include <vector>
#include <unordered_map>
#include <yaml-cpp/yaml.h>

namespace vt { namespace arguments {

/**
 * \struct AppConfig
 *
 * \brief Configuration parameters for a VT instance
 */
struct AppConfig {
  inline bool user1() const { return vt_user_1; }
  inline bool user2() const { return vt_user_2; }
  inline bool user3() const { return vt_user_3; }

  inline bool traceTerm() const {
    return vt_trace_sys_term or vt_trace_sys_all;
  }
  inline bool traceLocation() const {
    return vt_trace_sys_location or vt_trace_sys_all;
  }
  inline bool traceCollection() const {
    return vt_trace_sys_collection or vt_trace_sys_all;
  }
  inline bool traceSerialMsg() const {
    return vt_trace_sys_serial_msg or vt_trace_sys_all;
  }

  inline bool alwaysFlush() const {
    return vt_debug_print_flush;
  }

  std::string vt_input_config_yaml = "";

  bool vt_color      = true;
#ifdef VT_NO_COLOR_ENABLED
  bool vt_no_color   = true;
#else
  bool vt_no_color   = false;
#endif
  bool vt_auto_color = false;
  bool vt_quiet      = false;
  // Derived from vt_*_color arguments after parsing.
  bool colorize_output;

  int32_t vt_sched_num_progress = 2;
  int32_t vt_sched_progress_han = 0;
  double vt_sched_progress_sec  = 0.0;
  bool vt_no_sigint    = false;
  bool vt_no_sigsegv   = false;
  bool vt_no_sigbus    = false;
  bool vt_no_terminate = false;
  std::string vt_memory_reporters =
# if (vt_feature_mimalloc != 0)
  "mimalloc,"
# endif
  "mstats,machinfo,selfstat,selfstatm,sbrk,mallinfo,getrusage,ps";
  bool vt_print_memory_each_phase  = false;
  std::string vt_print_memory_node = "0";
  bool vt_allow_memory_report_with_ps = false;
  bool vt_print_memory_at_threshold   = false;
  std::string vt_print_memory_threshold = "1 GiB";
  int32_t vt_print_memory_sched_poll    = 100;
  bool vt_print_memory_footprint  = false;

  bool vt_no_warn_stack     = false;
  bool vt_no_assert_stack   = false;
  bool vt_no_abort_stack    = false;
  bool vt_no_stack          = false;
  std::string vt_stack_file = "";
  std::string vt_stack_dir  = "";
  int32_t vt_stack_mod      = 0;

  bool vt_trace                   = false;
  bool vt_trace_mpi               = false;
  bool vt_trace_pmpi              = false;
  bool vt_trace_sys_all           = false;
  bool vt_trace_sys_term          = false;
  bool vt_trace_sys_location      = false;
  bool vt_trace_sys_collection    = false;
  bool vt_trace_sys_serial_msg    = false;
  std::string vt_trace_file       = "";
  std::string vt_trace_dir        = "";
  int32_t vt_trace_mod            = 0;
  int32_t vt_trace_flush_size     = 0;
  bool vt_trace_gzip_finish_flush = false;
  bool vt_trace_spec              = false;
  std::string vt_trace_spec_file  = "";
  bool vt_trace_memory_usage      = false;
  bool vt_trace_event_polling     = false;
  bool vt_trace_irecv_polling     = false;

  bool vt_lb                     = false;
  bool vt_lb_show_config           = false;
  bool vt_lb_quiet               = false;
  std::string vt_lb_file_name    = "";
  std::string vt_lb_name         = "NoLB";
  std::string vt_lb_args         = "";
  int32_t vt_lb_interval         = 1;
  bool vt_lb_keep_last_elm       = false;
  bool vt_lb_data                = false;
  bool vt_lb_data_compress       = true;
  bool vt_lb_data_in             = false;
  std::string vt_lb_data_dir     = "vt_lb_data";
  std::string vt_lb_data_file    = "data.%p.json";
  std::string vt_lb_data_dir_in  = "vt_lb_data_in";
  std::string vt_lb_data_file_in = "data.%p.json";
  bool vt_lb_statistics           = true;
  bool vt_lb_statistics_compress  = true;
  std::string vt_lb_statistics_file = "vt_lb_statistics.%t.json";
  std::string vt_lb_statistics_dir = "";
  bool vt_help_lb_args           = false;
  bool vt_lb_self_migration      = false;
  bool vt_lb_spec                = false;
  std::string vt_lb_spec_file    = "";


  bool vt_no_detect_hang       = false;
  bool vt_print_no_progress    = true;
  bool vt_epoch_graph_on_hang  = true;
  bool vt_epoch_graph_terse    = false;
  bool vt_term_rooted_use_ds   = false;
  bool vt_term_rooted_use_wave = false;
  int64_t vt_hang_freq         = 1024;

#if (vt_diagnostics_runtime != 0)
  bool vt_diag_enable = true;
#else
  bool vt_diag_enable = false;
#endif
  bool vt_diag_print_summary = false;
  std::string vt_diag_summary_csv_file = "";
  std::string vt_diag_summary_file = "vtdiag.txt";
  bool vt_diag_csv_base_units = false;

  bool vt_pause = false;
  bool vt_no_assert_fail = false;
  bool vt_throw_on_abort = false;
  std::size_t vt_max_mpi_send_size = 1ull << 30;

#if (vt_feature_fcontext != 0)
  bool vt_ult_disable = false;
  std::size_t vt_ult_stack_size = (1 << 21) - 64;
#endif

  std::string vt_debug_level = "terse";
  uint64_t vt_debug_level_val = 0;

  bool vt_debug_all          = false;
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
  bool vt_debug_temperedlb   = false;
  bool vt_debug_temperedwmin = false;
  bool vt_debug_scatter      = false;
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
  bool vt_debug_phase        = false;
  bool vt_debug_context      = false;
  bool vt_debug_epoch        = false;
  bool vt_debug_replay       = false;

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

  /// Arguments are being ref-returned as the result of parse(..).
  /// Does not include argv[0]. Original char* objects.
  std::vector<char*> passthru_args;

  std::string getLBDataFileOut() const;
  std::string getLBDataFileIn() const;
  std::string getLBStatisticsFile() const;

  // {CLI arg, {Node, Key}}
  std::unordered_map<std::string, std::pair<std::string, std::string>> cli_to_yaml_args = {
    // Output Control
    {"vt_color", {"Output Control", "Color"}},
    {"vt_no_color", {"Output Control", "Color"}},
    {"vt_quiet", {"Output Control", "Quiet"}},

    // Signal Handling
    {"vt_no_sigint", {"Signal Handling", "Disable SIGINT"}},
    {"vt_no_sigsegv", {"Signal Handling", "Disable SIGSEGV"}},
    {"vt_no_sigbus", {"Signal Handling", "Disable SIGBUS"}},
    {"vt_no_terminate", {"Signal Handling", "Disable Terminate Signal"}},

    // Memory Usage Reporting
    {"vt_memory_reporters", {"Memory Usage Reporting", "Memory Reporters"}},
    {"vt_print_memory_each_phase", {"Memory Usage Reporting", "Print Memory Each Phase"}},
    {"vt_print_memory_node", {"Memory Usage Reporting", "Print Memory on Node"}},
    {"vt_allow_memory_report_with_ps", {"Memory Usage Reporting", "Allow Memory Report With ps"}},
    {"vt_print_memory_threshold", {"Memory Usage Reporting", "Print Memory Threshold"}},
    {"vt_print_memory_sched_poll", {"Memory Usage Reporting", "Print Memory Scheduler Poll"}},
    {"vt_print_memory_footprint", {"Memory Usage Reporting", "Print Memory Footprint"}},

    // Dump Stack Backtrace
    {"vt_no_warn_stack", {"Dump Stack Backtrace", "Enable Stack Output on Warning"}},
    {"vt_no_assert_stack", {"Dump Stack Backtrace", "Enable Stack Output on Assert"}},
    {"vt_no_abort_stack", {"Dump Stack Backtrace", "Enable Stack Output on Abort"}},
    {"vt_no_stack", {"Dump Stack Backtrace", "Enable Stack Output"}},
    {"vt_stack_file", {"Dump Stack Backtrace", "File"}},
    {"vt_stack_dir", {"Dump Stack Backtrace", "Directory"}},
    {"vt_stack_mod", {"Dump Stack Backtrace", "Output Rank Mod"}},

    // Tracing Configuration
    {"vt_trace", {"Tracing Configuration", "Enabled"}},
    {"vt_trace_mpi", {"Tracing Configuration", "MPI Type Events"}},
    {"vt_trace_pmpi", {"Tracing Configuration", "MPI Type Events"}},
    {"vt_trace_file", {"Tracing Configuration", "File"}},
    {"vt_trace_dir", {"Tracing Configuration", "Directory"}},
    {"vt_trace_mod", {"Tracing Configuration", "Output Rank Mod"}},
    {"vt_trace_flush_size", {"Tracing Configuration", "Flush Size"}},
    {"vt_trace_gzip_finish_flush", {"Tracing Configuration", "GZip Finish Flush"}},
    {"vt_trace_sys_all", {"Tracing Configuration", "Include All System Events"}},
    {"vt_trace_sys_term", {"Tracing Configuration", "Include Termination Events"}},
    {"vt_trace_sys_location", {"Tracing Configuration", "Include Location Events"}},
    {"vt_trace_sys_collection", {"Tracing Configuration", "Include Collection Events"}},
    {"vt_trace_sys_serial_msg", {"Tracing Configuration", "Include Message Serialization Events"}},
    {"vt_trace_spec", {"Tracing Configuration", "Specification Enabled"}},
    {"vt_trace_spec_file", {"Tracing Configuration", "Spec File"}},
    {"vt_trace_memory_usage", {"Tracing Configuration", "Memory Usage"}},
    {"vt_trace_event_polling", {"Tracing Configuration", "Event Polling"}},
    {"vt_trace_irecv_polling", {"Tracing Configuration", "IRecv Polling"}},

    // Debug Print Configuration
    {"vt_debug_level", {"Debug Print Configuration", "Level"}},
    {"vt_debug_all", {"Debug Print Configuration", "Enable All"}},
    {"vt_debug_none", {"Debug Print Configuration", "Disable All"}},
    {"vt_debug_print_flush", {"Debug Print Configuration", "Debug Print Flush"}},
    {"vt_debug_gen", {"Debug Print Configuration/Enable", "gen"}},
    {"vt_debug_runtime", {"Debug Print Configuration/Enable", "runtime"}},
    {"vt_debug_active", {"Debug Print Configuration/Enable", "active"}},
    {"vt_debug_term", {"Debug Print Configuration/Enable", "term"}},
    {"vt_debug_termds", {"Debug Print Configuration/Enable", "termds"}},
    {"vt_debug_barrier", {"Debug Print Configuration/Enable", "barrier"}},
    {"vt_debug_event", {"Debug Print Configuration/Enable", "event"}},
    {"vt_debug_pipe", {"Debug Print Configuration/Enable", "pipe"}},
    {"vt_debug_pool", {"Debug Print Configuration/Enable", "pool"}},
    {"vt_debug_reduce", {"Debug Print Configuration/Enable", "reduce"}},
    {"vt_debug_rdma", {"Debug Print Configuration/Enable", "rdma"}},
    {"vt_debug_rdma_channel", {"Debug Print Configuration/Enable", "rdma_channel"}},
    {"vt_debug_rdma_state", {"Debug Print Configuration/Enable", "rdma_state"}},
    {"vt_debug_handler", {"Debug Print Configuration/Enable", "handler"}},
    {"vt_debug_hierlb", {"Debug Print Configuration/Enable", "hierlb"}},
    {"vt_debug_temperedlb", {"Debug Print Configuration/Enable", "temperedlb"}},
    {"vt_debug_temperedwmin", {"Debug Print Configuration/Enable", "temperedwmin"}},
    {"vt_debug_scatter", {"Debug Print Configuration/Enable", "scatter"}},
    {"vt_debug_serial_msg", {"Debug Print Configuration/Enable", "serial_msg"}},
    {"vt_debug_trace", {"Debug Print Configuration/Enable", "trace"}},
    {"vt_debug_location", {"Debug Print Configuration/Enable", "location"}},
    {"vt_debug_lb", {"Debug Print Configuration/Enable", "lb"}},
    {"vt_debug_vrt", {"Debug Print Configuration/Enable", "vrt"}},
    {"vt_debug_vrt_coll", {"Debug Print Configuration/Enable", "vrt_coll"}},
    {"vt_debug_worker", {"Debug Print Configuration/Enable", "worker"}},
    {"vt_debug_group", {"Debug Print Configuration/Enable", "group"}},
    {"vt_debug_broadcast", {"Debug Print Configuration/Enable", "broadcast"}},
    {"vt_debug_objgroup", {"Debug Print Configuration/Enable", "objgroup"}},
    {"vt_debug_phase", {"Debug Print Configuration/Enable", "phase"}},
    {"vt_debug_context", {"Debug Print Configuration/Enable", "context"}},
    {"vt_debug_epoch", {"Debug Print Configuration/Enable", "epoch"}},

    // Load Balancing
    {"vt_lb", {"Load Balancing", "Enabled"}},
    {"vt_lb_quiet", {"Load Balancing", "Quiet"}},
    {"vt_lb_file_name", {"Load Balancing", "File"}},
    {"vt_lb_show_config", {"Load Balancing", "Show Configuration"}},
    {"vt_lb_name", {"Load Balancing", "Name"}},
    {"vt_lb_args", {"Load Balancing", "Arguments"}},
    {"vt_lb_interval", {"Load Balancing", "Interval"}},
    {"vt_lb_keep_last_elm", {"Load Balancing", "Keep Last Element"}},
    {"vt_lb_data", {"Load Balancing/LB Data Output", "Enabled"}},
    {"vt_lb_data_dir", {"Load Balancing/LB Data Output", "Directory"}},
    {"vt_lb_data_file", {"Load Balancing/LB Data Output", "File"}},
    {"vt_lb_data_in", {"Load Balancing/LB Data Input", "Enabled"}},
    {"vt_lb_data_compress", {"Load Balancing/LB Data Input", "Enable Compression"}},
    {"vt_lb_data_dir_in", {"Load Balancing/LB Data Input", "Directory"}},
    {"vt_lb_data_file_in", {"Load Balancing/LB Data Input", "File"}},
    {"vt_lb_statistics", {"Load Balancing/LB Statistics", "Enabled"}},
    {"vt_lb_statistics_compress", {"Load Balancing/LB Statistics", "Enable Compression"}},
    {"vt_lb_statistics_file", {"Load Balancing/LB Statistics", "File"}},
    {"vt_lb_statistics_dir", {"Load Balancing/LB Statistics", "Directory"}},
    {"vt_lb_self_migration", {"Load Balancing", "Enable Self Migration"}},
    {"vt_lb_spec", {"Load Balancing", "Enable Specification"}},
    {"vt_lb_spec_file", {"Load Balancing", "Specification File"}},

    // Diagnostics
    {"vt_diag_enable", {"Diagnostics", "Enabled"}},
    {"vt_diag_print_summary", {"Diagnostics", "Enable Print Summary"}},
    {"vt_diag_summary_file", {"Diagnostics", "Summary File"}},
    {"vt_diag_summary_csv_file", {"Diagnostics", "Summary CSV File"}},
    {"vt_diag_csv_base_units", {"Diagnostics", "Use CSV Base Units"}},

    // Termination
    {"vt_no_detect_hang", {"Termination", "Detect Hangs"}},
    {"vt_term_rooted_use_ds", {"Termination", "Use DS for Rooted"}},
    {"vt_term_rooted_use_wave", {"Termination", "Use Wave for Rooted"}},
    {"vt_epoch_graph_on_hang", {"Termination", "Output Epoch Graph on Hang"}},
    {"vt_epoch_graph_terse", {"Termination", "Terse Epoch Graph Output"}},
    {"vt_print_no_progress", {"Termination", "Print No Progress"}},
    {"vt_hang_freq", {"Termination", "Hang Check Frequency"}},

    // Debugging/Launch
    {"vt_pause", {"Launch", "Pause"}},

    // User Options
    {"vt_user_1", {"User Options", "User 1"}},
    {"vt_user_2", {"User Options", "User 2"}},
    {"vt_user_3", {"User Options", "User 3"}},
    {"vt_user_int_1", {"User Options", "User int 1"}},
    {"vt_user_int_2", {"User Options", "User int 2"}},
    {"vt_user_int_3", {"User Options", "User int 3"}},
    {"vt_user_str_1", {"User Options", "User str 1"}},
    {"vt_user_str_2", {"User Options", "User str 2"}},
    {"vt_user_str_3", {"User Options", "User str 3"}},

    // Scheduler Configuration
    {"vt_sched_num_progress", {"Scheduler Configuration", "Num Progress Times"}},
    {"vt_sched_progress_han", {"Scheduler Configuration", "Progress Handlers"}},
    {"vt_sched_progress_sec", {"Scheduler Configuration", "Progress Seconds"}},

    // Configuration File
    {"vt_output_config", {"Configuration File", "Enable Output Config"}},
    {"vt_output_config_file", {"Configuration File", "File"}},

    // Runtime
    {"vt_max_mpi_send_size", {"Runtime", "Max MPI Send Size"}},
    {"vt_no_assert_fail", {"Runtime", "Disable Assert Failure"}},
    {"vt_throw_on_abort", {"Runtime", "Throw on Abort"}}
  };

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | vt_color
      | vt_no_color
      | vt_auto_color
      | vt_quiet
      | colorize_output

      | vt_sched_num_progress
      | vt_sched_progress_han
      | vt_sched_progress_sec

      | vt_no_sigint
      | vt_no_sigsegv
      | vt_no_sigbus
      | vt_no_terminate

      | vt_memory_reporters
      | vt_print_memory_each_phase
      | vt_print_memory_node
      | vt_allow_memory_report_with_ps
      | vt_print_memory_at_threshold
      | vt_print_memory_threshold
      | vt_print_memory_sched_poll
      | vt_print_memory_footprint

      | vt_no_warn_stack
      | vt_no_assert_stack
      | vt_no_abort_stack
      | vt_no_stack
      | vt_stack_file
      | vt_stack_dir
      | vt_stack_mod

      | vt_trace
      | vt_trace_mpi
      | vt_trace_pmpi
      | vt_trace_sys_all
      | vt_trace_sys_term
      | vt_trace_sys_location
      | vt_trace_sys_collection
      | vt_trace_sys_serial_msg
      | vt_trace_file
      | vt_trace_dir
      | vt_trace_mod
      | vt_trace_flush_size
      | vt_trace_spec
      | vt_trace_spec_file
      | vt_trace_memory_usage
      | vt_trace_event_polling
      | vt_trace_irecv_polling

      | vt_lb
      | vt_lb_show_config
      | vt_lb_quiet
      | vt_lb_file_name
      | vt_lb_name
      | vt_lb_args
      | vt_lb_interval
      | vt_lb_data
      | vt_lb_data_compress
      | vt_lb_data_dir
      | vt_lb_data_file
      | vt_lb_data_in
      | vt_lb_data_dir_in
      | vt_lb_data_file_in
      | vt_lb_statistics
      | vt_lb_statistics_compress
      | vt_lb_statistics_file
      | vt_lb_statistics_dir
      | vt_help_lb_args
      | vt_lb_self_migration

      | vt_no_detect_hang
      | vt_print_no_progress
      | vt_epoch_graph_on_hang
      | vt_epoch_graph_terse
      | vt_term_rooted_use_ds
      | vt_term_rooted_use_wave
      | vt_hang_freq

      | vt_diag_enable
      | vt_diag_print_summary
      | vt_diag_summary_csv_file
      | vt_diag_summary_file
      | vt_diag_csv_base_units

      | vt_pause
      | vt_no_assert_fail
      | vt_throw_on_abort
      | vt_max_mpi_send_size

      | vt_debug_level
      | vt_debug_level_val

      | vt_debug_all
      | vt_debug_none
      | vt_debug_gen
      | vt_debug_runtime
      | vt_debug_active
      | vt_debug_term
      | vt_debug_termds
      | vt_debug_barrier
      | vt_debug_event
      | vt_debug_pipe
      | vt_debug_pool
      | vt_debug_reduce
      | vt_debug_rdma
      | vt_debug_rdma_channel
      | vt_debug_rdma_state
      | vt_debug_param
      | vt_debug_handler
      | vt_debug_hierlb
      | vt_debug_temperedlb
      | vt_debug_temperedwmin
      | vt_debug_scatter
      | vt_debug_serial_msg
      | vt_debug_trace
      | vt_debug_location
      | vt_debug_lb
      | vt_debug_vrt
      | vt_debug_vrt_coll
      | vt_debug_worker
      | vt_debug_group
      | vt_debug_broadcast
      | vt_debug_objgroup
      | vt_debug_phase
      | vt_debug_context
      | vt_debug_epoch
      | vt_debug_replay

      | vt_debug_print_flush

      | vt_user_1
      | vt_user_2
      | vt_user_3
      | vt_user_int_1
      | vt_user_int_2
      | vt_user_int_3
      | vt_user_str_1
      | vt_user_str_2
      | vt_user_str_3

      | vt_output_config
      | vt_output_config_file
      | vt_output_config_str

      | prog_name

      | argv_prog_name

      | passthru_args;
  }

  // std::vector<std::string> split_string(std::string input_str, std::string delimiter) {
  //   std::vector<std::string> splits;
  //   std::string segment;
  //   while (std::getline(input_str, segment, delimiter)) {
  //     splits.push_back(segment);
  //   }
  //   return splits;
  // }

  // void config_to_yaml_str(std::string ini_string) {
  //   std::string yaml_conf_str;
  //   std::istringstream iss(ini_string);
  //   std::string line;

  //   // Loop through every line of the .ini string
  //   while (std::getline(iss, line)) {
  //     std::vector<std::string> cli_key = split_string(line, "=");

  //     // Map the CLI key to the YAML key
  //     std::unordered_map<std::string, std::pair<std::string, std::string>> cli_to_yaml_args;
  //     auto it = cli_to_yaml_args.find(cli_key[0]);
  //     if (it != cli_to_yaml_args.end()) {
  //       auto yaml_key_pair = it->second;
  //       auto yaml_node_name = yaml_key_pair.first;
  //       auto yaml_key = yaml_key_pair.second;

  //       // Look for indication of nested structure
  //       if (yaml_node_name.find("/") != yaml_node_name.end()) {
  //         if (split_string(yaml_node_name, "/")[0] == "Debug Print Configuration") {
  //             // Handle the list
  //         } else {
  //             // Handle the generic Sub-Node
  //         }
  //       } else {
  //           // Handle the generic node
  //       }
  //     }
  //   }
  // }
};

}} /* end namespace vt::arguments */

namespace vt {

extern arguments::AppConfig* theConfig();

} /* end namespace vt */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_APP_CONFIG_H*/
