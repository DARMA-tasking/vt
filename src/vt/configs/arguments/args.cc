/*
//@HEADER
// *****************************************************************************
//
//                                   args.cc
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

#include "vt/config.h"
#include "vt/configs/arguments/args.h"
#include "vt/context/context.h"

#include <string>
#include <vector>
#include <sstream>
#include <tuple>
#include <ctime>

#include "CLI/CLI11.hpp"

namespace vt { namespace arguments {

// Temporary variables used only for parsing artifacts.
namespace {

std::vector<std::string> arg_trace_mpi;

/**
 * \internal
 * Application specific cleanup and mapping to actual app args.
 */
void postParseTransform(AppConfig& appConfig) {
  auto contains = [](std::vector<std::string> &v, std::string str){
    return std::find(v.begin(), v.end(), str) not_eq v.end();
  };

  appConfig.vt_trace_mpi = contains(arg_trace_mpi, "internal");
  appConfig.vt_trace_pmpi = contains(arg_trace_mpi, "external");

  using config::ModeEnum;

  auto const& level = appConfig.vt_debug_level;
  if (level == "terse" or level == "0") {
    appConfig.vt_debug_level_val = ModeEnum::terse;
  } else if (level == "normal" or level == "1") {
    appConfig.vt_debug_level_val = ModeEnum::terse | ModeEnum::normal;
  } else if (level == "verbose" or level == "2") {
    appConfig.vt_debug_level_val =
      ModeEnum::terse | ModeEnum::normal | ModeEnum::verbose;
  } else {
    vtAbort("Invalid value passed to --vt_debug_level");
  }
}

std::tuple<int, std::string> parseArguments(
  CLI::App& app, int& argc, char**& argv, AppConfig& appConfig
) {

  std::vector<char*> vt_args;

  // Load up vectors (has curious ability to altnerate vt/mpi/passthru)
  std::vector<char*>* rargs = nullptr;
  for (int i = 1; i < argc; i++) {
    char* c = argv[i];
    if (0 == strcmp(c, "--vt_args")) {
      rargs = &vt_args;
    } else if (0 == strcmp(c, "--")) {
      rargs = &appConfig.passthru_args;
    } else if (rargs) {
      rargs->push_back(c);
    } else if (0 == strncmp(c, "--vt_", 5)) {
      // Implicit start of VT args allows pass-thru 'for compatibility'
      // although the recommended calling pattern to always provide VT args first.
      rargs = &vt_args;
      rargs->push_back(c);
    } else {
      appConfig.passthru_args.push_back(c);
    }
  }

  // All must be accounted for
  app.allow_extras(false);

  // Build string-vector and reverse order to parse (CLI quirk)
  std::vector<std::string> args_to_parse;
  for (auto it = vt_args.crbegin(); it != vt_args.crend(); ++it) {
    args_to_parse.push_back(*it);
  }

  // Allow a input config file
  app.set_config(
    "--vt_input_config",
    "", // no default file name
    "Read in an ini config file for VT",
    false // not required
  );

  try {
    app.parse(args_to_parse);
  } catch (CLI::Error &ex) {
    // Return exit code and message, delaying logic processing of such.
    // The default exit code for 'help' is 0.
    std::stringstream message_stream;
    int result = app.exit(ex, message_stream, message_stream);

    return std::make_tuple(result, message_stream.str());
  }

  // If the user specified to output the full configuration, save it in a string
  // so node 0 can output in the runtime once MPI is init'ed
  if (appConfig.vt_output_config) {
    appConfig.vt_output_config_str = app.config_to_str(true, true);
  }

  // Get the clean prog name; don't allow path bleed in usages.
  // std::filesystem is C++17.
  std::string clean_prog_name = argv[0];
  size_t l = clean_prog_name.find_last_of("/\\");
  if (l not_eq std::string::npos and l + 1 < clean_prog_name.size()) {
    clean_prog_name = clean_prog_name.substr(l + 1, std::string::npos);
  }

  appConfig.prog_name = clean_prog_name;
  appConfig.argv_prog_name = argv[0];

  postParseTransform(appConfig);

  // Rebuild passthru into ref-returned argc/argv

  // It should be possible to modify the original argv as the outgoing
  // number of arguments is always less. As currently allocated here,
  // ownership of the new object is ill-defined.
  int new_argc = appConfig.passthru_args.size() + 1; // does not include argv[0]

  static std::unique_ptr<char*[]> new_argv = nullptr;

  new_argv = std::make_unique<char*[]>(new_argc + 1);

  int i = 0;
  new_argv[i++] = appConfig.argv_prog_name;
  for (auto&& arg : appConfig.passthru_args) {
    new_argv[i++] = arg;
  }
  new_argv[i++] = nullptr;

  // Set them back with all vt (and MPI) arguments elided
  argc = new_argc;
  argv = new_argv.get();

  return std::make_tuple(-1, std::string{});
}

