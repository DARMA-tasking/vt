/*
//@HEADER
// *****************************************************************************
//
//                                   args.cc
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

#include "vt/config.h"
#include "vt/configs/arguments/args.h"

#include <string>
#include <vector>

#include "CLI/CLI11.hpp"

namespace vt { namespace arguments {

/*static*/ bool        ArgConfig::vt_color              = true;
/*static*/ bool        ArgConfig::vt_no_color           = false;
/*static*/ bool        ArgConfig::vt_quiet              = false;

/*static*/ bool        ArgConfig::colorize_output       = false;
/*static*/ int32_t     ArgConfig::vt_sched_num_progress = 2;
/*static*/ int32_t     ArgConfig::vt_sched_progress_han = 0;
/*static*/ double      ArgConfig::vt_sched_progress_sec = 0.0;
/*static*/ bool        ArgConfig::vt_no_sigint          = false;
/*static*/ bool        ArgConfig::vt_no_sigsegv         = false;
/*static*/ bool        ArgConfig::vt_no_terminate       = false;
/*static*/ std::string ArgConfig::vt_memory_reporters   =
# if backend_check_enabled(mimalloc)
  "mimalloc,"
# endif
  "mstats,machinfo,selfstat,selfstatm,sbrk,mallinfo,getrusage,ps";
/*static*/ bool        ArgConfig::vt_print_memory_each_phase = false;
/*static*/ std::string ArgConfig::vt_print_memory_node  = "0";
/*static*/ bool        ArgConfig::vt_allow_memory_report_with_ps = false;

/*static*/ bool        ArgConfig::vt_no_warn_stack      = false;
/*static*/ bool        ArgConfig::vt_no_assert_stack    = false;
/*static*/ bool        ArgConfig::vt_no_abort_stack     = false;
/*static*/ bool        ArgConfig::vt_no_stack           = false;
/*static*/ std::string ArgConfig::vt_stack_file         = "";
/*static*/ std::string ArgConfig::vt_stack_dir          = "";
/*static*/ int32_t     ArgConfig::vt_stack_mod          = 0;

/*static*/ bool        ArgConfig::vt_trace              = false;
/*static*/ bool        ArgConfig::vt_trace_mpi          = false;
/*static*/ std::string ArgConfig::vt_trace_file         = "";
/*static*/ std::string ArgConfig::vt_trace_dir          = "";
/*static*/ int32_t     ArgConfig::vt_trace_mod          = 0;
/*static*/ int32_t     ArgConfig::vt_trace_flush_size   = 0;
/*static*/ bool        ArgConfig::vt_trace_sys_all        = false;
/*static*/ bool        ArgConfig::vt_trace_sys_term       = false;
/*static*/ bool        ArgConfig::vt_trace_sys_location   = false;
/*static*/ bool        ArgConfig::vt_trace_sys_collection = false;
/*static*/ bool        ArgConfig::vt_trace_sys_serial_msg = false;
/*static*/ bool        ArgConfig::vt_trace_spec           = false;
/*static*/ std::string ArgConfig::vt_trace_spec_file      = "";
/*static*/ bool        ArgConfig::vt_trace_memory_usage   = false;


/*static*/ bool        ArgConfig::vt_lb                 = false;
/*static*/ bool        ArgConfig::vt_lb_file            = false;
/*static*/ bool        ArgConfig::vt_lb_quiet           = false;
/*static*/ std::string ArgConfig::vt_lb_file_name       = "";
/*static*/ std::string ArgConfig::vt_lb_name            = "NoLB";
/*static*/ std::string ArgConfig::vt_lb_args            = "";
/*static*/ int32_t     ArgConfig::vt_lb_interval        = 1;
/*static*/ bool        ArgConfig::vt_lb_stats           = false;
/*static*/ std::string ArgConfig::vt_lb_stats_dir       = "vt_lb_stats";
/*static*/ std::string ArgConfig::vt_lb_stats_file      = "stats";
/*static*/ std::string ArgConfig::vt_lb_stats_dir_in    = "vt_lb_stats_in";
/*static*/ std::string ArgConfig::vt_lb_stats_file_in   = "stats_in";

