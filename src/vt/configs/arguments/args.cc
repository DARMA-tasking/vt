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
#include "vt/configs/arguments/args_utils.h"

#include <string>
#include <utility>
#include <vector>

#include "CLI/CLI11.hpp"
#include "fmt/format.h"

namespace vt { namespace arguments {

//--- Initialization of static variables for vt::AnchorBase
/* static */ std::map<ContextEnum, AnchorBase::OrderContextEnum>
  AnchorBase::emap_ = {
  {std::make_pair(ContextEnum::dFault, AnchorBase::OrderContextEnum::dFault),
    std::make_pair(
      ContextEnum::commandLine, AnchorBase::OrderContextEnum::commandLine),
    std::make_pair(
      ContextEnum::thirdParty, AnchorBase::OrderContextEnum::thirdParty)}};

/* static */ std::map<ContextEnum, std::string> AnchorBase::smap_ = {
  {std::make_pair(ContextEnum::dFault, "Default Value"),
    std::make_pair(ContextEnum::commandLine, "Command Line"),
    std::make_pair(ContextEnum::thirdParty, "Third-Party Context")}};


//--- Initialization of static variables for vt::Args
/*static*/ Configs Args::config = {};
/*static*/ bool Args::parsed = false;

/*static*/
int Args::parse(int& argc, char**& argv) {

  if (parsed || argc == 0 || argv == nullptr)
    return 0;

  // CLI11 app parser expects to get the arguments in *reverse* order!
  std::vector<std::string> args;
  for (int i = argc - 1; i > 0; i--) {
    args.emplace_back(argv[i]);
  }

  // fmt::print("argc={}, argv={}\n", argc, print_ptr(argv));

  /* Setup the parser */
  std::unique_ptr<CLI::App> app_ = std::make_unique<CLI::App>("vt");
  setup(app_.get());

  /* Run the parser */
  app_->allow_extras(true);
  try {
    app_->parse(args);
  } catch (CLI::Error &ex) {
    return app_->exit(ex);
  }

  // Determine the final colorization setting.
  if (config.vt_no_color) {
    config.colorize_output = false;
  } else {
    // Otherwise, colorize.
    // (Within MPI there is no good method to auto-detect.)
    config.colorize_output = true;
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
    for (int ii = item; ii < argc; ii++) {
      if (std::string(argv[ii]) == skipped) {
        ret_idx.push_back(ii);
        item++;
        break;
      }
    }
    ret_args.push_back(skipped);
  }

  // Use the saved index to setup the new_argv and new_argc
  int new_argc = static_cast<int>(ret_args.size()) + 1;
  char** new_argv = new char*[new_argc + 1];
  new_argv[0] = argv[0];
  for (int ii = 1; ii < new_argc; ii++) {
    new_argv[ii] = argv[ret_idx[ii - 1]];
  }

  // Set them back with all vt arguments elided
  argc = new_argc;
  argv = new_argv;

  parsed = true;
  return 0;
}

/*static*/
void Args::setup(CLI::App *app_) {
  /*
   * Flags for controlling the colorization of output from vt
   */
  auto quiet  = "Quiet the output from vt (only errors, warnings)";
  auto always = "Colorize output (default)";
  auto never  = "Do not colorize output (overrides --vt_color)";
  auto a  = app_->add_flag("-c,--vt_color",      config.vt_color,      always);
  auto b  = app_->add_flag("-n,--vt_no_color",   config.vt_no_color,   never);
  auto a1 = app_->add_flag("-q,--vt_quiet",      config.vt_quiet,      quiet);
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
  auto d = app_->add_flag("--vt_no_SIGINT",    config.vt_no_sigint,    no_sigint);
  auto e = app_->add_flag("--vt_no_SIGSEGV",   config.vt_no_sigsegv,   no_sigsegv);
  auto f = app_->add_flag("--vt_no_terminate", config.vt_no_terminate, no_terminate);
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
  auto g = app_->add_flag("--vt_no_warn_stack",   config.vt_no_warn_stack,   warn);
  auto h = app_->add_flag("--vt_no_assert_stack", config.vt_no_assert_stack, assert);
  auto i = app_->add_flag("--vt_no_abort_stack",  config.vt_no_abort_stack,  abort);
  auto j = app_->add_flag("--vt_no_stack",        config.vt_no_stack,        stack);
  auto k = app_->add_option("--vt_stack_file",    config.vt_stack_file,      file, "");
  auto l = app_->add_option("--vt_stack_dir",     config.vt_stack_dir,       dir,  "");
  auto m = app_->add_option("--vt_stack_mod",     config.vt_stack_mod,       mod,  1);
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
  auto n  = app_->add_flag("--vt_trace",              config.vt_trace,           trace);
  auto nm = app_->add_flag("--vt_trace_mpi",          config.vt_trace_mpi,       trace_mpi);
  auto o  = app_->add_option("--vt_trace_file",       config.vt_trace_file,      tfile, "");
  auto p  = app_->add_option("--vt_trace_dir",        config.vt_trace_dir,       tdir,  "");
  auto q  = app_->add_option("--vt_trace_mod",        config.vt_trace_mod,       tmod,  1);
  auto qf = app_->add_option("--vt_trace_flush_size", config.vt_trace_flush_size,tflushmod,
                           0);
  auto traceGroup = "Tracing Configuration";
  n->group(traceGroup);
  nm->group(traceGroup);
  o->group(traceGroup);
  p->group(traceGroup);
  q->group(traceGroup);
  qf->group(traceGroup);


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

  auto r  = app_->add_flag("--vt_debug_all",          config.vt_debug_all,          rp);
  auto r1 = app_->add_flag("--vt_debug_verbose",      config.vt_debug_verbose,      rq);
  auto aa = app_->add_flag("--vt_debug_none",         config.vt_debug_none,         aap);
  auto ba = app_->add_flag("--vt_debug_gen",          config.vt_debug_gen,          bap);
  auto ca = app_->add_flag("--vt_debug_runtime",      config.vt_debug_runtime,      cap);
  auto da = app_->add_flag("--vt_debug_active",       config.vt_debug_active,       dap);
  auto ea = app_->add_flag("--vt_debug_term",         config.vt_debug_term,         eap);
  auto fa = app_->add_flag("--vt_debug_termds",       config.vt_debug_termds,       fap);
  auto ga = app_->add_flag("--vt_debug_barrier",      config.vt_debug_barrier,      gap);
  auto ha = app_->add_flag("--vt_debug_event",        config.vt_debug_event,        hap);
  auto ia = app_->add_flag("--vt_debug_pipe",         config.vt_debug_pipe,         iap);
  auto ja = app_->add_flag("--vt_debug_pool",         config.vt_debug_pool,         jap);
  auto ka = app_->add_flag("--vt_debug_reduce",       config.vt_debug_reduce,       kap);
  auto la = app_->add_flag("--vt_debug_rdma",         config.vt_debug_rdma,         lap);
  auto ma = app_->add_flag("--vt_debug_rdma_channel", config.vt_debug_rdma_channel, map);
  auto na = app_->add_flag("--vt_debug_rdma_state",   config.vt_debug_rdma_state,   nap);
  auto oa = app_->add_flag("--vt_debug_param",        config.vt_debug_param,        oap);
  auto pa = app_->add_flag("--vt_debug_handler",      config.vt_debug_handler,      pap);
  auto qa = app_->add_flag("--vt_debug_hierlb",       config.vt_debug_hierlb,       qap);
  auto qb = app_->add_flag("--vt_debug_gossiplb",     config.vt_debug_gossiplb,     qbp);
  auto ra = app_->add_flag("--vt_debug_scatter",      config.vt_debug_scatter,      rap);
  auto sa = app_->add_flag("--vt_debug_sequence",     config.vt_debug_sequence,     sap);
  auto ta = app_->add_flag("--vt_debug_sequence_vrt", config.vt_debug_sequence_vrt, tap);
  auto ua = app_->add_flag("--vt_debug_serial_msg",   config.vt_debug_serial_msg,   uap);
  auto va = app_->add_flag("--vt_debug_trace",        config.vt_debug_trace,        vap);
  auto wa = app_->add_flag("--vt_debug_location",     config.vt_debug_location,     wap);
  auto xa = app_->add_flag("--vt_debug_lb",           config.vt_debug_lb,           xap);
  auto ya = app_->add_flag("--vt_debug_vrt",          config.vt_debug_vrt,          yap);
  auto za = app_->add_flag("--vt_debug_vrt_coll",     config.vt_debug_vrt_coll,     zap);
  auto ab = app_->add_flag("--vt_debug_worker",       config.vt_debug_worker,       abp);
  auto bb = app_->add_flag("--vt_debug_group",        config.vt_debug_group,        bbp);
  auto cb = app_->add_flag("--vt_debug_broadcast",    config.vt_debug_broadcast,    cbp);
  auto db = app_->add_flag("--vt_debug_objgroup",     config.vt_debug_objgroup,     dbp);
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
  auto lbn = "NoLB";
  auto lbi = 1;
  auto lbf = "";
  auto lbd = "vt_lb_stats";
  auto lbs = "stats";
  auto lba = "";
  auto s  = app_->add_flag("--vt_lb",              config.vt_lb,             lb);
  auto t  = app_->add_flag("--vt_lb_file",         config.vt_lb_file,        lb_file);
  auto t1 = app_->add_flag("--vt_lb_quiet",        config.vt_lb_quiet,       lb_quiet);
  auto u  = app_->add_option("--vt_lb_file_name",  config.vt_lb_file_name,   lb_file_name, lbf);
  auto v  = app_->add_option("--vt_lb_name",       config.vt_lb_name,        lb_name,      lbn);
  auto v1 = app_->add_option("--vt_lb_args",       config.vt_lb_args,        lb_args,      lba);
  auto w  = app_->add_option("--vt_lb_interval",   config.vt_lb_interval,    lb_interval,  lbi);
  auto ww = app_->add_flag("--vt_lb_stats",        config.vt_lb_stats,       lb_stats);
  auto wx = app_->add_option("--vt_lb_stats_dir",  config.vt_lb_stats_dir,   lb_stats_dir, lbd);
  auto wy = app_->add_option("--vt_lb_stats_file", config.vt_lb_stats_file,  lb_stats_file,lbs);
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
  auto x  = app_->add_flag("--vt_no_detect_hang",        config.vt_no_detect_hang,       hang);
  auto x1 = app_->add_flag("--vt_term_rooted_use_ds",    config.vt_term_rooted_use_ds,   ds);
  auto x2 = app_->add_flag("--vt_term_rooted_use_wave",  config.vt_term_rooted_use_wave, wave);
  auto x3 = app_->add_option("--vt_epoch_graph_on_hang", config.vt_epoch_graph_on_hang,  graph_on, true);
  auto x4 = app_->add_flag("--vt_epoch_graph_terse",     config.vt_epoch_graph_terse,    terse);
  auto x5 = app_->add_option("--vt_print_no_progress",   config.vt_print_no_progress,    progress, true);
  auto y = app_->add_option("--vt_hang_freq",            config.vt_hang_freq,      hang_freq, hfd);
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
  auto z = app_->add_flag("--vt_pause", config.vt_pause, pause);
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
  auto u1  = app_->add_flag("--vt_user_1", config.vt_user_1, user1);
  auto u2  = app_->add_flag("--vt_user_2", config.vt_user_2, user2);
  auto u3  = app_->add_flag("--vt_user_3", config.vt_user_3, user3);
  auto ui1 = app_->add_option("--vt_user_int_1", config.vt_user_int_1, userint1, 0);
  auto ui2 = app_->add_option("--vt_user_int_2", config.vt_user_int_2, userint2, 0);
  auto ui3 = app_->add_option("--vt_user_int_3", config.vt_user_int_3, userint3, 0);
  auto us1 = app_->add_option("--vt_user_str_1", config.vt_user_str_1, userstr1, "");
  auto us2 = app_->add_option("--vt_user_str_2", config.vt_user_str_2, userstr2, "");
  auto us3 = app_->add_option("--vt_user_str_3", config.vt_user_str_3, userstr3, "");
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
  auto sca = app_->add_option("--vt_sched_num_progress", config.vt_sched_num_progress, nsched, 2);
  auto hca = app_->add_option("--vt_sched_progress_han", config.vt_sched_progress_han, ksched, 0);
  auto kca = app_->add_option("--vt_sched_progress_sec", config.vt_sched_progress_sec, ssched, 0.0);
  auto schedulerGroup = "Scheduler Configuration";
  sca->group(schedulerGroup);
  hca->group(schedulerGroup);
  kca->group(schedulerGroup);
}


/* ------------------------------------------------- */
// --- Member functions for struct PrintOn
/* ------------------------------------------------- */


template <typename T>
PrintOn<T>::PrintOn(Anchor<T>* opt, std::string  msg_str)
  : option_(opt), msg_str_(std::move(msg_str)), condition_(nullptr) {}

template <typename T>
PrintOn<T>::PrintOn(
  Anchor<T>* opt, std::string  msg_str, std::function<bool()> fun)
  : option_(opt), msg_str_(std::move(msg_str)),
  condition_(std::move(fun)) {}

template <typename T>
void PrintOn<T>::output() {

  T setValue = option_->getValue();
  if (setValue == option_->getDefaultValue())
    return;

  if ((condition_) && (!condition_()))
    return;

  auto green = debug::green();
  auto red = debug::red();
  auto reset = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta = debug::magenta();
  auto vt_pre = debug::vtPre();

  std::string cli_name = std::string("--") + option_->getName();
  auto f8 = fmt::format(msg_str_, setValue);

  auto f9 = fmt::format(
    "{}Option:{} flag {}{}{} on: {}{}\n", green, reset, magenta, cli_name,
    reset, f8, reset);
  fmt::print("{}\t{}{}", vt_pre, f9, reset);
}


/* ------------------------------------------------- */
// --- Member functions for struct PrintOnOff
/* ------------------------------------------------- */


template <typename T>
PrintOnOff<T>::PrintOnOff(
  Anchor<T>* opt, std::string msg_on, std::string msg_off)
  : option_(opt), msg_on_(std::move(msg_on)), msg_off_(std::move(msg_off)),
  condition_(nullptr) {}


template <typename T>
PrintOnOff<T>::PrintOnOff(
  Anchor<T>* opt, std::string msg_on, std::string msg_off,
  std::function<bool()> fun)
  : option_(opt), msg_on_(std::move(msg_on)), msg_off_(std::move(msg_off)),
  condition_(std::move(fun)) {}


template <typename T>
void PrintOnOff<T>::output() {

  if ((condition_) && (!condition_()))
    return;

  auto green = debug::green();
  auto red = debug::red();
  auto reset = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta = debug::magenta();
  auto vt_pre = debug::vtPre();

  std::string cli_name = std::string("--") + option_->getName();
  T setValue = option_->getValue();
  const T defaultValue = option_->getDefaultValue();

  if (setValue != defaultValue) {
    if (!msg_on_.empty()) {
      auto f_on = fmt::format(msg_on_, setValue);
      auto f9 = fmt::format(
        "{}Option:{} flag {}{}{} on: {}{}\n", green, reset, magenta, cli_name,
        reset, f_on, reset);
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
    }
  } else {
    if (!msg_off_.empty()) {
      auto f_off = fmt::format(msg_off_, setValue);
      auto f12 = fmt::format(
        "{}Default:{} {}, use {}{}{} to activate{}\n", green, reset, f_off,
        magenta, cli_name, reset, reset);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }
}


/* ------------------------------------------------- */
// --- Member functions for struct Warning
/* ------------------------------------------------- */


Warning::Warning(Anchor<bool>* opt, std::string compile)
  : option_(opt), compile_(std::move(compile)), condition_(nullptr) {}

Warning::Warning(
  Anchor<bool>* opt, std::string compile, std::function<bool()> fun)
  : option_(opt), compile_(std::move(compile)),
  condition_(std::move(fun)) {}

void Warning::output() {

  if (!option_->getValue())
    return;

  if ((condition_) && (!condition_()))
    return;

  auto green = debug::green();
  auto red = debug::red();
  auto reset = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta = debug::magenta();
  auto vt_pre = debug::vtPre();

  std::string cli_name = std::string("--") + option_->getName();
  auto fcompile = fmt::format(compile_, option_->getValue());
  auto f9 = fmt::format(
    "{}Warning:{} {}{}{} has no effect: compile-time"
    " feature {}{}{} is disabled{}\n",
    red, reset, magenta, cli_name, reset, magenta, fcompile, reset, reset);
  fmt::print("{}\t{}{}", vt_pre, f9, reset);
}


/* ------------------------------------------------- */
// --- Member functions for struct AnchorBase
/* ------------------------------------------------- */


AnchorBase::AnchorBase(std::string name, std::string desc, std::string grp) :
  isResolved_(false), name_(std::move(name)), group_(std::move(grp)),
  description_(std::move(desc)), ordering_(), needsOptOff_(), needsOptOn_(),
  excludes_(), statusPrint_(PrintOption::never), screenPrint_()
{
}


/* ------------------------------------------------- */
// --- Member functions for struct Anchor<T>
/* ------------------------------------------------- */


template <typename T>
Anchor<T>::Anchor(T& var, const std::string& name, const std::string& desc,
  const std::string& grp
) : AnchorBase(name, desc, grp), value_(var), specifications_(),
    hasNewDefault_(false),
    resolvedContext_(ContextEnum::dFault), resolvedInstance_(),
    resolvedToDefault_(false)
{
  Instance<T> myCase(var, static_cast<AnchorBase*>(this));
  specifications_.insert(std::make_pair(ContextEnum::dFault, myCase));
  ordering_[ContextEnum::dFault] = OrderContextEnum::dFault;
}


template <typename T>
void Anchor<T>::addInstance(const T& value, bool highestPrecedence) {
  try {
    addGeneralInstance(ContextEnum::thirdParty, value);
  } catch (const std::exception& e) {
    throw;
  }
  if (highestPrecedence)
    setHighestPrecedence(ContextEnum::thirdParty);
}


template <typename T>
void Anchor<T>::setHighestPrecedence(const ContextEnum& origin) {
  // Reset the 'current' highest precedence
  for (auto& entry : ordering_) {
    if (entry.second == OrderContextEnum::MAX)
      entry.second = emap_[entry.first];
  }
  ordering_[origin] = OrderContextEnum::MAX;
}


template <typename T>
void Anchor<T>::setNewDefaultValue(const T& ref) {
  //--- Context dFault always has one instance
  //--- So the find is always successful.
  auto iter = specifications_.find(ContextEnum::dFault);
  (iter->second).setNewValue(ref);
  hasNewDefault_ = true;
}


template <typename T>
void Anchor<T>::setPrintOption(PrintOption po, std::function<bool()> fun) {
  //--- Test if the 'printer' has already been set
  //--- If so, release the previous value
  if ((po != statusPrint_) && (screenPrint_)) {
    auto* ptr = screenPrint_.release();
    delete ptr;
  }
  //
  statusPrint_ = po;
  //
  auto desc = getDescription();
  if (statusPrint_ == PrintOption::whenSet) {
    std::string msg = "'" + desc + "'";
    screenPrint_ = std::make_unique<PrintOn<T>>(this, msg, fun);
  }
  //
  setPrintOptionImpl(po, fun);
}


template <typename T>
void Anchor<T>::setPrintOptionImpl(PrintOption po, std::function<bool()> fun) {}


template <>
void Anchor<bool>::setPrintOptionImpl(
  PrintOption po, std::function<bool()> fun) {
  //--- Case for a boolean flag
  auto desc = getDescription();
  if (statusPrint_ == PrintOption::always) {
    std::string msg_off = "Inactive flag '" + desc + "'";
    std::string msg_on = desc;
    screenPrint_ = std::make_unique<PrintOnOff<bool>>(this, msg_on, msg_off, fun);
  } else {
    //--- We should not arrive here
    vtAssert(
      statusPrint_ == PrintOption::whenSet,
      "Unexpected option for boolean flag");
  }
}


template <>
void Anchor<std::string>::setPrintOptionImpl(
  PrintOption po, std::function<bool()> fun) {
  //--- Case for a string option
  auto desc = getDescription();
  if (statusPrint_ == PrintOption::always) {
    std::string msg_off = "Inactive option '" + desc + "'";
    std::string msg_on = "'" + desc + "' {}";
    screenPrint_ =
      std::make_unique<PrintOnOff<std::string>>(this, msg_on, msg_off, fun);
  } else {
    //--- We should not arrive here
    vtAssert(
      statusPrint_ == PrintOption::whenSet,
      "Unexpected option for string option");
  }
}


template <typename T>
std::string Anchor<T>::stringifyValue() const {
  std::string val;
  if (!isResolved_)
    return val;
  //
  val = getDisplayValue<T>(value_);
  //
  return val;
}


template <typename T>
void Anchor<T>::print() {
  if (!isResolved_) {
    std::string code = std::string(" Option ") + name_ +
                       std::string(" should be resolved before printing.");
    throw std::runtime_error(code);
  }
  //---
  auto green = debug::green();
  auto red = debug::red();
  auto reset = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta = debug::magenta();
  auto vt_pre = debug::vtPre();
  //---
  // Check whether the value should be skipped/overriden
  for (auto opt : needsOptOn_) {
    if ((count() > 1) && (opt->count() == 1)) {
      auto cli_name1 = std::string("--") + name_;
      auto cli_name2 = std::string("--") + opt->getName();
      auto f9 = fmt::format(
        "{}Option:{} {}{}{} skipped; it requires {}{}{} to be active\n", green,
        reset, magenta, cli_name1, reset, magenta, cli_name2, reset);
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
      resetToDefault();
    }
  }
  //---
  // Check whether the value should be skipped/overriden
  for (auto opt : needsOptOff_) {
    if ((count() > 1) && (opt->count() > 1)) {
      auto cli_name1 = std::string("--") + name_;
      auto cli_name2 = std::string("--") + opt->getName();
      auto f9 = fmt::format(
        "{}Option:{} {}{}{} skipped; it requires {}{}{} to be deactivated\n",
        green, reset, magenta, cli_name1, reset, magenta, cli_name2, reset);
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
      resetToDefault();
    }
  }
  //---
  if (screenPrint_.get())
    screenPrint_->output();
}


template <typename T>
std::string Anchor<T>::stringifyContext() const {
  std::string val;
  if (!isResolved_)
    return val;
  return smap_[resolvedContext_];
}


template <typename T>
const T Anchor<T>::getDefaultValue() const {
  T val;
  for (auto item : specifications_) {
    if (item.first == ContextEnum::dFault) {
      val = item.second.getValue();
      break;
    }
  }
  return val;
}


template <typename T>
std::string Anchor<T>::stringifyDefault() const {
  return getDisplayValue<T>(getDefaultValue());
}


template <typename T>
void Anchor<T>::resetToDefault() {
  value_ = getDefaultValue();
}


template <typename T>
void Anchor<T>::addGeneralInstance(ContextEnum ctxt, const T& value) {
  // Does not allow to specify a 'dFault' instance
  if (ctxt == ContextEnum::dFault) {
    std::string code = std::string("Anchor<T>::addGeneralInstance") +
                       std::string("::") + std::string(" Default for ") + smap_[ctxt] +
                       std::string(" Can Not Be Added ");
    throw std::runtime_error(code);
  }
  //---
  if (specifications_.count(ctxt) > 0) {
    std::string code = std::string("Anchor<T>::addGeneralInstance") +
                       std::string(" Context ") + smap_[ctxt] +
                       std::string(" Already Inserted ");
    throw std::runtime_error(code);
    return;
  }
  Instance<T> myCase(value, static_cast<AnchorBase*>(this));
  specifications_.insert(std::make_pair(ctxt, myCase));
  if (
    (ordering_.count(ctxt) == 0) || (ordering_[ctxt] != OrderContextEnum::MAX))
    ordering_[ctxt] = emap_[ctxt];
}


template <typename T>
void Anchor<T>::checkExcludes() const {
  for (auto opt_ex : excludes_) {
    // Check whether the 'excluded' options have specifications
    // in addition to their default ones.
    if ((opt_ex->count() > 1) && (this->count() > 1)) {
      std::string code = std::string("checkExcludes") + std::string("::") +
                         name_ + std::string(" excludes ") + opt_ex->getName();
      throw std::runtime_error(code);
    }
  }
}


template <typename T>
void Anchor<T>::resolve() {
  if (isResolved_)
    return;
  //
  try {
    checkExcludes();
  } catch (const std::exception& e) {
    throw;
  }
  //
  int cmax = 0;
  ContextEnum rslvContext_ = ContextEnum::dFault;
  for (const auto& iter : specifications_) {
    auto my_ctxt = iter.first;
    if (
      (ordering_.count(my_ctxt) > 0) &&
      (static_cast<int>(ordering_[my_ctxt]) > cmax)) {
      rslvContext_ = my_ctxt;
      resolvedInstance_ = iter.second;
      cmax = static_cast<int>(ordering_[my_ctxt]);
    }
  }
  //
  value_ = resolvedInstance_.getValue();
  if ((cmax <= 1) || (rslvContext_ == ContextEnum::dFault))
    resolvedToDefault_ = true;
  //
  isResolved_ = true;
}


template <typename T>
void Anchor<T>::setBannerMsgWarning(
  std::string msg_compile, std::function<bool()> fun) {
  screenPrint_ = std::make_unique<Warning>(this, msg_compile, fun);
}

}} /* end namespace vt::arguments */