void addColorArgs(CLI::App& app, AppConfig& appConfig) {
  auto quiet  = "Quiet the output from vt (only errors, warnings)";
  auto always = "Colorize output (default)";
  auto never  = "Do not colorize output (overrides --vt_color)";
  auto a  = app.add_flag("--vt_color",    appConfig.vt_color,      always);
  auto b  = app.add_flag("--vt_no_color", appConfig.vt_no_color,   never);
  auto a1 = app.add_flag("--vt_quiet",    appConfig.vt_quiet,      quiet);
  auto outputGroup = "Output Control";
  a->group(outputGroup);
  b->group(outputGroup);
  a1->group(outputGroup);
  // Do not exclude 'a' from 'b' here because when inputting/outputting a
  // config, both will be written out causing an error when reading a written
  // input file with defaults
  // b->excludes(a);
}

void addSignalArgs(CLI::App& app, AppConfig& appConfig) {
  auto no_sigint      = "Do not register signal handler for SIGINT";
  auto no_sigsegv     = "Do not register signal handler for SIGSEGV";
  auto no_sigbus      = "Do not register signal handler for SIGBUS";
  auto no_terminate   = "Do not register handler for std::terminate";
  auto d = app.add_flag("--vt_no_SIGINT",    appConfig.vt_no_sigint,    no_sigint);
  auto e = app.add_flag("--vt_no_SIGSEGV",   appConfig.vt_no_sigsegv,   no_sigsegv);
  auto g = app.add_flag("--vt_no_SIGBUS",    appConfig.vt_no_sigbus,    no_sigbus);
  auto f = app.add_flag("--vt_no_terminate", appConfig.vt_no_terminate, no_terminate);
  auto signalGroup = "Signal Handling";
  d->group(signalGroup);
  e->group(signalGroup);
  f->group(signalGroup);
  g->group(signalGroup);
}

void addMemUsageArgs(CLI::App& app, AppConfig& appConfig) {
  /*
   * Flags for controlling memory usage reporting
   */
  auto mem_desc  = "List of memory reporters to query in order of precedence";
  auto mem_phase = "Print memory usage each new phase";
  auto mem_node  = "Node to print memory usage from or \"all\"";
  auto mem_ps    = "Enable memory reporting with PS (warning: forking to query 'ps' may not be scalable)";
  auto mem_at_thresh = "Print memory usage from scheduler when reaches a threshold increment";
  auto mem_thresh    = "The threshold increments to print memory usage: \"<value> {GiB,MiB,KiB,B}\"";
  auto mem_sched     = "The frequency to query the memory threshold check (some memory reporters might be expensive)";
  auto mem_footprint = "Print live components' memory footprint after initialization and before shutdown";
  auto mm = app.add_option("--vt_memory_reporters",          appConfig.vt_memory_reporters, mem_desc)->capture_default_str();
  auto mn = app.add_flag("--vt_print_memory_each_phase",     appConfig.vt_print_memory_each_phase, mem_phase);
  auto mo = app.add_option("--vt_print_memory_node",         appConfig.vt_print_memory_node, mem_node)->capture_default_str();
  auto mp = app.add_flag("--vt_allow_memory_report_with_ps", appConfig.vt_allow_memory_report_with_ps, mem_ps);
  auto mq = app.add_flag("--vt_print_memory_at_threshold",   appConfig.vt_print_memory_at_threshold, mem_at_thresh);
  auto mr = app.add_option("--vt_print_memory_threshold",    appConfig.vt_print_memory_threshold, mem_thresh)->capture_default_str();
  auto ms = app.add_option("--vt_print_memory_sched_poll",   appConfig.vt_print_memory_sched_poll, mem_sched)->capture_default_str();
  auto mf = app.add_flag("--vt_print_memory_footprint",      appConfig.vt_print_memory_footprint, mem_footprint);
  auto memoryGroup = "Memory Usage Reporting";
  mm->group(memoryGroup);
  mn->group(memoryGroup);
  mo->group(memoryGroup);
  mp->group(memoryGroup);
  mq->group(memoryGroup);
  mr->group(memoryGroup);
  ms->group(memoryGroup);
  mf->group(memoryGroup);
}