/*static*/ bool        ArgConfig::vt_term_rooted_use_ds = false;
/*static*/ bool        ArgConfig::vt_term_rooted_use_wave = false;
/*static*/ bool        ArgConfig::vt_no_detect_hang     = false;
/*static*/ int64_t     ArgConfig::vt_hang_freq          = 1024;
/*static*/ bool        ArgConfig::vt_epoch_graph_on_hang= true;
/*static*/ bool        ArgConfig::vt_epoch_graph_terse  = false;
/*static*/ bool        ArgConfig::vt_print_no_progress  = true;

/*static*/ bool        ArgConfig::vt_pause              = false;

/*static*/ bool        ArgConfig::vt_debug_all          = false;
/*static*/ bool        ArgConfig::vt_debug_verbose      = false;
/*static*/ bool        ArgConfig::vt_debug_none         = false;
/*static*/ bool        ArgConfig::vt_debug_gen          = false;
/*static*/ bool        ArgConfig::vt_debug_runtime      = false;
/*static*/ bool        ArgConfig::vt_debug_active       = false;
/*static*/ bool        ArgConfig::vt_debug_term         = false;
/*static*/ bool        ArgConfig::vt_debug_termds       = false;
/*static*/ bool        ArgConfig::vt_debug_barrier      = false;
/*static*/ bool        ArgConfig::vt_debug_event        = false;
/*static*/ bool        ArgConfig::vt_debug_pipe         = false;
/*static*/ bool        ArgConfig::vt_debug_pool         = false;
/*static*/ bool        ArgConfig::vt_debug_reduce       = false;
/*static*/ bool        ArgConfig::vt_debug_rdma         = false;
/*static*/ bool        ArgConfig::vt_debug_rdma_channel = false;
/*static*/ bool        ArgConfig::vt_debug_rdma_state   = false;
/*static*/ bool        ArgConfig::vt_debug_param        = false;
/*static*/ bool        ArgConfig::vt_debug_handler      = false;
/*static*/ bool        ArgConfig::vt_debug_hierlb       = false;
/*static*/ bool        ArgConfig::vt_debug_gossiplb     = false;
/*static*/ bool        ArgConfig::vt_debug_scatter      = false;
/*static*/ bool        ArgConfig::vt_debug_sequence     = false;
/*static*/ bool        ArgConfig::vt_debug_sequence_vrt = false;
/*static*/ bool        ArgConfig::vt_debug_serial_msg   = false;
/*static*/ bool        ArgConfig::vt_debug_trace        = false;
/*static*/ bool        ArgConfig::vt_debug_location     = false;
/*static*/ bool        ArgConfig::vt_debug_lb           = false;
/*static*/ bool        ArgConfig::vt_debug_vrt          = false;
/*static*/ bool        ArgConfig::vt_debug_vrt_coll     = false;
/*static*/ bool        ArgConfig::vt_debug_worker       = false;
/*static*/ bool        ArgConfig::vt_debug_group        = false;
/*static*/ bool        ArgConfig::vt_debug_broadcast    = false;
/*static*/ bool        ArgConfig::vt_debug_objgroup     = false;

/*static*/ bool        ArgConfig::vt_user_1             = false;
/*static*/ bool        ArgConfig::vt_user_2             = false;
/*static*/ bool        ArgConfig::vt_user_3             = false;
/*static*/ int32_t     ArgConfig::vt_user_int_1         = 0;
/*static*/ int32_t     ArgConfig::vt_user_int_2         = 0;
/*static*/ int32_t     ArgConfig::vt_user_int_3         = 0;
/*static*/ std::string ArgConfig::vt_user_str_1         = "";
/*static*/ std::string ArgConfig::vt_user_str_2         = "";
/*static*/ std::string ArgConfig::vt_user_str_3         = "";

