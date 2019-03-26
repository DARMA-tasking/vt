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

struct ArgConfig {

  static int parse(int& argc, char**& argv);

public:
  static bool vt_color;
  static bool vt_no_color;
  static bool vt_auto_color;
  static bool vt_quiet;

  static bool vt_no_sigint;
  static bool vt_no_sigsegv;
  static bool vt_no_terminate;

  static bool vt_no_warn_stack;
  static bool vt_no_assert_stack;
  static bool vt_no_abort_stack;
  static bool vt_no_stack;
  static std::string vt_stack_file;
  static std::string vt_stack_dir;
  static int32_t vt_stack_mod;

  static bool vt_trace;
  static std::string vt_trace_file;
  static std::string vt_trace_dir;
  static int32_t vt_trace_mod;

  static bool vt_lb;
  static bool vt_lb_file;
  static std::string vt_lb_file_name;
  static std::string vt_lb_name;
  static int32_t vt_lb_interval;
  static bool vt_lb_stats;
  static std::string vt_lb_stats_dir;
  static std::string vt_lb_stats_file;

  static bool vt_no_detect_hang;
  static int64_t vt_hang_freq;

  static bool vt_pause;

  static bool vt_debug_all;
  static bool vt_debug_none;
  static bool vt_debug_gen;
  static bool vt_debug_runtime;
  static bool vt_debug_active;
  static bool vt_debug_term;
  static bool vt_debug_termds;
  static bool vt_debug_barrier;
  static bool vt_debug_event;
  static bool vt_debug_pipe;
  static bool vt_debug_pool;
  static bool vt_debug_reduce;
  static bool vt_debug_rdma;
  static bool vt_debug_rdma_channel;
  static bool vt_debug_rdma_state;
  static bool vt_debug_param;
  static bool vt_debug_handler;
  static bool vt_debug_hierlb;
  static bool vt_debug_scatter;
  static bool vt_debug_sequence;
  static bool vt_debug_sequence_vrt;
  static bool vt_debug_serial_msg;
  static bool vt_debug_trace;
  static bool vt_debug_location;
  static bool vt_debug_lb;
  static bool vt_debug_vrt;
  static bool vt_debug_vrt_coll;
  static bool vt_debug_worker;
  static bool vt_debug_group;
  static bool vt_debug_broadcast;
  static bool vt_debug_objgroup;

  static bool vt_user_1;
  static bool vt_user_2;
  static bool vt_user_3;
  static int32_t vt_user_int_1;
  static int32_t vt_user_int_2;
  static int32_t vt_user_int_3;
  static std::string vt_user_str_1;
  static std::string vt_user_str_2;
  static std::string vt_user_str_3;

private:
  static CLI::App app;
  static bool parsed;
};

inline bool user1() { return ArgConfig::vt_user_1; }
inline bool user2() { return ArgConfig::vt_user_2; }
inline bool user3() { return ArgConfig::vt_user_3; }

}} /* end namespace vt::arguments */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