void addStackDumpArgs(CLI::App& app, AppConfig& appConfig) {
  /*
   * Flags to control stack dumping
   */
  auto stack  = "Do not dump stack traces";
  auto warn   = "Do not dump stack traces when vtWarn(..) is invoked";
  auto assert = "Do not dump stack traces when vtAssert(..) is invoked";
  auto abort  = "Do not dump stack traces when vtAabort(..) is invoked";
  auto file   = "Dump stack traces to file instead of stdout";
  auto dir    = "Name of directory to write stack files";
  auto mod    = "Write stack dump if (node % config_.vt_stack_mod) == 0";
  auto g = app.add_flag("--vt_no_warn_stack",   appConfig.vt_no_warn_stack,   warn);
  auto h = app.add_flag("--vt_no_assert_stack", appConfig.vt_no_assert_stack, assert);
  auto i = app.add_flag("--vt_no_abort_stack",  appConfig.vt_no_abort_stack,  abort);
  auto j = app.add_flag("--vt_no_stack",        appConfig.vt_no_stack,        stack);
  auto k = app.add_option("--vt_stack_file",    appConfig.vt_stack_file,      file)->capture_default_str();
  auto l = app.add_option("--vt_stack_dir",     appConfig.vt_stack_dir,       dir)->capture_default_str();
  auto m = app.add_option("--vt_stack_mod",     appConfig.vt_stack_mod,       mod)->capture_default_str();
  auto stackGroup = "Dump Stack Backtrace";
  g->group(stackGroup);
  h->group(stackGroup);
  i->group(stackGroup);
  j->group(stackGroup);
  k->group(stackGroup);
  l->group(stackGroup);
  m->group(stackGroup);
}

void addTraceArgs(CLI::App& app, AppConfig& appConfig) {
  /*
   * Flags to control tracing output
   */
  auto trace     = "Enable tracing (must be compiled with trace_enabled)";
  auto trace_mpi = "Enable tracing of MPI calls"
    " (must be compiled with trace_enabled)";
  auto tfile     = "Name of trace files";
  auto tdir      = "Name of directory for trace files";
  auto tmod      = "Output trace file if (node % config_.vt_stack_mod) == 0";
  auto zflush    = "Set flush mode to Z_FINISH for trace file";
  auto tflushmod = "Flush output trace every (vt_trace_flush_size) trace records";
  auto tsysall   = "Trace all system events";
  auto tsysTD    = "Trace system termination events";
  auto tsysloc   = "Trace system location manager events";
  auto tsyscoll  = "Trace system virtual context collection events";
  auto tsyssmsg  = "Trace system serialization manager events";
  auto tspec     = "Enable trace spec file (defines which phases tracing is on)";
  auto tspecfile = "File containing trace spec; --vt_trace_spec to enable";
  auto tmemusage = "Trace memory usage using first memory reporter";
  auto tpolled   = "Trace AsyncEvent component polling (inc. MPI_Isend requests)";
  auto tirecv     = "Trace MPI_Irecv request polling";
  auto n  = app.add_flag("--vt_trace",                   appConfig.vt_trace,                   trace);
  auto nm = app.add_option("--vt_trace_mpi",             arg_trace_mpi,                      trace_mpi)
    ->check(CLI::IsMember({"internal", "external"}));
  auto o  = app.add_option("--vt_trace_file",            appConfig.vt_trace_file,              tfile)->capture_default_str();
  auto p  = app.add_option("--vt_trace_dir",             appConfig.vt_trace_dir,               tdir)->capture_default_str();
  auto q  = app.add_option("--vt_trace_mod",             appConfig.vt_trace_mod,               tmod)->capture_default_str();
  auto qf = app.add_option("--vt_trace_flush_size",      appConfig.vt_trace_flush_size,        tflushmod)->capture_default_str();
  auto qg = app.add_flag("--vt_trace_gzip_finish_flush", appConfig.vt_trace_gzip_finish_flush, zflush);
  auto qt = app.add_flag("--vt_trace_sys_all",           appConfig.vt_trace_sys_all,           tsysall);
  auto qw = app.add_flag("--vt_trace_sys_term",          appConfig.vt_trace_sys_term,          tsysTD);
  auto qx = app.add_flag("--vt_trace_sys_location",      appConfig.vt_trace_sys_location,      tsysloc);
  auto qy = app.add_flag("--vt_trace_sys_collection",    appConfig.vt_trace_sys_collection,    tsyscoll);
  auto qz = app.add_flag("--vt_trace_sys_serial_msg",    appConfig.vt_trace_sys_serial_msg,    tsyssmsg);
  auto qza = app.add_flag("--vt_trace_spec",             appConfig.vt_trace_spec,              tspec);
  auto qzb = app.add_option("--vt_trace_spec_file",      appConfig.vt_trace_spec_file,         tspecfile)->capture_default_str()->check(CLI::ExistingFile);
  auto qzc = app.add_flag("--vt_trace_memory_usage",     appConfig.vt_trace_memory_usage,      tmemusage);
  auto qzd = app.add_flag("--vt_trace_event_polling",    appConfig.vt_trace_event_polling,     tpolled);
  auto qze = app.add_flag("--vt_trace_irecv_polling",    appConfig.vt_trace_irecv_polling,     tirecv);
  auto traceGroup = "Tracing Configuration";
  n->group(traceGroup);
  nm->group(traceGroup);
  o->group(traceGroup);
  p->group(traceGroup);
  q->group(traceGroup);
  qf->group(traceGroup);
  qg->group(traceGroup);
  qt->group(traceGroup);
  qw->group(traceGroup);
  qx->group(traceGroup);
  qy->group(traceGroup);
  qz->group(traceGroup);
  qza->group(traceGroup);
  qzb->group(traceGroup);
  qzc->group(traceGroup);
  qzd->group(traceGroup);
  qze->group(traceGroup);
}

