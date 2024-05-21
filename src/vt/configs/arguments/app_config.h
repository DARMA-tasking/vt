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
#include <tuple>
#include <variant>
#include <fstream>
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

  using variantArg_t = std::variant<bool, int32_t, int64_t, double, std::size_t, std::string>;

  std::vector<std::string> splitString(std::string input_str) {
    std::vector<std::string> splits;
    long unsigned int pos = 0;
    while (input_str.find("/") != input_str.npos) {
      pos = input_str.find("/");
      splits.push_back(input_str.substr(0, pos));
      input_str.erase(0, pos + 1);
    }
    splits.push_back(input_str);
    return splits;
  }

  void addVariantToNode(YAML::Node& node, std::string key, variantArg_t variant_val) {
      // Get the yaml_val from the variant
      if (std::holds_alternative<bool>(variant_val)) {
        node[key] = std::get<bool>(variant_val);
      } else if (std::holds_alternative<int32_t>(variant_val)) {
        node[key] = std::get<int32_t>(variant_val);
      } else if (std::holds_alternative<int64_t>(variant_val)) {
        node[key] = std::get<int64_t>(variant_val);
      } else if (std::holds_alternative<double>(variant_val)) {
        node[key] = std::get<double>(variant_val);
      } else if (std::holds_alternative<std::size_t>(variant_val)) {
        node[key] = std::get<std::size_t>(variant_val);
      } else if (std::holds_alternative<std::string>(variant_val)) {
        node[key] = std::get<std::string>(variant_val);
      } else {
        throw std::runtime_error("Argument type not recognized.");
      }
  }

  YAML::Node convertConfigToYaml() {

    // First, create a converter vector of tuples {Node(s), Key, Value}
    std::vector< std::tuple<std::string, std::string, variantArg_t> > cli_to_yaml_args =
    {
      // Output Control
      {"Output Control", "Color", static_cast<variantArg_t>(vt_color)},
      {"Output Control", "Color", static_cast<variantArg_t>(not vt_no_color)}, // overwrites vt_color
      {"Output Control", "Quiet", static_cast<variantArg_t>(vt_quiet)},

      // Signal Handling
      {"Signal Handling", "Disable SIGINT", static_cast<variantArg_t>(vt_no_sigint)},
      {"Signal Handling", "Disable SIGSEGV", static_cast<variantArg_t>(vt_no_sigsegv)},
      {"Signal Handling", "Disable SIGBUS", static_cast<variantArg_t>(vt_no_sigbus)},
      {"Signal Handling", "Disable Terminate Signal", static_cast<variantArg_t>(vt_no_terminate)},

      // Memory Usage Reporting
      {"Memory Usage Reporting", "Memory Reporters", static_cast<variantArg_t>(vt_memory_reporters)},
      {"Memory Usage Reporting", "Print Memory Each Phase", static_cast<variantArg_t>(vt_print_memory_each_phase)},
      {"Memory Usage Reporting", "Print Memory on Node", static_cast<variantArg_t>(vt_print_memory_node)},
      {"Memory Usage Reporting", "Allow Memory Report With ps", static_cast<variantArg_t>(vt_allow_memory_report_with_ps)},
      {"Memory Usage Reporting", "Print Memory Threshold", static_cast<variantArg_t>(vt_print_memory_threshold)},
      {"Memory Usage Reporting", "Print Memory Scheduler Poll", static_cast<variantArg_t>(vt_print_memory_sched_poll)},
      {"Memory Usage Reporting", "Print Memory Footprint", static_cast<variantArg_t>(vt_print_memory_footprint)},

      // Dump Stack Backtrace
      {"Dump Stack Backtrace", "Enable Stack Output on Warning", static_cast<variantArg_t>(vt_no_warn_stack)},
      {"Dump Stack Backtrace", "Enable Stack Output on Assert", static_cast<variantArg_t>(vt_no_assert_stack)},
      {"Dump Stack Backtrace", "Enable Stack Output on Abort", static_cast<variantArg_t>(vt_no_abort_stack)},
      {"Dump Stack Backtrace", "Enable Stack Output", static_cast<variantArg_t>(vt_no_stack)},
      {"Dump Stack Backtrace", "File", static_cast<variantArg_t>(vt_stack_file)},
      {"Dump Stack Backtrace", "Directory", static_cast<variantArg_t>(vt_stack_dir)},
      {"Dump Stack Backtrace", "Output Rank Mod", static_cast<variantArg_t>(vt_stack_mod)},

      // Tracing Configuration
      {"Tracing Configuration", "Enabled", static_cast<variantArg_t>(vt_trace)},
      {"Tracing Configuration", "MPI Type Events", static_cast<variantArg_t>(vt_trace_mpi)},
      {"Tracing Configuration", "MPI Type Events", static_cast<variantArg_t>(vt_trace_pmpi)},
      {"Tracing Configuration", "File", static_cast<variantArg_t>(vt_trace_file)},
      {"Tracing Configuration", "Directory", static_cast<variantArg_t>(vt_trace_dir)},
      {"Tracing Configuration", "Output Rank Mod", static_cast<variantArg_t>(vt_trace_mod)},
      {"Tracing Configuration", "Flush Size", static_cast<variantArg_t>(vt_trace_flush_size)},
      {"Tracing Configuration", "GZip Finish Flush", static_cast<variantArg_t>(vt_trace_gzip_finish_flush)},
      {"Tracing Configuration", "Include All System Events", static_cast<variantArg_t>(vt_trace_sys_all)},
      {"Tracing Configuration", "Include Termination Events", static_cast<variantArg_t>(vt_trace_sys_term)},
      {"Tracing Configuration", "Include Location Events", static_cast<variantArg_t>(vt_trace_sys_location)},
      {"Tracing Configuration", "Include Collection Events", static_cast<variantArg_t>(vt_trace_sys_collection)},
      {"Tracing Configuration", "Include Message Serialization Events", static_cast<variantArg_t>(vt_trace_sys_serial_msg)},
      {"Tracing Configuration", "Specification Enabled", static_cast<variantArg_t>(vt_trace_spec)},
      {"Tracing Configuration", "Spec File", static_cast<variantArg_t>(vt_trace_spec_file)},
      {"Tracing Configuration", "Memory Usage", static_cast<variantArg_t>(vt_trace_memory_usage)},
      {"Tracing Configuration", "Event Polling", static_cast<variantArg_t>(vt_trace_event_polling)},
      {"Tracing Configuration", "IRecv Polling", static_cast<variantArg_t>(vt_trace_irecv_polling)},

      // Debug Print Configuration
      {"Debug Print Configuration", "Level", static_cast<variantArg_t>(vt_debug_level)},
      {"Debug Print Configuration", "Enable All", static_cast<variantArg_t>(vt_debug_all)},
      {"Debug Print Configuration", "Disable All", static_cast<variantArg_t>(vt_debug_none)},
      {"Debug Print Configuration", "Debug Print Flush", static_cast<variantArg_t>(vt_debug_print_flush)},
      {"Debug Print Configuration/Enable", "gen", static_cast<variantArg_t>(vt_debug_gen)},
      {"Debug Print Configuration/Enable", "runtime", static_cast<variantArg_t>(vt_debug_runtime)},
      {"Debug Print Configuration/Enable", "active", static_cast<variantArg_t>(vt_debug_active)},
      {"Debug Print Configuration/Enable", "term", static_cast<variantArg_t>(vt_debug_term)},
      {"Debug Print Configuration/Enable", "termds", static_cast<variantArg_t>(vt_debug_termds)},
      {"Debug Print Configuration/Enable", "barrier", static_cast<variantArg_t>(vt_debug_barrier)},
      {"Debug Print Configuration/Enable", "event", static_cast<variantArg_t>(vt_debug_event)},
      {"Debug Print Configuration/Enable", "pipe", static_cast<variantArg_t>(vt_debug_pipe)},
      {"Debug Print Configuration/Enable", "pool", static_cast<variantArg_t>(vt_debug_pool)},
      {"Debug Print Configuration/Enable", "reduce", static_cast<variantArg_t>(vt_debug_reduce)},
      {"Debug Print Configuration/Enable", "rdma", static_cast<variantArg_t>(vt_debug_rdma)},
      {"Debug Print Configuration/Enable", "rdma_channel", static_cast<variantArg_t>(vt_debug_rdma_channel)},
      {"Debug Print Configuration/Enable", "rdma_state", static_cast<variantArg_t>(vt_debug_rdma_state)},
      {"Debug Print Configuration/Enable", "handler", static_cast<variantArg_t>(vt_debug_handler)},
      {"Debug Print Configuration/Enable", "hierlb", static_cast<variantArg_t>(vt_debug_hierlb)},
      {"Debug Print Configuration/Enable", "temperedlb", static_cast<variantArg_t>(vt_debug_temperedlb)},
      {"Debug Print Configuration/Enable", "temperedwmin", static_cast<variantArg_t>(vt_debug_temperedwmin)},
      {"Debug Print Configuration/Enable", "scatter", static_cast<variantArg_t>(vt_debug_scatter)},
      {"Debug Print Configuration/Enable", "serial_msg", static_cast<variantArg_t>(vt_debug_serial_msg)},
      {"Debug Print Configuration/Enable", "trace", static_cast<variantArg_t>(vt_debug_trace)},
      {"Debug Print Configuration/Enable", "location", static_cast<variantArg_t>(vt_debug_location)},
      {"Debug Print Configuration/Enable", "lb", static_cast<variantArg_t>(vt_debug_lb)},
      {"Debug Print Configuration/Enable", "vrt", static_cast<variantArg_t>(vt_debug_vrt)},
      {"Debug Print Configuration/Enable", "vrt_coll", static_cast<variantArg_t>(vt_debug_vrt_coll)},
      {"Debug Print Configuration/Enable", "worker", static_cast<variantArg_t>(vt_debug_worker)},
      {"Debug Print Configuration/Enable", "group", static_cast<variantArg_t>(vt_debug_group)},
      {"Debug Print Configuration/Enable", "broadcast", static_cast<variantArg_t>(vt_debug_broadcast)},
      {"Debug Print Configuration/Enable", "objgroup", static_cast<variantArg_t>(vt_debug_objgroup)},
      {"Debug Print Configuration/Enable", "phase", static_cast<variantArg_t>(vt_debug_phase)},
      {"Debug Print Configuration/Enable", "context", static_cast<variantArg_t>(vt_debug_context)},
      {"Debug Print Configuration/Enable", "epoch", static_cast<variantArg_t>(vt_debug_epoch)},

      // Load Balancing
      {"Load Balancing", "Enabled", static_cast<variantArg_t>(vt_lb)},
      {"Load Balancing", "Quiet", static_cast<variantArg_t>(vt_lb_quiet)},
      {"Load Balancing", "File", static_cast<variantArg_t>(vt_lb_file_name)},
      {"Load Balancing", "Show Configuration", static_cast<variantArg_t>(vt_lb_show_config)},
      {"Load Balancing", "Name", static_cast<variantArg_t>(vt_lb_name)},
      {"Load Balancing", "Arguments", static_cast<variantArg_t>(vt_lb_args)},
      {"Load Balancing", "Interval", static_cast<variantArg_t>(vt_lb_interval)},
      {"Load Balancing", "Keep Last Element", static_cast<variantArg_t>(vt_lb_keep_last_elm)},
      {"Load Balancing/LB Data Output", "Enabled", static_cast<variantArg_t>(vt_lb_data)},
      {"Load Balancing/LB Data Output", "Directory", static_cast<variantArg_t>(vt_lb_data_dir)},
      {"Load Balancing/LB Data Output", "File", static_cast<variantArg_t>(vt_lb_data_file)},
      {"Load Balancing/LB Data Input", "Enabled", static_cast<variantArg_t>(vt_lb_data_in)},
      {"Load Balancing/LB Data Input", "Enable Compression", static_cast<variantArg_t>(vt_lb_data_compress)},
      {"Load Balancing/LB Data Input", "Directory", static_cast<variantArg_t>(vt_lb_data_dir_in)},
      {"Load Balancing/LB Data Input", "File", static_cast<variantArg_t>(vt_lb_data_file_in)},
      {"Load Balancing/LB Statistics", "Enabled", static_cast<variantArg_t>(vt_lb_statistics)},
      {"Load Balancing/LB Statistics", "Enable Compression", static_cast<variantArg_t>(vt_lb_statistics_compress)},
      {"Load Balancing/LB Statistics", "File", static_cast<variantArg_t>(vt_lb_statistics_file)},
      {"Load Balancing/LB Statistics", "Directory", static_cast<variantArg_t>(vt_lb_statistics_dir)},
      {"Load Balancing", "Enable Self Migration", static_cast<variantArg_t>(vt_lb_self_migration)},
      {"Load Balancing", "Enable Specification", static_cast<variantArg_t>(vt_lb_spec)},
      {"Load Balancing", "Specification File", static_cast<variantArg_t>(vt_lb_spec_file)},

      // Diagnostics
      {"Diagnostics", "Enabled", static_cast<variantArg_t>(vt_diag_enable)},
      {"Diagnostics", "Enable Print Summary", static_cast<variantArg_t>(vt_diag_print_summary)},
      {"Diagnostics", "Summary File", static_cast<variantArg_t>(vt_diag_summary_file)},
      {"Diagnostics", "Summary CSV File", static_cast<variantArg_t>(vt_diag_summary_csv_file)},
      {"Diagnostics", "Use CSV Base Units", static_cast<variantArg_t>(vt_diag_csv_base_units)},

      // Termination
      {"Termination", "Detect Hangs", static_cast<variantArg_t>(vt_no_detect_hang)},
      {"Termination", "Use DS for Rooted", static_cast<variantArg_t>(vt_term_rooted_use_ds)},
      {"Termination", "Use Wave for Rooted", static_cast<variantArg_t>(vt_term_rooted_use_wave)},
      {"Termination", "Output Epoch Graph on Hang", static_cast<variantArg_t>(vt_epoch_graph_on_hang)},
      {"Termination", "Terse Epoch Graph Output", static_cast<variantArg_t>(vt_epoch_graph_terse)},
      {"Termination", "Print No Progress", static_cast<variantArg_t>(vt_print_no_progress)},
      {"Termination", "Hang Check Frequency", static_cast<variantArg_t>(vt_hang_freq)},

      // Debugging/Launch
      {"Launch", "Pause", static_cast<variantArg_t>(vt_pause)},

      // User Options
      {"User Options", "User 1", static_cast<variantArg_t>(vt_user_1)},
      {"User Options", "User 2", static_cast<variantArg_t>(vt_user_2)},
      {"User Options", "User 3", static_cast<variantArg_t>(vt_user_3)},
      {"User Options", "User int 1", static_cast<variantArg_t>(vt_user_int_1)},
      {"User Options", "User int 2", static_cast<variantArg_t>(vt_user_int_2)},
      {"User Options", "User int 3", static_cast<variantArg_t>(vt_user_int_3)},
      {"User Options", "User str 1", static_cast<variantArg_t>(vt_user_str_1)},
      {"User Options", "User str 2", static_cast<variantArg_t>(vt_user_str_2)},
      {"User Options", "User str 3", static_cast<variantArg_t>(vt_user_str_3)},

      // Scheduler Configuration
      {"Scheduler Configuration", "Num Progress Times", static_cast<variantArg_t>(vt_sched_num_progress)},
      {"Scheduler Configuration", "Progress Handlers", static_cast<variantArg_t>(vt_sched_progress_han)},
      {"Scheduler Configuration", "Progress Seconds", static_cast<variantArg_t>(vt_sched_progress_sec)},

      // Configuration File
      {"Configuration File", "Enable Output Config", static_cast<variantArg_t>(vt_output_config)},
      {"Configuration File", "File", static_cast<variantArg_t>(vt_output_config_file)},

      // Runtime
      {"Runtime", "Max MPI Send Size", static_cast<variantArg_t>(vt_max_mpi_send_size)},
      {"Runtime", "Disable Assert Failure", static_cast<variantArg_t>(vt_no_assert_fail)},
      {"Runtime", "Throw on Abort", static_cast<variantArg_t>(vt_throw_on_abort)}
    };

    // Create an empty node that we will populate
    YAML::Node output_config_yaml;

    // Then convert to YAML
    for (const auto& yaml_data : cli_to_yaml_args) {

      // Unpack the yaml data
      auto yaml_node = std::get<0>(yaml_data);
      auto yaml_key = std::get<1>(yaml_data);
      auto yaml_val = std::get<2>(yaml_data);

      // First, explicitly handle the Debug Print Configuration list
      if (yaml_node == "Debug Print Configuration/Enable") {
        if (std::get<bool>(yaml_val)) {
          output_config_yaml["Debug Print Configuration"]["Enable"].push_back(yaml_key);
        }
      }
      // Then handle any nested nodes (with "/" in them)
      else if (yaml_node.find("/") != yaml_node.npos) {
        auto nodes = splitString(yaml_node);
        auto current_node = output_config_yaml[nodes[0]][nodes[1]]; // TODO: generalize this
        addVariantToNode(current_node, yaml_key, yaml_val);
      }
      // The rest are straightforward
      else {
        auto current_node = output_config_yaml[yaml_node];
        addVariantToNode(current_node, yaml_key, yaml_val);
      }
    }
  return output_config_yaml;
  }

  void writeConfigToYaml() {
    auto output_config_yaml = convertConfigToYaml();
    std::ofstream fout(vt_output_config_file);
    fout << output_config_yaml;
  }
};

}} /* end namespace vt::arguments */

namespace vt {

extern arguments::AppConfig* theConfig();

} /* end namespace vt */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_APP_CONFIG_H*/