/*static*/ bool        ArgConfig::parsed                = false;

/*static*/ int ArgConfig::parse(int& argc, char**& argv) {
  static CLI::App app{"vt"};

  if (parsed || argc == 0 || argv == nullptr) {
    return 0;
  }

  // CLI11 app parser expects to get the arguments in *reverse* order!
  std::vector<std::string> args;
  for (auto i = argc-1; i > 0; i--) {
    args.push_back(std::string(argv[i]));
  }

  // fmt::print("argc={}, argv={}\n", argc, print_ptr(argv));

  /*
   * Flags for controlling the colorization of output from vt
   */
  auto quiet  = "Quiet the output from vt (only errors, warnings)";
  auto always = "Colorize output (default)";
  auto never  = "Do not colorize output (overrides --vt_color)";
  auto a  = app.add_flag("-c,--vt_color",      vt_color,      always);
  auto b  = app.add_flag("-n,--vt_no_color",   vt_no_color,   never);
  auto a1 = app.add_flag("-q,--vt_quiet",      vt_quiet,      quiet);
  auto outputGroup = "Output Control";
  a->group(outputGroup);
  b->group(outputGroup);
  a1->group(outputGroup);
  b->excludes(a);

  /*
   * Flags for controlling the signals that VT tries to catch
   */
  auto no_sigint      = "Do not register signal handler for SIGINT";
  auto no_sigsegv     = "Do not register signal handler for SIGSEGV";
  auto no_terminate   = "Do not register handler for std::terminate";
  auto d = app.add_flag("--vt_no_SIGINT",    vt_no_sigint,    no_sigint);
  auto e = app.add_flag("--vt_no_SIGSEGV",   vt_no_sigsegv,   no_sigsegv);
  auto f = app.add_flag("--vt_no_terminate", vt_no_terminate, no_terminate);
  auto signalGroup = "Signal Handling";
  d->group(signalGroup);
  e->group(signalGroup);
  f->group(signalGroup);


  /*
   * Flags for controlling memory usage reporting
   */
  auto mem_desc  = "List of memory reporters to query in order of precedence";
  auto mem_phase = "Print memory usage each new phase";
  auto mem_node  = "Node to print memory usage from or \"all\"";
  auto mem_ps    = "Enable memory reporting with PS (warning: forking to query 'ps' may not be scalable)";
  auto mm = app.add_option("--vt_memory_reporters", vt_memory_reporters, mem_desc, true);
  auto mn = app.add_flag("--vt_print_memory_each_phase", vt_print_memory_each_phase, mem_phase);
  auto mo = app.add_option("--vt_print_memory_node", vt_print_memory_node, mem_node, true);
  auto mp = app.add_flag("--vt_allow_memory_report_with_ps", vt_allow_memory_report_with_ps, mem_ps);
  auto memoryGroup = "Memory Usage Reporting";
  mm->group(memoryGroup);
  mn->group(memoryGroup);
  mo->group(memoryGroup);
  mp->group(memoryGroup);


  /*
   * Flags to control stack dumping
   */
  auto stack  = "Do not dump stack traces";
  auto warn   = "Do not dump stack traces when vtWarn(..) is invoked";
  auto assert = "Do not dump stack traces when vtAssert(..) is invoked";
  auto abort  = "Do not dump stack traces when vtAabort(..) is invoked";
  auto file   = "Dump stack traces to file instead of stdout";
  auto dir    = "Name of directory to write stack files";
  auto mod    = "Write stack dump if (node % vt_stack_mod) == 0";
  auto g = app.add_flag("--vt_no_warn_stack",   vt_no_warn_stack,   warn);
  auto h = app.add_flag("--vt_no_assert_stack", vt_no_assert_stack, assert);
  auto i = app.add_flag("--vt_no_abort_stack",  vt_no_abort_stack,  abort);
  auto j = app.add_flag("--vt_no_stack",        vt_no_stack,        stack);
  auto k = app.add_option("--vt_stack_file",    vt_stack_file,      file, "");
  auto l = app.add_option("--vt_stack_dir",     vt_stack_dir,       dir,  "");
  auto m = app.add_option("--vt_stack_mod",     vt_stack_mod,       mod,  1);
  auto stackGroup = "Dump Stack Backtrace";
  g->group(stackGroup);
  h->group(stackGroup);
  i->group(stackGroup);
  j->group(stackGroup);
  k->group(stackGroup);
  l->group(stackGroup);
  m->group(stackGroup);


  /*
   * Flags to control tracing output
   */
  auto trace     = "Enable tracing (must be compiled with trace_enabled)";
  auto trace_mpi = "Enable tracing of MPI calls (must be compiled with "
                   "trace_enabled)";
  auto tfile     = "Name of trace files";
  auto tdir      = "Name of directory for trace files";
  auto tmod      = "Output trace file if (node % vt_stack_mod) == 0";
  auto tflushmod = "Flush output trace every (vt_trace_flush_size) trace records";
  auto tsysall   = "Trace all system events";
  auto tsysTD    = "Trace system termination events";
  auto tsysloc   = "Trace system location manager events";
  auto tsyscoll  = "Trace system virtual context collection events";
  auto tsyssmsg  = "Trace system serialization manager events";
  auto tspec     = "Enable trace spec file (defines which phases tracing is on)";
  auto tspecfile = "File containing trace spec; --vt_trace_spec to enable";
  auto tmemusage = "Trace memory usage using first memory reporter";
  auto n  = app.add_flag("--vt_trace",              vt_trace,           trace);
  auto nm = app.add_flag("--vt_trace_mpi",          vt_trace_mpi,       trace_mpi);
  auto o  = app.add_option("--vt_trace_file",       vt_trace_file,      tfile, "");
  auto p  = app.add_option("--vt_trace_dir",        vt_trace_dir,       tdir,  "");
  auto q  = app.add_option("--vt_trace_mod",        vt_trace_mod,       tmod,  1);
  auto qf = app.add_option("--vt_trace_flush_size", vt_trace_flush_size,tflushmod,
    0);
  auto qt = app.add_flag("--vt_trace_sys_all",        vt_trace_sys_all,        tsysall);
  auto qw = app.add_flag("--vt_trace_sys_term",       vt_trace_sys_term,       tsysTD);
  auto qx = app.add_flag("--vt_trace_sys_location",   vt_trace_sys_location,   tsysloc);
  auto qy = app.add_flag("--vt_trace_sys_collection", vt_trace_sys_collection, tsyscoll);
  auto qz = app.add_flag("--vt_trace_sys_serial_msg", vt_trace_sys_serial_msg, tsyssmsg);
  auto qza = app.add_flag("--vt_trace_spec",          vt_trace_spec,           tspec);
  auto qzb = app.add_option("--vt_trace_spec_file",   vt_trace_spec_file,      tspecfile, "");
  auto qzc = app.add_flag("--vt_trace_memory_usage",  vt_trace_memory_usage,   tmemusage);
  auto traceGroup = "Tracing Configuration";
  n->group(traceGroup);
  nm->group(traceGroup);
  o->group(traceGroup);
  p->group(traceGroup);
  q->group(traceGroup);
  qf->group(traceGroup);
  qt->group(traceGroup);
  qw->group(traceGroup);
  qx->group(traceGroup);
  qy->group(traceGroup);
  qz->group(traceGroup);
  qza->group(traceGroup);
  qzb->group(traceGroup);
  qzc->group(traceGroup);


  /*
   * Flags for controlling debug print output at runtime
   */

  #define debug_pp(opt) +std::string(vt::config::PrettyPrintCat<config::opt>::str)+

  auto rp  = "Enable all debug prints";
  auto rq  = "Enable verbose debug prints";
  auto aap = "Enable debug_none         = \"" debug_pp(none)         "\"";
  auto bap = "Enable debug_gen          = \"" debug_pp(gen)          "\"";
  auto cap = "Enable debug_runtime      = \"" debug_pp(runtime)      "\"";
  auto dap = "Enable debug_active       = \"" debug_pp(active)       "\"";
  auto eap = "Enable debug_term         = \"" debug_pp(term)         "\"";
  auto fap = "Enable debug_termds       = \"" debug_pp(termds)       "\"";
  auto gap = "Enable debug_barrier      = \"" debug_pp(barrier)      "\"";
  auto hap = "Enable debug_event        = \"" debug_pp(event)        "\"";
  auto iap = "Enable debug_pipe         = \"" debug_pp(pipe)         "\"";
  auto jap = "Enable debug_pool         = \"" debug_pp(pool)         "\"";
  auto kap = "Enable debug_reduce       = \"" debug_pp(reduce)       "\"";
  auto lap = "Enable debug_rdma         = \"" debug_pp(rdma)         "\"";
  auto map = "Enable debug_rdma_channel = \"" debug_pp(rdma_channel) "\"";
  auto nap = "Enable debug_rdma_state   = \"" debug_pp(rdma_state)   "\"";
  auto oap = "Enable debug_param        = \"" debug_pp(param)        "\"";
  auto pap = "Enable debug_handler      = \"" debug_pp(handler)      "\"";
  auto qap = "Enable debug_hierlb       = \"" debug_pp(hierlb)       "\"";
  auto qbp = "Enable debug_gossiplb     = \"" debug_pp(gossiplb)     "\"";
  auto rap = "Enable debug_scatter      = \"" debug_pp(scatter)      "\"";
  auto sap = "Enable debug_sequence     = \"" debug_pp(sequence)     "\"";
  auto tap = "Enable debug_sequence_vrt = \"" debug_pp(sequence_vrt) "\"";
  auto uap = "Enable debug_serial_msg   = \"" debug_pp(serial_msg)   "\"";
  auto vap = "Enable debug_trace        = \"" debug_pp(trace)        "\"";
  auto wap = "Enable debug_location     = \"" debug_pp(location)     "\"";
  auto xap = "Enable debug_lb           = \"" debug_pp(lb)           "\"";
  auto yap = "Enable debug_vrt          = \"" debug_pp(vrt)          "\"";
  auto zap = "Enable debug_vrt_coll     = \"" debug_pp(vrt_coll)     "\"";
  auto abp = "Enable debug_worker       = \"" debug_pp(worker)       "\"";
  auto bbp = "Enable debug_group        = \"" debug_pp(group)        "\"";
  auto cbp = "Enable debug_broadcast    = \"" debug_pp(broadcast)    "\"";
  auto dbp = "Enable debug_objgroup     = \"" debug_pp(objgroup)     "\"";

  auto r  = app.add_flag("--vt_debug_all",          vt_debug_all,          rp);
  auto r1 = app.add_flag("--vt_debug_verbose",      vt_debug_verbose,      rq);
  auto aa = app.add_flag("--vt_debug_none",         vt_debug_none,         aap);
  auto ba = app.add_flag("--vt_debug_gen",          vt_debug_gen,          bap);
  auto ca = app.add_flag("--vt_debug_runtime",      vt_debug_runtime,      cap);
  auto da = app.add_flag("--vt_debug_active",       vt_debug_active,       dap);
  auto ea = app.add_flag("--vt_debug_term",         vt_debug_term,         eap);
  auto fa = app.add_flag("--vt_debug_termds",       vt_debug_termds,       fap);
  auto ga = app.add_flag("--vt_debug_barrier",      vt_debug_barrier,      gap);
  auto ha = app.add_flag("--vt_debug_event",        vt_debug_event,        hap);
  auto ia = app.add_flag("--vt_debug_pipe",         vt_debug_pipe,         iap);
  auto ja = app.add_flag("--vt_debug_pool",         vt_debug_pool,         jap);
  auto ka = app.add_flag("--vt_debug_reduce",       vt_debug_reduce,       kap);
  auto la = app.add_flag("--vt_debug_rdma",         vt_debug_rdma,         lap);
  auto ma = app.add_flag("--vt_debug_rdma_channel", vt_debug_rdma_channel, map);
  auto na = app.add_flag("--vt_debug_rdma_state",   vt_debug_rdma_state,   nap);
  auto oa = app.add_flag("--vt_debug_param",        vt_debug_param,        oap);
  auto pa = app.add_flag("--vt_debug_handler",      vt_debug_handler,      pap);
  auto qa = app.add_flag("--vt_debug_hierlb",       vt_debug_hierlb,       qap);
  auto qb = app.add_flag("--vt_debug_gossiplb",     vt_debug_gossiplb,     qbp);
  auto ra = app.add_flag("--vt_debug_scatter",      vt_debug_scatter,      rap);
  auto sa = app.add_flag("--vt_debug_sequence",     vt_debug_sequence,     sap);
  auto ta = app.add_flag("--vt_debug_sequence_vrt", vt_debug_sequence_vrt, tap);
  auto ua = app.add_flag("--vt_debug_serial_msg",   vt_debug_serial_msg,   uap);
  auto va = app.add_flag("--vt_debug_trace",        vt_debug_trace,        vap);
  auto wa = app.add_flag("--vt_debug_location",     vt_debug_location,     wap);
  auto xa = app.add_flag("--vt_debug_lb",           vt_debug_lb,           xap);
  auto ya = app.add_flag("--vt_debug_vrt",          vt_debug_vrt,          yap);
  auto za = app.add_flag("--vt_debug_vrt_coll",     vt_debug_vrt_coll,     zap);
  auto ab = app.add_flag("--vt_debug_worker",       vt_debug_worker,       abp);
  auto bb = app.add_flag("--vt_debug_group",        vt_debug_group,        bbp);
  auto cb = app.add_flag("--vt_debug_broadcast",    vt_debug_broadcast,    cbp);
  auto db = app.add_flag("--vt_debug_objgroup",     vt_debug_objgroup,     dbp);
  auto debugGroup = "Debug Print Configuration (must be compile-time enabled)";
  r->group(debugGroup);
  r1->group(debugGroup);
  aa->group(debugGroup);
  ba->group(debugGroup);
  ca->group(debugGroup);
  da->group(debugGroup);
  ea->group(debugGroup);
  fa->group(debugGroup);
  ga->group(debugGroup);
  ha->group(debugGroup);
  ia->group(debugGroup);
  ja->group(debugGroup);
  ka->group(debugGroup);
  la->group(debugGroup);
  ma->group(debugGroup);
  na->group(debugGroup);
  oa->group(debugGroup);
  pa->group(debugGroup);
  qa->group(debugGroup);
  qb->group(debugGroup);
  ra->group(debugGroup);
  sa->group(debugGroup);
  ta->group(debugGroup);
  ua->group(debugGroup);
  va->group(debugGroup);
  xa->group(debugGroup);
  wa->group(debugGroup);
  ya->group(debugGroup);
  za->group(debugGroup);
  ab->group(debugGroup);
  bb->group(debugGroup);
  cb->group(debugGroup);
  db->group(debugGroup);

  /*
   * Flags for enabling load balancing and configuring it
   */

  auto lb            = "Enable load balancing";
  auto lb_file       = "Enable reading LB configuration from file";
  auto lb_args       = "Arguments pass to LB: \"x=0 y=1 test=2\"";
  auto lb_quiet      = "Silence load balancing output";
  auto lb_file_name  = "LB configuration file to read";
  auto lb_name       = "Name of the load balancer to use";
  auto lb_interval   = "Load balancing interval";
  auto lb_stats      = "Enable load balancing statistics";
  auto lb_stats_dir  = "Load balancing statistics output directory";
  auto lb_stats_file = "Load balancing statistics output file name";
  auto lb_stats_dir_in  = "Load balancing statistics input directory";
  auto lb_stats_file_in = "Load balancing statistics input file name";
  auto lbn = "NoLB";
  auto lbi = 1;
  auto lbf = "";
  auto lbd = "vt_lb_stats";
  auto lbs = "stats";
  auto lba = "";
  auto s  = app.add_flag("--vt_lb",              vt_lb,             lb);
  auto t  = app.add_flag("--vt_lb_file",         vt_lb_file,        lb_file);
  auto t1 = app.add_flag("--vt_lb_quiet",        vt_lb_quiet,       lb_quiet);
  auto u  = app.add_option("--vt_lb_file_name",  vt_lb_file_name,   lb_file_name, lbf);
  auto v  = app.add_option("--vt_lb_name",       vt_lb_name,        lb_name,      lbn);
  auto v1 = app.add_option("--vt_lb_args",       vt_lb_args,        lb_args,      lba);
  auto w  = app.add_option("--vt_lb_interval",   vt_lb_interval,    lb_interval,  lbi);
  auto ww = app.add_flag("--vt_lb_stats",        vt_lb_stats,       lb_stats);
  auto wx = app.add_option("--vt_lb_stats_dir",  vt_lb_stats_dir,   lb_stats_dir, lbd);
  auto wy = app.add_option("--vt_lb_stats_file", vt_lb_stats_file,  lb_stats_file,lbs);

  auto xx = app.add_option("--vt_lb_stats_dir_in", vt_lb_stats_dir_in, lb_stats_dir_in, lbd);
  auto xy = app.add_option("--vt_lb_stats_file_in", vt_lb_stats_file_in, lb_stats_file_in,lbs);

  auto debugLB = "Load Balancing";
  s->group(debugLB);
  t->group(debugLB);
  t1->group(debugLB);
  u->group(debugLB);
  v->group(debugLB);
  v1->group(debugLB);
  w->group(debugLB);
  ww->group(debugLB);
  wx->group(debugLB);
  wy->group(debugLB);
  xx->group(debugLB);
  xy->group(debugLB);

  /*
   * Flags for controlling termination
   */

  auto hang         = "Disable termination hang detection";
  auto hang_freq    = "The number of tree traversals before a hang is detected";
  auto ds           = "Force use of Dijkstra-Scholten (DS) algorithm for rooted epoch termination detection";
  auto wave         = "Force use of 4-counter algorithm for rooted epoch termination detection";
  auto graph_on     = "Output epoch graph to file (DOT) when hang is detected";
  auto terse        = "Output epoch graph to file in terse mode";
  auto progress     = "Print termination counts when progress is stalled";
  auto hfd          = 1024;
  auto x  = app.add_flag("--vt_no_detect_hang",        vt_no_detect_hang,       hang);
  auto x1 = app.add_flag("--vt_term_rooted_use_ds",    vt_term_rooted_use_ds,   ds);
  auto x2 = app.add_flag("--vt_term_rooted_use_wave",  vt_term_rooted_use_wave, wave);
  auto x3 = app.add_option("--vt_epoch_graph_on_hang", vt_epoch_graph_on_hang,  graph_on, true);
  auto x4 = app.add_flag("--vt_epoch_graph_terse",     vt_epoch_graph_terse,    terse);
  auto x5 = app.add_option("--vt_print_no_progress",   vt_print_no_progress,    progress, true);
  auto y = app.add_option("--vt_hang_freq",            vt_hang_freq,      hang_freq, hfd);
  auto debugTerm = "Termination";
  x->group(debugTerm);
  x1->group(debugTerm);
  x2->group(debugTerm);
  x3->group(debugTerm);
  x4->group(debugTerm);
  x5->group(debugTerm);
  y->group(debugTerm);

  /*
   * Flags for controlling termination
   */

  auto pause        = "Pause at startup so GDB/LLDB can be attached";
  auto z = app.add_flag("--vt_pause", vt_pause, pause);
  auto launchTerm = "Debugging/Launch";
  z->group(launchTerm);

  /*
   * User option flags for convenience; VT will parse these and the app can use
   * them however the apps requires
   */

  auto user1    = "User Option 1a (boolean)";
  auto user2    = "User Option 2a (boolean)";
  auto user3    = "User Option 3a (boolean)";
  auto userint1 = "User Option 1b (int32_t)";
  auto userint2 = "User Option 2b (int32_t)";
  auto userint3 = "User Option 3b (int32_t)";
  auto userstr1 = "User Option 1c (std::string)";
  auto userstr2 = "User Option 2c (std::string)";
  auto userstr3 = "User Option 3c (std::string)";
  auto u1  = app.add_flag("--vt_user_1", vt_user_1, user1);
  auto u2  = app.add_flag("--vt_user_2", vt_user_2, user2);
  auto u3  = app.add_flag("--vt_user_3", vt_user_3, user3);
  auto ui1 = app.add_option("--vt_user_int_1", vt_user_int_1, userint1, 0);
  auto ui2 = app.add_option("--vt_user_int_2", vt_user_int_2, userint2, 0);
  auto ui3 = app.add_option("--vt_user_int_3", vt_user_int_3, userint3, 0);
  auto us1 = app.add_option("--vt_user_str_1", vt_user_str_1, userstr1, "");
  auto us2 = app.add_option("--vt_user_str_2", vt_user_str_2, userstr2, "");
  auto us3 = app.add_option("--vt_user_str_3", vt_user_str_3, userstr3, "");
  auto userOpts = "User Options";
  u1->group(userOpts);
  u2->group(userOpts);
  u3->group(userOpts);
  ui1->group(userOpts);
  ui2->group(userOpts);
  ui3->group(userOpts);
  us1->group(userOpts);
  us2->group(userOpts);
  us3->group(userOpts);

  /*
   * Options for configuring the VT scheduler
   */

  auto nsched = "Number of times to run the progress function in scheduler";
  auto ksched = "Run the MPI progress function at least every k handlers that run";
  auto ssched = "Run the MPI progress function at least every s seconds";
  auto sca = app.add_option("--vt_sched_num_progress", vt_sched_num_progress, nsched, 2);
  auto hca = app.add_option("--vt_sched_progress_han", vt_sched_progress_han, ksched, 0);
  auto kca = app.add_option("--vt_sched_progress_sec", vt_sched_progress_sec, ssched, 0.0);
  auto schedulerGroup = "Scheduler Configuration";
  sca->group(schedulerGroup);
  hca->group(schedulerGroup);
  kca->group(schedulerGroup);

  /*
   * Run the parser!
   */
  app.allow_extras(true);
  try {
    app.parse(args);
  } catch (CLI::Error &ex) {
    return app.exit(ex);
  }

  // Determine the final colorization setting.
  if (vt_no_color) {
    colorize_output = false;
  } else {
    // Otherwise, colorize.
    // (Within MPI there is no good method to auto-detect.)
    colorize_output = true;
  }

  /*
   * Put the arguments back into argc, argv, but properly order them based on
   * the input order by comparing between the current args
   */
  std::vector<std::string> ret_args;
  std::vector<std::size_t> ret_idx;
  int item = 0;

  // Iterate forward (CLI11 reverses the order when it modifies the args)
  for (auto&& skipped : args) {
    for (auto ii = item; ii < argc; ii++) {
      if (std::string(argv[ii]) == skipped) {
        ret_idx.push_back(ii);
        item++;
        break;
      }
    }
    ret_args.push_back(skipped);
  }

  // Use the saved index to setup the new_argv and new_argc
  int new_argc = ret_args.size() + 1;
  char** new_argv = new char*[new_argc + 1];
  new_argv[0] = argv[0];
  for (auto ii = 1; ii < new_argc; ii++) {
    new_argv[ii] = argv[ret_idx[ii - 1]];
  }

  // Set them back with all vt arguments elided
  argc = new_argc;
  argv = new_argv;

  parsed = true;
  return 1;
}

}} /* end namespace vt::arguments */