void addDebugPrintArgs(CLI::App& app, AppConfig& appConfig) {
  #define debug_pp(opt) +std::string(config::PrettyPrintCat<config::opt>::str)+

  auto rp  = "Enable all debug prints";
  auto rq  = "Set level for debug prints (0=>terse, 1=>normal, 2=>verbose)";
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
  auto pap = "Enable debug_handler      = \"" debug_pp(handler)      "\"";
  auto qap = "Enable debug_hierlb       = \"" debug_pp(hierlb)       "\"";
  auto qbp = "Enable debug_temperedlb   = \"" debug_pp(temperedlb)   "\"";
  auto qcp = "Enable debug_temperedwmin = \"" debug_pp(temperedwmin) "\"";
  auto rap = "Enable debug_scatter      = \"" debug_pp(scatter)      "\"";
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
  auto dcp = "Enable debug_phase        = \"" debug_pp(phase)        "\"";
  auto ddp = "Enable debug_context      = \"" debug_pp(context)      "\"";
  auto dep = "Enable debug_epoch        = \"" debug_pp(epoch)        "\"";

  auto r1 = app.add_option("--vt_debug_level",      appConfig.vt_debug_level,        rq);

  auto r  = app.add_flag("--vt_debug_all",          appConfig.vt_debug_all,          rp);
  auto aa = app.add_flag("--vt_debug_none",         appConfig.vt_debug_none,         aap);
  auto ba = app.add_flag("--vt_debug_gen",          appConfig.vt_debug_gen,          bap);
  auto ca = app.add_flag("--vt_debug_runtime",      appConfig.vt_debug_runtime,      cap);
  auto da = app.add_flag("--vt_debug_active",       appConfig.vt_debug_active,       dap);
  auto ea = app.add_flag("--vt_debug_term",         appConfig.vt_debug_term,         eap);
  auto fa = app.add_flag("--vt_debug_termds",       appConfig.vt_debug_termds,       fap);
  auto ga = app.add_flag("--vt_debug_barrier",      appConfig.vt_debug_barrier,      gap);
  auto ha = app.add_flag("--vt_debug_event",        appConfig.vt_debug_event,        hap);
  auto ia = app.add_flag("--vt_debug_pipe",         appConfig.vt_debug_pipe,         iap);
  auto ja = app.add_flag("--vt_debug_pool",         appConfig.vt_debug_pool,         jap);
  auto ka = app.add_flag("--vt_debug_reduce",       appConfig.vt_debug_reduce,       kap);
  auto la = app.add_flag("--vt_debug_rdma",         appConfig.vt_debug_rdma,         lap);
  auto ma = app.add_flag("--vt_debug_rdma_channel", appConfig.vt_debug_rdma_channel, map);
  auto na = app.add_flag("--vt_debug_rdma_state",   appConfig.vt_debug_rdma_state,   nap);
  auto pa = app.add_flag("--vt_debug_handler",      appConfig.vt_debug_handler,      pap);
  auto qa = app.add_flag("--vt_debug_hierlb",       appConfig.vt_debug_hierlb,       qap);
  auto qb = app.add_flag("--vt_debug_temperedlb",   appConfig.vt_debug_temperedlb,   qbp);
  auto qc = app.add_flag("--vt_debug_temperedwmin", appConfig.vt_debug_temperedwmin, qcp);
  auto ra = app.add_flag("--vt_debug_scatter",      appConfig.vt_debug_scatter,      rap);
  auto ua = app.add_flag("--vt_debug_serial_msg",   appConfig.vt_debug_serial_msg,   uap);
  auto va = app.add_flag("--vt_debug_trace",        appConfig.vt_debug_trace,        vap);
  auto wa = app.add_flag("--vt_debug_location",     appConfig.vt_debug_location,     wap);
  auto xa = app.add_flag("--vt_debug_lb",           appConfig.vt_debug_lb,           xap);
  auto ya = app.add_flag("--vt_debug_vrt",          appConfig.vt_debug_vrt,          yap);
  auto za = app.add_flag("--vt_debug_vrt_coll",     appConfig.vt_debug_vrt_coll,     zap);
  auto ab = app.add_flag("--vt_debug_worker",       appConfig.vt_debug_worker,       abp);
  auto bb = app.add_flag("--vt_debug_group",        appConfig.vt_debug_group,        bbp);
  auto cb = app.add_flag("--vt_debug_broadcast",    appConfig.vt_debug_broadcast,    cbp);
  auto db = app.add_flag("--vt_debug_objgroup",     appConfig.vt_debug_objgroup,     dbp);
  auto dc = app.add_flag("--vt_debug_phase",        appConfig.vt_debug_phase,        dcp);
  auto dd = app.add_flag("--vt_debug_context",      appConfig.vt_debug_context,      ddp);
  auto de = app.add_flag("--vt_debug_epoch",        appConfig.vt_debug_epoch,        dep);

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
  pa->group(debugGroup);
  qa->group(debugGroup);
  qb->group(debugGroup);
  qc->group(debugGroup);
  ra->group(debugGroup);
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
  dc->group(debugGroup);
  dd->group(debugGroup);
  de->group(debugGroup);

  auto dbq = "Always flush VT runtime prints";
  auto eb  = app.add_flag("--vt_debug_print_flush", appConfig.vt_debug_print_flush, dbq);
  eb->group(debugGroup);
}

