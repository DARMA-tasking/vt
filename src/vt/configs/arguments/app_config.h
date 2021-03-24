/*
//@HEADER
// *****************************************************************************
//
//                                app_config.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2020 National Technology & Engineering Solutions of Sandia, LLC
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

  bool vt_lb                  = false;
  bool vt_lb_show_spec        = false;
  bool vt_lb_quiet            = false;
  std::string vt_lb_file_name = "";
  std::string vt_lb_name      = "NoLB";
  std::string vt_lb_args      = "";
  int32_t vt_lb_interval      = 1;
  bool vt_lb_keep_last_elm    = false;
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
  std::size_t vt_max_mpi_send_size = 1ull << 30;

#if (vt_feature_fcontext != 0)
  bool vt_ult_disable = false;
  std::size_t vt_ult_stack_size = (1 << 21) - 64;
#endif

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
  bool vt_debug_phase        = false;
  bool vt_debug_context      = false;

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

  /// Arguments are being ref-returend as the result of parse(..).
  /// Does not include argv[0]. Original char* objects.
  std::vector<char*> passthru_args;


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
      | vt_lb_show_spec
      | vt_lb_quiet
      | vt_lb_file_name
      | vt_lb_name
      | vt_lb_args
      | vt_lb_interval
      | vt_lb_stats
      | vt_lb_stats_dir
      | vt_lb_stats_file
      | vt_lb_stats_dir_in
      | vt_lb_stats_file_in

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
      | vt_max_mpi_send_size

      | vt_debug_all
      | vt_debug_verbose
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
      | vt_debug_gossiplb
      | vt_debug_scatter
      | vt_debug_sequence
      | vt_debug_sequence_vrt
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
};

}} /* end namespace vt::arguments */

namespace vt {

extern arguments::AppConfig* theConfig();

} /* end namespace vt */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_APP_CONFIG_H*/
