/*
//@HEADER
// ************************************************************************
//
//                          args.cc
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

#include "vt/config.h"
#include "vt/configs/arguments/args.h"

#include <string>
#include <vector>

#include "CLI/CLI11.hpp"

namespace vt { namespace arguments {


/*static*/ CLI::App    Args::app{"vt"};
/*static*/ Configs Args::config = {};
/*static*/ bool Args::parsed = false;


/*static*/ int Args::parse(
  int& argc, char**& argv, 
  const Configs *ref
) 
{

  if (parsed)
    return 0;

  if (argc == 0 || argv == nullptr) {
	if (ref == nullptr)
	  return 0;
	//
    config = *ref;
	return 1;
  }

  std::vector<std::string> args;
  for (auto i = 0; i < argc; i++) {
    args.push_back(std::string(argv[i]));
  }

  // fmt::print("argc={}, argv={}\n", argc, print_ptr(argv));

  //
  // Default options to pass to all subcommands
  // https://cliutils.gitlab.io/CLI11Tutorial/chapters/options.html
  // CLI allows to handle multiple occurences of one argument
  //
  //app.option_defaults()->multi_option_policy(CLI::MultiOptionPolicy::TakeFirst);

  /*
   * Flags for controlling the colorization of output from vt
   */
  auto quiet  = "Quiet the output from vt (only errors, warnings)";
  auto always = "Always colorize output";
  auto never  = "Never colorize output";
  auto maybe  = "Use isatty to determine colorization of output";
  auto a  = app.add_flag("-c,--vt_color",      config.vt_color,      always);
  auto b  = app.add_flag("-n,--vt_no_color",   config.vt_no_color,   never);
  auto c  = app.add_flag("-a,--vt_auto_color", config.vt_auto_color, maybe);
  auto a1 = app.add_flag("-q,--vt_quiet",      config.vt_quiet,      quiet);
  auto outputGroup = "Output Control";
  a->group(outputGroup);
  b->group(outputGroup);
  c->group(outputGroup);
  a1->group(outputGroup);
  b->excludes(a);
  b->excludes(c);

  /*
   * Flags for controlling the signals that VT tries to catch
   */
  auto no_sigint      = "Do not register signal handler for SIGINT";
  auto no_sigsegv     = "Do not register signal handler for SIGSEGV";
  auto no_terminate   = "Do not register handler for std::terminate";
  auto d = app.add_flag("--vt_no_SIGINT",    config.vt_no_sigint,    no_sigint);
  auto e = app.add_flag("--vt_no_SIGSEGV",   config.vt_no_sigsegv,   no_sigsegv);
  auto f = app.add_flag("--vt_no_terminate", config.vt_no_terminate, no_terminate);
  auto signalGroup = "Signa Handling";
  d->group(signalGroup);
  e->group(signalGroup);
  f->group(signalGroup);


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
  auto g = app.add_flag("--vt_no_warn_stack",   config.vt_no_warn_stack,   warn);
  auto h = app.add_flag("--vt_no_assert_stack", config.vt_no_assert_stack, assert);
  auto i = app.add_flag("--vt_no_abort_stack",  config.vt_no_abort_stack,  abort);
  auto j = app.add_flag("--vt_no_stack",        config.vt_no_stack,        stack);
  auto k = app.add_option("--vt_stack_file",    config.vt_stack_file,      file);
  auto l = app.add_option("--vt_stack_dir",     config.vt_stack_dir,       dir);
  auto m = app.add_option("--vt_stack_mod",     config.vt_stack_mod,       mod);
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
  auto trace  = "Enable tracing (must be compiled with trace_enabled)";
  auto tfile  = "Name of trace files";
  auto tdir   = "Name of directory for trace files";
  auto tmod   = "Output trace file if (node % vt_stack_mod) == 0";
  auto n = app.add_flag("--vt_trace",        config.vt_trace,       trace);
  auto o = app.add_option("--vt_trace_file", config.vt_trace_file,  tfile, "");
  auto p = app.add_option("--vt_trace_dir",  config.vt_trace_dir,   tdir,  "");
  auto q = app.add_option("--vt_trace_mod",  config.vt_trace_mod,   tmod,  1);
  auto traceGroup = "Tracing Configuration";
  n->group(traceGroup);
  o->group(traceGroup);
  p->group(traceGroup);
  q->group(traceGroup);


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

  auto r  = app.add_flag("--vt_debug_all",          config.vt_debug_all,          rp);
  auto r1 = app.add_flag("--vt_debug_verbose",      config.vt_debug_verbose,      rq);
  auto aa = app.add_flag("--vt_debug_none",         config.vt_debug_none,         aap);
  auto ba = app.add_flag("--vt_debug_gen",          config.vt_debug_gen,          bap);
  auto ca = app.add_flag("--vt_debug_runtime",      config.vt_debug_runtime,      cap);
  auto da = app.add_flag("--vt_debug_active",       config.vt_debug_active,       dap);
  auto ea = app.add_flag("--vt_debug_term",         config.vt_debug_term,         eap);
  auto fa = app.add_flag("--vt_debug_termds",       config.vt_debug_termds,       fap);
  auto ga = app.add_flag("--vt_debug_barrier",      config.vt_debug_barrier,      gap);
  auto ha = app.add_flag("--vt_debug_event",        config.vt_debug_event,        hap);
  auto ia = app.add_flag("--vt_debug_pipe",         config.vt_debug_pipe,         iap);
  auto ja = app.add_flag("--vt_debug_pool",         config.vt_debug_pool,         jap);
  auto ka = app.add_flag("--vt_debug_reduce",       config.vt_debug_reduce,       kap);
  auto la = app.add_flag("--vt_debug_rdma",         config.vt_debug_rdma,         lap);
  auto ma = app.add_flag("--vt_debug_rdma_channel", config.vt_debug_rdma_channel, map);
  auto na = app.add_flag("--vt_debug_rdma_state",   config.vt_debug_rdma_state,   nap);
  auto oa = app.add_flag("--vt_debug_param",        config.vt_debug_param,        oap);
  auto pa = app.add_flag("--vt_debug_handler",      config.vt_debug_handler,      pap);
  auto qa = app.add_flag("--vt_debug_hierlb",       config.vt_debug_hierlb,       qap);
  auto ra = app.add_flag("--vt_debug_scatter",      config.vt_debug_scatter,      rap);
  auto sa = app.add_flag("--vt_debug_sequence",     config.vt_debug_sequence,     sap);
  auto ta = app.add_flag("--vt_debug_sequence_vrt", config.vt_debug_sequence_vrt, tap);
  auto ua = app.add_flag("--vt_debug_serial_msg",   config.vt_debug_serial_msg,   uap);
  auto va = app.add_flag("--vt_debug_trace",        config.vt_debug_trace,        vap);
  auto wa = app.add_flag("--vt_debug_location",     config.vt_debug_location,     wap);
  auto xa = app.add_flag("--vt_debug_lb",           config.vt_debug_lb,           xap);
  auto ya = app.add_flag("--vt_debug_vrt",          config.vt_debug_vrt,          yap);
  auto za = app.add_flag("--vt_debug_vrt_coll",     config.vt_debug_vrt_coll,     zap);
  auto ab = app.add_flag("--vt_debug_worker",       config.vt_debug_worker,       abp);
  auto bb = app.add_flag("--vt_debug_group",        config.vt_debug_group,        bbp);
  auto cb = app.add_flag("--vt_debug_broadcast",    config.vt_debug_broadcast,    cbp);
  auto db = app.add_flag("--vt_debug_objgroup",     config.vt_debug_objgroup,     dbp);
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
  auto lb_quiet      = "Silence load balancing output";
  auto lb_file_name  = "LB configuration file to read";
  auto lb_name       = "Name of the load balancer to use";
  auto lb_interval   = "Load balancing interval";
  auto lb_stats      = "Enable load balancing statistics";
  auto lb_stats_dir  = "Load balancing statistics output directory";
  auto lb_stats_file = "Load balancing statistics output file name";
  auto lbn = "NoLB";
  auto lbi = 1;
  auto lbf = "balance.in";
  auto lbd = "vt_lb_stats";
  auto lbs = "stats";
  auto s  = app.add_flag("--vt_lb",              config.vt_lb,            lb);
  auto t  = app.add_flag("--vt_lb_file",         config.vt_lb_file,       lb_file);
  auto t1 = app.add_flag("--vt_lb_quiet",        config.vt_lb_quiet,      lb_quiet);
  auto u  = app.add_option("--vt_lb_file_name",  config.vt_lb_file_name,  lb_file_name, 
		  lbf);
  auto v  = app.add_option("--vt_lb_name",       config.vt_lb_name,       lb_name,      
		  lbn);
  auto w  = app.add_option("--vt_lb_interval",   config.vt_lb_interval,   lb_interval,  
		  lbi);
  auto ww = app.add_flag("--vt_lb_stats",        config.vt_lb_stats,      lb_stats);
  auto wx = app.add_option("--vt_lb_stats_dir",  config.vt_lb_stats_dir,  lb_stats_dir, 
		  lbd);
  auto wy = app.add_option("--vt_lb_stats_file", config.vt_lb_stats_file, lb_stats_file,
		  lbs);
  auto debugLB = "Load Balancing";
  s->group(debugLB);
  t->group(debugLB);
  t1->group(debugLB);
  u->group(debugLB);
  v->group(debugLB);
  w->group(debugLB);
  ww->group(debugLB);
  wx->group(debugLB);
  wy->group(debugLB);

  /*
   * Flags for controlling termination
   */

  auto hang         = "Disable termination hang detection";
  auto hang_freq    = "The number of tree traversals before a hang is detected";
  auto ds           = "Force use of Dijkstra-Scholten (DS) algorithm for rooted epoch termination detection";
  auto wave         = "Force use of 4-counter algorithm for rooted epoch termination detection";
  auto hfd          = 1024;
  auto x  = app.add_flag("--vt_no_detect_hang",       config.vt_no_detect_hang,       hang);
  auto x1 = app.add_flag("--vt_term_rooted_use_ds",   config.vt_term_rooted_use_ds,   ds);
  auto x2 = app.add_flag("--vt_term_rooted_use_wave", config.vt_term_rooted_use_wave, wave);
  auto y = app.add_option("--vt_hang_freq",           config.vt_hang_freq, hang_freq, hfd);
  auto debugTerm = "Termination";
  x->group(debugTerm);
  x1->group(debugTerm);
  x2->group(debugTerm);
  y->group(debugTerm);

  /*
   * Flags for controlling termination
   */

  auto pause        = "Pause at startup so GDB/LLDB can be attached";
  auto z = app.add_flag("--vt_pause", config.vt_pause, pause);
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
  auto u1  = app.add_flag("--vt_user_1", config.vt_user_1, user1);
  auto u2  = app.add_flag("--vt_user_2", config.vt_user_2, user2);
  auto u3  = app.add_flag("--vt_user_3", config.vt_user_3, user3);
  auto ui1 = app.add_option("--vt_user_int_1", config.vt_user_int_1, userint1);
  auto ui2 = app.add_option("--vt_user_int_2", config.vt_user_int_2, userint2);
  auto ui3 = app.add_option("--vt_user_int_3", config.vt_user_int_3, userint3);
  auto us1 = app.add_option("--vt_user_str_1", config.vt_user_str_1, userstr1);
  auto us2 = app.add_option("--vt_user_str_2", config.vt_user_str_2, userstr2);
  auto us3 = app.add_option("--vt_user_str_3", config.vt_user_str_3, userstr3);
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
   * Run the parser!
   */
  app.allow_extras(true);
  try {
    app.parse(args);
  } catch (CLI::Error &ex) {
    return app.exit(ex);
  }

  /*
   * Put the arguments back into argc, argv, but properly order them based on
   * the input order by comparing between the current args
   */
  std::vector<std::string> ret_args;
  std::vector<std::size_t> ret_idx;

  // Reverse iterate (CLI11 reverses the order when they modify the args)
  for (auto iter = args.rbegin(); iter != args.rend(); ++iter) {
    for (auto ii = 0; ii < argc; ii++) {
      if (std::string(argv[ii]) == *iter) {
        ret_idx.push_back(ii);
        break;
      }
    }
    ret_args.push_back(*iter);
  }

  // Use the saved index to setup the new_argv and new_argc
  int new_argc = ret_args.size();
  char** new_argv = new char*[new_argc];
  for (auto ii = 0; ii < new_argc; ii++) {
    new_argv[ii] = argv[ret_idx[ii]];
  }

  // Set them back with all vt arguments elided
  argc = new_argc;
  argv = new_argv;

  parsed = true;

  if (ref == nullptr)
    return 1;

  //
  // Treat the configuration from the files
  //

  if (a->count() == 0)
    config.vt_color = ref->vt_color;

  if (b->count() == 0)
    config.vt_no_color = ref->vt_no_color;

  if (c->count() == 0)
    config.vt_auto_color = ref->vt_auto_color;

  if (a1->count() == 0)
    config.vt_quiet = ref->vt_quiet;

  //------

  if (d->count() == 0)
    config.vt_no_sigint = ref->vt_no_sigint;

  if (e->count() == 0)
    config.vt_no_sigsegv = ref->vt_no_sigsegv;

  if (f->count() == 0)
    config.vt_no_terminate = ref->vt_no_terminate;

  //------

  if (g->count() == 0)
    config.vt_no_warn_stack = ref->vt_no_warn_stack;

  if (h->count() == 0)
    config.vt_no_assert_stack = ref->vt_no_assert_stack;

  if (i->count() == 0)
    config.vt_no_abort_stack = ref->vt_no_abort_stack;

  if (j->count() == 0)
    config.vt_no_stack = ref->vt_no_stack;

  if (k->count() == 0)
    config.vt_stack_file = ref->vt_stack_file;

  if (l->count() == 0)
    config.vt_stack_dir = ref->vt_stack_dir;

  if (m->count() == 0)
    config.vt_stack_mod = ref->vt_stack_mod;

  //------

  if (n->count() == 0)
    config.vt_trace = ref->vt_trace;

  if (o->count() == 0)
    config.vt_trace_file = ref->vt_trace_file;

  if (p->count() == 0)
    config.vt_trace_dir = ref->vt_trace_dir;

  if (q->count() == 0)
    config.vt_trace_mod = ref->vt_trace_mod;

  //------

  if (r->count() == 0)
    config.vt_debug_all = ref->vt_debug_all;

  if (r1->count() == 0)
    config.vt_debug_verbose = ref->vt_debug_verbose;

  if (aa->count() == 0)
    config.vt_debug_none = ref->vt_debug_none;

  if (ba->count() == 0)
    config.vt_debug_gen = ref->vt_debug_gen;

  if (ca->count() == 0)
    config.vt_debug_runtime = ref->vt_debug_runtime;

  if (da->count() == 0)
    config.vt_debug_active = ref->vt_debug_active;

  if (ea->count() == 0)
    config.vt_debug_term = ref->vt_debug_term;

  if (fa->count() == 0)
    config.vt_debug_termds = ref->vt_debug_termds;

  if (ga->count() == 0)
    config.vt_debug_barrier = ref->vt_debug_barrier;

  if (ha->count() == 0)
    config.vt_debug_event = ref->vt_debug_event;

  if (ia->count() == 0)
    config.vt_debug_pipe = ref->vt_debug_pipe;

  if (ja->count() == 0)
    config.vt_debug_pool = ref->vt_debug_pool;

  if (ka->count() == 0)
    config.vt_debug_reduce = ref->vt_debug_reduce;

  if (la->count() == 0)
    config.vt_debug_rdma = ref->vt_debug_rdma;

  if (ma->count() == 0)
    config.vt_debug_rdma_channel = ref->vt_debug_rdma_channel;

  if (na->count() == 0)
    config.vt_debug_rdma_state = ref->vt_debug_rdma_state;

  if (oa->count() == 0)
    config.vt_debug_param = ref->vt_debug_param;

  if (pa->count() == 0)
    config.vt_debug_handler = ref->vt_debug_handler;

  if (qa->count() == 0)
    config.vt_debug_hierlb = ref->vt_debug_hierlb;

  if (ra->count() == 0)
    config.vt_debug_scatter = ref->vt_debug_scatter;

  if (sa->count() == 0)
    config.vt_debug_sequence = ref->vt_debug_sequence;

  if (ta->count() == 0)
    config.vt_debug_sequence_vrt = ref->vt_debug_sequence_vrt;

  if (ua->count() == 0)
    config.vt_debug_serial_msg = ref->vt_debug_serial_msg;

  if (va->count() == 0)
    config.vt_debug_trace = ref->vt_debug_trace;

  if (wa->count() == 0)
    config.vt_debug_location = ref->vt_debug_location;

  if (xa->count() == 0)
    config.vt_debug_lb = ref->vt_debug_lb;

  if (ya->count() == 0)
    config.vt_debug_vrt = ref->vt_debug_vrt;

  if (za->count() == 0)
    config.vt_debug_vrt_coll = ref->vt_debug_vrt_coll;

  if (ab->count() == 0)
    config.vt_debug_worker = ref->vt_debug_worker;

  if (bb->count() == 0)
    config.vt_debug_group = ref->vt_debug_group;

  if (cb->count() == 0)
    config.vt_debug_broadcast = ref->vt_debug_broadcast;

  if (db->count() == 0)
    config.vt_debug_objgroup = ref->vt_debug_objgroup;

  //------

  if (s->count() == 0)
    config.vt_lb = ref->vt_lb;

  if (t->count() == 0)
    config.vt_lb_file = ref->vt_lb_file;

  if (t1->count() == 0)
    config.vt_lb_quiet = ref->vt_lb_quiet;

  if (u->count() == 0)
    config.vt_lb_file_name = ref->vt_lb_file_name;

  if (v->count() == 0)
    config.vt_lb_name = ref->vt_lb_name;

  if (w->count() == 0)
    config.vt_lb_interval = ref->vt_lb_interval;

  if (ww->count() == 0)
    config.vt_lb_stats = ref->vt_lb_stats;

  if (wx->count() == 0)
    config.vt_lb_stats_dir = ref->vt_lb_stats_dir;

  if (wy->count() == 0)
    config.vt_lb_stats_file = ref->vt_lb_stats_file;

  //------

  if (u1->count() == 0)
    config.vt_user_1 = ref->vt_user_1;

  if (u2->count() == 0)
    config.vt_user_2 = ref->vt_user_2;

  if (u3->count() == 0)
    config.vt_user_3 = ref->vt_user_3;

  if (ui1->count() == 0)
    config.vt_user_int_1 = ref->vt_user_int_1;

  if (ui2->count() == 0)
    config.vt_user_int_2 = ref->vt_user_int_2;

  if (ui3->count() == 0)
    config.vt_user_int_3 = ref->vt_user_int_3;

  if (us1->count() == 0)
    config.vt_user_str_1 = ref->vt_user_str_1;

  if (us2->count() == 0)
    config.vt_user_str_2 = ref->vt_user_str_2;

  if (us3->count() == 0)
    config.vt_user_str_3 = ref->vt_user_str_3;

  //------

  return 2;

}

}} /* end namespace vt::arguments */