void addLbArgs(CLI::App& app, AppConfig& appConfig) {
  /*
   * Flags for enabling load balancing and configuring it
   */
  auto lb             = "Enable load balancing";
  auto lb_args        = "Arguments pass to LB: \"x=0 y=1\"; try --vt_help_lb_args";
  auto lb_quiet       = "Silence load balancing output";
  auto lb_file_name   = "LB config file to read";
  auto lb_show_config = "Show LB config during startup";
  auto lb_name       = "Name of the load balancer to use";
  auto lb_interval   = "Load balancing interval";
  auto lb_keep_last_elm = "Do not migrate last element in collection";
  auto lb_data      = "Enable load balancing data";
  auto lb_data_in   = "Enable load balancing data input";
  auto lb_data_comp = "Compress load balancing data output with brotli";
  auto lb_data_dir  = "Load balancing data output directory";
  auto lb_data_file = "Load balancing data output file name";
  auto lb_data_dir_in  = "Load balancing data input directory";
  auto lb_data_file_in = "Load balancing data input file name";
  auto lb_statistics = "Dump load balancing statistics to file";
  auto lb_statistics_comp = "Compress load balancing statistics file with brotli";
  auto lb_statistics_file = "Load balancing statistics output file name";
  auto lb_statistics_dir  = "Load balancing statistics output directory name";
  auto lb_self_migration = "Allow load balancer to migrate objects to the same node";
  auto lb_spec      = "Enable LB spec file (defines which phases output LB data)";
  auto lb_spec_file = "File containing LB spec; --vt_lb_spec to enable";
  auto s  = app.add_flag("--vt_lb", appConfig.vt_lb, lb);
  auto t1 = app.add_flag("--vt_lb_quiet", appConfig.vt_lb_quiet, lb_quiet);
  auto u  = app.add_option("--vt_lb_file_name", appConfig.vt_lb_file_name, lb_file_name)->capture_default_str()->check(CLI::ExistingFile);
  auto u1 = app.add_flag("--vt_lb_show_config", appConfig.vt_lb_show_config, lb_show_config);
  auto v  = app.add_option("--vt_lb_name", appConfig.vt_lb_name, lb_name)->capture_default_str();
  auto v1 = app.add_option("--vt_lb_args", appConfig.vt_lb_args, lb_args)->capture_default_str();
  auto w  = app.add_option("--vt_lb_interval", appConfig.vt_lb_interval, lb_interval)->capture_default_str();
  auto wl = app.add_flag("--vt_lb_keep_last_elm", appConfig.vt_lb_keep_last_elm, lb_keep_last_elm);
  auto ww = app.add_flag("--vt_lb_data", appConfig.vt_lb_data, lb_data);
  auto za = app.add_flag("--vt_lb_data_in", appConfig.vt_lb_data_in, lb_data_in);
  auto xz = app.add_flag("--vt_lb_data_compress", appConfig.vt_lb_data_compress, lb_data_comp);
  auto wx = app.add_option("--vt_lb_data_dir", appConfig.vt_lb_data_dir, lb_data_dir)->capture_default_str();
  auto wy = app.add_option("--vt_lb_data_file", appConfig.vt_lb_data_file, lb_data_file)->capture_default_str();
  auto xx = app.add_option("--vt_lb_data_dir_in", appConfig.vt_lb_data_dir_in, lb_data_dir_in)->capture_default_str();
  auto xy = app.add_option("--vt_lb_data_file_in", appConfig.vt_lb_data_file_in, lb_data_file_in)->capture_default_str();
  auto yx = app.add_flag("--vt_lb_statistics",          appConfig.vt_lb_statistics,          lb_statistics);
  auto yy = app.add_flag("--vt_lb_statistics_compress", appConfig.vt_lb_statistics_compress, lb_statistics_comp);
  auto yz = app.add_option("--vt_lb_statistics_file",   appConfig.vt_lb_statistics_file,     lb_statistics_file)->capture_default_str();
  auto zz = app.add_option("--vt_lb_statistics_dir",    appConfig.vt_lb_statistics_dir,      lb_statistics_dir)->capture_default_str();
  auto lbasm = app.add_flag("--vt_lb_self_migration",   appConfig.vt_lb_self_migration,      lb_self_migration);
  auto lbspec = app.add_flag("--vt_lb_spec",            appConfig.vt_lb_spec,                lb_spec);
  auto lbspecfile = app.add_option("--vt_lb_spec_file", appConfig.vt_lb_spec_file,           lb_spec_file)->capture_default_str()->check(CLI::ExistingFile);

  // --vt_lb_name excludes --vt_lb_file_name, and vice versa
  v->excludes(u);
  u->excludes(v);

  auto debugLB = "Load Balancing";
  s->group(debugLB);
  t1->group(debugLB);
  u->group(debugLB);
  u1->group(debugLB);
  v->group(debugLB);
  v1->group(debugLB);
  w->group(debugLB);
  wl->group(debugLB);
  ww->group(debugLB);
  wx->group(debugLB);
  za->group(debugLB);
  wy->group(debugLB);
  xx->group(debugLB);
  xy->group(debugLB);
  xz->group(debugLB);
  yx->group(debugLB);
  yy->group(debugLB);
  yz->group(debugLB);
  zz->group(debugLB);
  lbasm->group(debugLB);
  lbspec->group(debugLB);
  lbspecfile->group(debugLB);

  // help options deliberately omitted from the debugLB group above so that
  // they appear grouped with --vt_help when --vt_help is used
  auto help_lb_args  = "Print help for --vt_lb_args";
  auto h1 = app.add_flag("--vt_help_lb_args", appConfig.vt_help_lb_args, help_lb_args);
  (void) h1;
}

