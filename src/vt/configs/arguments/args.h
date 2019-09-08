/*
//@HEADER
// ************************************************************************
//
//                          args.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H

#include "vt/config.h"

#include "CLI/CLI11.hpp"

namespace vt { namespace arguments {

struct Configs {

public:

  bool vt_color = true;
  bool vt_no_color = false;
  bool vt_auto_color = false;
  bool vt_quiet = false;

  bool vt_no_sigint = false;
  bool vt_no_sigsegv = false;
  bool vt_no_terminate = false;

  bool vt_no_warn_stack = false;
  bool vt_no_assert_stack = false;
  bool vt_no_abort_stack = false;
  bool vt_no_stack = false;
  std::string vt_stack_file = "";
  std::string vt_stack_dir = "";
  int32_t vt_stack_mod = 1;

  bool vt_trace = false;
  std::string vt_trace_file = "";
  std::string vt_trace_dir = "";
  int32_t vt_trace_mod = 1;

  bool vt_lb = false;
  bool vt_lb_file = false;
  bool vt_lb_quiet = false;
  std::string vt_lb_file_name = "balance.in";
  std::string vt_lb_name = "NoLB";
  int32_t vt_lb_interval = 1;
  bool vt_lb_stats = false;
  std::string vt_lb_stats_dir = "vt_lb_stats";
  std::string vt_lb_stats_file = "stats";

  bool vt_no_detect_hang = false;
  bool vt_term_rooted_use_ds = false;
  bool vt_term_rooted_use_wave = false;
  int64_t vt_hang_freq = 1024;

  bool vt_pause = false;

  bool vt_debug_all = false;
  bool vt_debug_verbose = false;
  bool vt_debug_none = false;
  bool vt_debug_gen = false;
  bool vt_debug_runtime = false;
  bool vt_debug_active = false;
  bool vt_debug_term = false;
  bool vt_debug_termds = false;
  bool vt_debug_barrier = false;
  bool vt_debug_event = false;
  bool vt_debug_pipe = false;
  bool vt_debug_pool = false;
  bool vt_debug_reduce = false;
  bool vt_debug_rdma = false;
  bool vt_debug_rdma_channel = false;
  bool vt_debug_rdma_state = false;
  bool vt_debug_param = false;
  bool vt_debug_handler = false;
  bool vt_debug_hierlb = false;
  bool vt_debug_scatter = false;
  bool vt_debug_sequence = false;
  bool vt_debug_sequence_vrt = false;
  bool vt_debug_serial_msg = false;
  bool vt_debug_trace = false;
  bool vt_debug_location = false;
  bool vt_debug_lb = false;
  bool vt_debug_vrt = false;
  bool vt_debug_vrt_coll = false;
  bool vt_debug_worker = false;
  bool vt_debug_group = false;
  bool vt_debug_broadcast = false;
  bool vt_debug_objgroup = false;

  bool vt_user_1 = false;
  bool vt_user_2 = false;
  bool vt_user_3 = false;
  int32_t vt_user_int_1 = 0;
  int32_t vt_user_int_2 = 0;
  int32_t vt_user_int_3 = 0;
  std::string vt_user_str_1 = "";
  std::string vt_user_str_2 = "";
  std::string vt_user_str_3 = "";

};


struct Args {

  static int parse(int& argc, char**& argv, const Configs &ref);

public:
  static Configs config;

private:
  static CLI::App app;
  static bool parsed;

};


inline bool user1() { return Args::config.vt_user_1; }
inline bool user2() { return Args::config.vt_user_2; }
inline bool user3() { return Args::config.vt_user_3; }


}} /* end namespace vt::arguments */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