void addDiagnosticArgs(CLI::App& app, AppConfig& appConfig) {
  /*
   * Flags for controlling diagnostic collection and output
   */
  auto diag = "Enable diagnostic (performance metrics/stats) collection";
  auto sum  = "Print diagnostic summary table to stdout at finalization";
  auto file = "Output diagnostic summary table to text file";
  auto csv  = "Output diagnostic summary table to a comma-separated file";
  auto base = "Use base units (seconds, units, etc.) for CSV file output";
  auto a = app.add_flag("--vt_diag_enable,!--vt_diag_disable", appConfig.vt_diag_enable,           diag);
  auto b = app.add_flag("--vt_diag_print_summary",             appConfig.vt_diag_print_summary,    sum);
  auto c = app.add_option("--vt_diag_summary_file",            appConfig.vt_diag_summary_file,     file);
  auto d = app.add_option("--vt_diag_summary_csv_file",        appConfig.vt_diag_summary_csv_file, csv);
  auto e = app.add_flag("--vt_diag_csv_base_units",            appConfig.vt_diag_csv_base_units,   base);

  auto diagnosticGroup = "Diagnostics";
  a->group(diagnosticGroup);
  b->group(diagnosticGroup);
  c->group(diagnosticGroup);
  d->group(diagnosticGroup);
  e->group(diagnosticGroup);
}

void addTerminationArgs(CLI::App& app, AppConfig& appConfig) {
  auto hang         = "Disable termination hang detection";
  auto hang_freq    = "The number of tree traversals before a hang is detected";
  auto ds           = "Force use of Dijkstra-Scholten (DS) algorithm for rooted epoch termination detection";
  auto wave         = "Force use of 4-counter algorithm for rooted epoch termination detection";
  auto graph_on     = "Output epoch graph to file (DOT) when hang is detected";
  auto terse        = "Output epoch graph to file in terse mode";
  auto progress     = "Print termination counts when progress is stalled";
  auto x  = app.add_flag("--vt_no_detect_hang",        appConfig.vt_no_detect_hang,       hang);
  auto x1 = app.add_flag("--vt_term_rooted_use_ds",    appConfig.vt_term_rooted_use_ds,   ds);
  auto x2 = app.add_flag("--vt_term_rooted_use_wave",  appConfig.vt_term_rooted_use_wave, wave);
  auto x3 = app.add_option("--vt_epoch_graph_on_hang", appConfig.vt_epoch_graph_on_hang,  graph_on)->capture_default_str();
  auto x4 = app.add_flag("--vt_epoch_graph_terse",     appConfig.vt_epoch_graph_terse,    terse);
  auto x5 = app.add_option("--vt_print_no_progress",   appConfig.vt_print_no_progress,    progress)->capture_default_str();
  auto y = app.add_option("--vt_hang_freq",            appConfig.vt_hang_freq,            hang_freq)->capture_default_str();
  auto debugTerm = "Termination";
  x->group(debugTerm);
  x1->group(debugTerm);
  x2->group(debugTerm);
  x3->group(debugTerm);
  x4->group(debugTerm);
  x5->group(debugTerm);
  y->group(debugTerm);
}

void addDebuggerArgs(CLI::App& app, AppConfig& appConfig) {
  auto pause        = "Pause at startup so GDB/LLDB can be attached";
  auto z = app.add_flag("--vt_pause", appConfig.vt_pause, pause);
  auto launchTerm = "Debugging/Launch";
  z->group(launchTerm);
}

void addUserArgs(CLI::App& app, AppConfig& appConfig) {
  auto user1    = "User Option 1a (boolean)";
  auto user2    = "User Option 2a (boolean)";
  auto user3    = "User Option 3a (boolean)";
  auto userint1 = "User Option 1b (int32_t)";
  auto userint2 = "User Option 2b (int32_t)";
  auto userint3 = "User Option 3b (int32_t)";
  auto userstr1 = "User Option 1c (std::string)";
  auto userstr2 = "User Option 2c (std::string)";
  auto userstr3 = "User Option 3c (std::string)";
  auto u1  = app.add_flag("--vt_user_1",       appConfig.vt_user_1, user1);
  auto u2  = app.add_flag("--vt_user_2",       appConfig.vt_user_2, user2);
  auto u3  = app.add_flag("--vt_user_3",       appConfig.vt_user_3, user3);
  auto ui1 = app.add_option("--vt_user_int_1", appConfig.vt_user_int_1, userint1)->capture_default_str();
  auto ui2 = app.add_option("--vt_user_int_2", appConfig.vt_user_int_2, userint2)->capture_default_str();
  auto ui3 = app.add_option("--vt_user_int_3", appConfig.vt_user_int_3, userint3)->capture_default_str();
  auto us1 = app.add_option("--vt_user_str_1", appConfig.vt_user_str_1, userstr1)->capture_default_str();
  auto us2 = app.add_option("--vt_user_str_2", appConfig.vt_user_str_2, userstr2)->capture_default_str();
  auto us3 = app.add_option("--vt_user_str_3", appConfig.vt_user_str_3, userstr3)->capture_default_str();
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
}

void addSchedulerArgs(CLI::App& app, AppConfig& appConfig) {
  auto nsched = "Number of times to run the progress function in scheduler";
  auto ksched = "Run the MPI progress function at least every k handlers that run";
  auto ssched = "Run the MPI progress function at least every s seconds";
  auto sca = app.add_option("--vt_sched_num_progress", appConfig.vt_sched_num_progress, nsched)->capture_default_str();
  auto hca = app.add_option("--vt_sched_progress_han", appConfig.vt_sched_progress_han, ksched)->capture_default_str();
  auto kca = app.add_option("--vt_sched_progress_sec", appConfig.vt_sched_progress_sec, ssched)->capture_default_str();
  auto schedulerGroup = "Scheduler Configuration";
  sca->group(schedulerGroup);
  hca->group(schedulerGroup);
  kca->group(schedulerGroup);
}

void addConfigFileArgs(CLI::App& app, AppConfig& appConfig) {
  auto doconfig   = "Output all VT args to configuration file";
  auto configname = "Name of configuration file to output";

  auto a1 = app.add_flag("--vt_output_config",        appConfig.vt_output_config, doconfig);
  auto a2 = app.add_option("--vt_output_config_file", appConfig.vt_output_config_file, configname)->capture_default_str();

  auto configGroup = "Configuration File";
  a1->group(configGroup);
  a2->group(configGroup);
}

void addRuntimeArgs(CLI::App& app, AppConfig& appConfig) {
  auto max_size = "Maximum MPI send size (causes larger messages to be split "
                  "into multiple MPI sends)";
  auto assert = "Do not abort the program when vtAssert(..) is invoked";
  auto throw_on_abort = "Throw an exception when vtAbort(..) is called";


  auto a1 = app.add_option(
    "--vt_max_mpi_send_size", appConfig.vt_max_mpi_send_size, max_size
  )->capture_default_str();
  auto a2 = app.add_flag(
    "--vt_no_assert_fail", appConfig.vt_no_assert_fail, assert
  );
  auto a3 = app.add_flag(
    "--vt_throw_on_abort", appConfig.vt_throw_on_abort, throw_on_abort
  );


  auto configRuntime = "Runtime";
  a1->group(configRuntime);
  a2->group(configRuntime);
  a3->group(configRuntime);
}

void addThreadingArgs(CLI::App& app, AppConfig& appConfig) {
#if (vt_feature_fcontext != 0)
  auto ult_disable = "Disable running handlers in user-level threads";
  auto stack_size = "The default stack size for user-level threads";

  auto a1 = app.add_flag(
    "--vt_ult_disable", appConfig.vt_ult_disable, ult_disable
  );
  auto a2 = app.add_option(
    "--vt_ult_stack_size", appConfig.vt_ult_stack_size, stack_size
  )->capture_default_str();

  auto configThreads = "Threads";
  a1->group(configThreads);
  a2->group(configThreads);
#endif
}

} /* end anon namespace */

/*static*/ std::unique_ptr<ArgConfig>
ArgConfig::construct(std::unique_ptr<ArgConfig> arg) {
  return arg;
}

class VtFormatter : public CLI::Formatter {
public:
  std::string make_usage(const CLI::App *, std::string name) const override {
    std::stringstream u;
    u << "\n"
"Usage:"
"\n"
"[APP-ARGS..] [VT-ARGS] [-- APP-ARGS..]\n"
"\n"
"Arguments up until the first '--vt_*' are treated as application arguments.\n"
"\n"
"After the first '--vt_*', additional arguments are treated as VT arguments\n"
"unless '--' is used to switch the argument mode back to application args.\n"
"The '--vt_args' flag can be used to switch back into VT argument mode.\n"
"Modes can be switched indefinitely.\n"
"\n"
"Application pass-through arguments are supplied to the host program\n"
"for further processing and are not used by VT for any configuration.\n"
"\n"
"It is an error if an unexpected argument is encountered in VT argument mode.\n"
"The currently recognized VT arguments are listed below; availability varies\n"
"based on build and compilation settings.\n"
      << "\n";
    return u.str();
  }
};

std::tuple<int, std::string> ArgConfig::parse(
  int& argc, char**& argv, AppConfig const* appConfig
) {
  // If user didn't define appConfig, parse into this->config_.
  if (not appConfig) {
    return parseToConfig(argc, argv, config_);
  }

  // If user defines appConfig, parse into temporary config for later comparison.
  AppConfig config{*appConfig};
  auto const parse_result = parseToConfig(argc, argv, config);

  config_ = config;

  return parse_result;
}

std::tuple<int, std::string> ArgConfig::parseToConfig(
  int& argc, char**& argv, AppConfig& appConfig
) {
  if (parsed_ || argc == 0 || argv == nullptr) {
    // Odd case.. pretend nothing bad happened.
    return std::make_tuple(-1, std::string{});
  }

  CLI::App app{"vt (Virtual Transport)"};

  app.formatter(std::make_shared<VtFormatter>());

  app.set_help_flag("--vt_help", "Display help");

  addColorArgs(app, appConfig);
  addSignalArgs(app, appConfig);
  addMemUsageArgs(app, appConfig);
  addStackDumpArgs(app, appConfig);
  addTraceArgs(app, appConfig);
  addDebugPrintArgs(app, appConfig);
  addLbArgs(app, appConfig);
  addDiagnosticArgs(app, appConfig);
  addTerminationArgs(app, appConfig);
  addDebuggerArgs(app, appConfig);
  addUserArgs(app, appConfig);
  addSchedulerArgs(app, appConfig);
  addConfigFileArgs(app, appConfig);
  addRuntimeArgs(app, appConfig);
  addThreadingArgs(app, appConfig);

  std::tuple<int, std::string> result = parseArguments(app, argc, argv, appConfig);
  if (std::get<0>(result) not_eq -1) {
    // non-success
    return result;
  }

  // Determine the final colorization setting.
  if (appConfig.vt_no_color) {
    appConfig.colorize_output = false;
  } else {
    // Otherwise, colorize.
    // (Within MPI there is no good method to auto-detect.)
    appConfig.colorize_output = true;
  }

  parsed_ = true;

  return result;
}

namespace {
static std::string buildRankFile(std::string const& file) {
  std::string name = file;
  std::size_t rank = name.find("%p");
  auto str_rank = std::to_string(theContext()->getNode());
  if (rank == std::string::npos) {
    name = name + str_rank;
  } else {
    name.replace(rank, 2, str_rank);
  }
  return name;
}

static std::string buildFile(
  std::string const& file, std::string const& dir
) {
  auto const name = buildRankFile(file);
  return dir + "/" + name;
}

static std::string buildFileWithBrExtension(
  std::string const& file, std::string const& dir
) {
  auto name = buildRankFile(file);
  if (name.substr(name.length()-3, 3) != ".br") {
    name = name + ".br";
  }
  return dir + "/" + name;
}
} /* end anon namespace */

std::string AppConfig::getLBDataFileOut() const {
  if (vt_lb_data_compress) {
    return buildFileWithBrExtension(vt_lb_data_file, vt_lb_data_dir);
  } else {
    return buildFile(vt_lb_data_file, vt_lb_data_dir);
  }
}

std::string AppConfig::getLBDataFileIn() const {
  return buildFile(vt_lb_data_file_in, vt_lb_data_dir_in);
}

std::string AppConfig::getLBStatisticsFile() const {
  std::string name = vt_lb_statistics_file;
  std::string dir = vt_lb_statistics_dir;
  if (not dir.empty()) {
    name = dir + "/" + name;
  }
  std::size_t timestamp = name.find("%t");
  if (timestamp != std::string::npos) {
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    std::stringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d-%H-%M-%S");
    name.replace(timestamp, 2, ss.str());
  }
  if (vt_lb_statistics_compress and name.substr(name.length()-3, 3) != ".br") {
    name = name + ".br";
  }
  return name;
}

}} /* end namespace vt::arguments */
