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
#include "vt/trace/trace.h"

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
/*static*/ bool Args::parsed_ = false;
/*static*/ std::unique_ptr<ArgsManager> Args::manager_ = {};


std::string verifyName(const std::string &name)
{
  int ipos = 0;
  while (name[ipos] == '-') {
    ipos++;
  }
  std::string tmpName = name.substr(ipos);
  //
  if (!CLI::detail::valid_name_string(tmpName)) {
    std::string code = std::string(" Invalid Name ") + name;
    throw std::invalid_argument(code);
  }
  //
  return tmpName;
}


/* ------------------------------------------------- */
// --- Member functions of ArgsManager
/* ------------------------------------------------- */


ArgsManager::ArgsManager(const std::string& name) :
  app_(std::make_unique<CLI::App>(name))
{
  initializeOutputControl();
  initializeSignalHandling();
  initializeTermination();
  initializeStackGroup();
  initializeTracing();
  initializeLoadBalancing();

  /*
   * Pausing flag for attaching debugger
   */
  auto str_pause = "Pause at Startup to Attach GDB/LLDB";
  auto str_grp = std::string("Debugging/Launch");
  auto ptr_pause = addFlag("vt_pause",
    Args::config.vt_pause, str_pause, str_grp);
  ptr_pause->setPrintOption(PrintOption::whenSet, nullptr);

  initializeUserOptions();
  initializeDebug();
}

void ArgsManager::initializeOutputControl()
{
  //--- Flags for controlling the colorization of output from vt

  auto outputGroup = std::string("Output Control");

  auto ptr_color = addFlag("vt_color",
    Args::config.vt_color, "Colorize output", outputGroup);

  auto ptr_no_color = addFlag("vt_no_color",
    Args::config.vt_no_color, "No colored output", outputGroup);
  ptr_no_color->setPrintOption(PrintOption::always, nullptr);

  auto ptr_auto = addFlag("vt_auto_color",
    Args::config.vt_auto_color, "Colorization of output with isatty",
    outputGroup);
  ptr_auto->setPrintOption(PrintOption::whenSet, nullptr);

  ptr_no_color->excludes(ptr_color);
  ptr_no_color->excludes(ptr_auto);

  auto quiet = std::string("Quiet VT-output (only errors, warnings)");
  auto ptr_quiet = addFlag("vt_quiet",
    Args::config.vt_quiet, quiet, outputGroup);
}

void ArgsManager::initializeSignalHandling()
{
  //--- Flags for controlling the signals that VT tries to catch

  auto signalGroup = std::string("Signal Handling");

  auto no_sigint = std::string("No signal handler registration for SIGINT");
  auto ptr_no_sigint = addFlag("vt_no_SIGINT",
    Args::config.vt_no_sigint, no_sigint, signalGroup);
  ptr_no_sigint->setPrintOption(PrintOption::always, nullptr);

  auto no_sigsegv = std::string("No signal handler registration for SIGSEGV");
  auto ptr_no_sigsegv = addFlag("vt_no_SIGSEGV",
    Args::config.vt_no_sigsegv, no_sigsegv, signalGroup);
  ptr_no_sigsegv->setPrintOption(PrintOption::always, nullptr);

  auto no_terminate = std::string(
    "No signal handler registration for std::terminate");
  auto ptr_no_terminate = addFlag("vt_no_terminate",
    Args::config.vt_no_terminate, no_terminate, signalGroup);
  ptr_no_terminate->setPrintOption(PrintOption::always, nullptr);
}

void ArgsManager::initializeTermination()
{
  //--- Flags for controlling termination
  auto termGroup = std::string("Termination");

  auto hang = std::string("No hang detection termination");
  auto ptr_no_detect_hang = addFlag("vt_no_detect_hang",
    Args::config.vt_no_detect_hang, hang, termGroup);
  ptr_no_detect_hang->setPrintOption(PrintOption::always, nullptr);

  auto ds = std::string(
    "Force use of Dijkstra-Scholten (DS) algorithm for rooted epoch "
    "termination detection");
  auto ptr_rooted_ds = addFlag("vt_term_rooted_use_ds",
    Args::config.vt_term_rooted_use_ds, ds, termGroup);
  ptr_rooted_ds->setPrintOption(PrintOption::whenSet, nullptr);

  auto wave = std::string(
    "Force use of 4-counter algorithm for rooted epoch termination detection");
  auto ptr_rooted_wave = addFlag("vt_term_rooted_use_wave",
    Args::config.vt_term_rooted_use_wave, wave, termGroup);
  ptr_rooted_wave->setPrintOption(PrintOption::whenSet, nullptr);

  auto hang_freq = "Number of tree traversals before detecting a hang";
  auto ptr_hang_freq = addOption("vt_hang_freq",
    Args::config.vt_hang_freq, hang_freq, termGroup);

  auto testFcnTerm = [&]() {
    return !Args::config.vt_no_detect_hang;
  };
  ptr_hang_freq->setPrintOption(PrintOption::whenSet, testFcnTerm);
  ptr_hang_freq->needsOptionOff(ptr_no_detect_hang);
}

void ArgsManager::initializeStackGroup()
{
  //--- Flags to control stack dumping

  auto stackGroup = std::string("Stack Dump Backtrace");

  auto stack = std::string("No strack traces dump");
  auto ptr_no_stack = addFlag("vt_no_stack",
    Args::config.vt_no_stack, stack, stackGroup);
  ptr_no_stack->setPrintOption(PrintOption::always, nullptr);

  auto testFcnStack = [&]() { return !Args::config.vt_no_stack; };

  auto warn = std::string("No stack traces dump when vtWarn(..) invoked");
  auto ptr_no_warn_stack = addFlag("vt_no_warn_stack",
    Args::config.vt_no_warn_stack, warn, stackGroup);
  ptr_no_warn_stack->setPrintOption(PrintOption::whenSet, testFcnStack);

  auto assert = std::string("No stack traces dump when vtAssert(..) invoked");
  auto ptr_no_assert_stack = addFlag("vt_no_assert_stack",
    Args::config.vt_no_assert_stack, assert, stackGroup);
  ptr_no_assert_stack->setPrintOption(PrintOption::whenSet, testFcnStack);

  auto abort = std::string("No stack traces dump when vtAbort(..) invoked");
  auto ptr_no_abort_stack = addFlag("vt_no_abort_stack",
    Args::config.vt_no_abort_stack, abort, stackGroup);
  ptr_no_abort_stack->setPrintOption(PrintOption::whenSet, testFcnStack);

  auto file = std::string("File for dumping stack traces");
  auto ptr_stack_file = addOption("vt_stack_file",
    Args::config.vt_stack_file, file, stackGroup);
  ptr_stack_file->setPrintOption(PrintOption::whenSet, testFcnStack);

  auto dir = std::string("Directory for dumping stack traces");
  auto ptr_stack_dir = addOption("vt_stack_dir",
    Args::config.vt_stack_dir, dir, stackGroup);
  ptr_stack_dir->setPrintOption(PrintOption::whenSet, testFcnStack);

  auto mod =
    std::string("Modulus for dumping stack traces (node % vt_stack_mod == 0)");
  auto ptr_stack_mod = addOption("vt_stack_mod",
    Args::config.vt_stack_mod, mod, stackGroup);
  ptr_stack_mod->setPrintOption(PrintOption::whenSet, testFcnStack);

  //--- Constraints
  ptr_no_warn_stack->needsOptionOff(ptr_no_stack);
  ptr_no_assert_stack->needsOptionOff(ptr_no_stack);
  ptr_no_abort_stack->needsOptionOff(ptr_no_stack);

  ptr_stack_file->needsOptionOff(ptr_no_stack);
  ptr_stack_dir->needsOptionOff(ptr_no_stack);
  ptr_stack_mod->needsOptionOff(ptr_no_stack);
}

void ArgsManager::initializeTracing()
{
  //--- Flags to control tracing output
  auto traceGroup = std::string("Tracing Configuration");

  auto trace = "Tracing (must be compiled with trace_enabled)";
  auto ptr_trace = addFlag("vt_trace",
    Args::config.vt_trace, trace, traceGroup);

  auto tfile = std::string("File for traces");
  auto ptr_trace_file = addOption("vt_trace_file",
    Args::config.vt_trace_file, tfile, traceGroup);

  auto tdir = "Directory for trace files";
  auto ptr_trace_dir = addOption("vt_trace_dir",
    Args::config.vt_trace_dir, tdir, traceGroup);

  auto tmod = "Modulus for outputting traces";
  auto ptr_trace_mod = addOption("vt_trace_mod",
    Args::config.vt_trace_mod, tmod, traceGroup);

  ptr_trace_file->needsOptionOn(ptr_trace);
  ptr_trace_dir->needsOptionOn(ptr_trace);
  ptr_trace_mod->needsOptionOn(ptr_trace);

#if !backend_check_enabled(trace_enabled)
  ptr_trace->setBannerMsgWarning("trace_enabled");
#else
  auto testFcnTrace = [&]() { return Args::config.vt_trace; };
  ptr_trace_file->setPrintOption(PrintOption::whenSet, testFcnTrace);
  ptr_trace_dir->setPrintOption(PrintOption::whenSet, testFcnTrace);
  ptr_trace_mod->setPrintOption(PrintOption::whenSet, testFcnTrace);
#endif
}

void ArgsManager::initializeLoadBalancing()
{
  //--- Flags for enabling load balancing and configuring it
  auto str_lbGroup = std::string("Load Balancing");

  auto str_lb = std::string("Load balancing");
  auto ptr_lb = addFlag("vt_lb", Args::config.vt_lb, str_lb, str_lbGroup);

  auto str_lb_stats = std::string("Load balancing statistics");
  auto ptr_lb_stats = addFlag("vt_lb_stats",
    Args::config.vt_lb_stats, str_lb_stats, str_lbGroup);

  auto lb_quiet = std::string("Silence load balancing output");
  auto ptr_lb_quiet = addFlag("vt_lb_quiet",
    Args::config.vt_lb_quiet, lb_quiet, str_lbGroup);

  auto lb_file = std::string("Enable reading LB configuration from file");
  auto ptr_lb_file = addFlag("vt_lb_file",
    Args::config.vt_lb_file, lb_file, str_lbGroup);

  auto lb_file_name = std::string("LB configuration file to read");
  auto ptr_lb_file_name = addOption("vt_lb_file_name",
    Args::config.vt_lb_file_name, lb_file_name, str_lbGroup);

  auto lb_name = std::string("Name of the load balancer to use");
  auto ptr_lb_name = addOption("vt_lb_name",
    Args::config.vt_lb_name, lb_name, str_lbGroup);

  auto lb_interval = std::string("Load balancing interval");
  auto ptr_lb_interval = addOption("vt_lb_interval",
    Args::config.vt_lb_interval, lb_interval, str_lbGroup);

  auto lb_stats_dir = std::string("Load balancing statistics output directory");
  auto ptr_lb_stats_dir = addOption("vt_lb_stats_dir",
    Args::config.vt_lb_stats_dir, lb_stats_dir, str_lbGroup);

  auto lb_stats_file = std::string("Load balancing statistics output file name");
  auto ptr_lb_stats_file = addOption("vt_lb_stats_file",
    Args::config.vt_lb_stats_file, lb_stats_file, str_lbGroup);

#if !backend_check_enabled(lblite)
  ptr_lb->setBannerMsgWarning("lblite");
  ptr_lb_stats->setBannerMsgWarning("lblite");
#else
  ptr_lb->setPrintOption(PrintOption::always, nullptr);
  auto testFcnLB = [&]() { return (Args::config.vt_lb == true); };

  ptr_lb_quiet->setPrintOption(PrintOption::whenSet, testFcnLB);

  auto testFcnLBfile = [&]() {
    return ((Args::config.vt_lb) and (Args::config.vt_lb_file));
  };
  ptr_lb_file->setPrintOption(PrintOption::whenSet, testFcnLBfile);
  ptr_lb_file_name->setPrintOption(PrintOption::whenSet, testFcnLBfile);

  auto testFcnLBinput = [&]() {
    return ((Args::config.vt_lb) and (!Args::config.vt_lb_file));
  };
  ptr_lb_name->setPrintOption(PrintOption::whenSet, testFcnLBinput);
  ptr_lb_interval->setPrintOption(PrintOption::whenSet, testFcnLBinput);

  ptr_lb_stats->setPrintOption(PrintOption::always, nullptr);

  auto testFcnLBstats = [&]() { return Args::config.vt_lb_stats; };
  ptr_lb_stats_dir->setPrintOption(PrintOption::whenSet, testFcnLBstats);
  ptr_lb_stats_file->setPrintOption(PrintOption::whenSet, testFcnLBstats);
#endif

  //--- Constraints

  ptr_lb_quiet->needsOptionOn(ptr_lb);

  ptr_lb_file->needsOptionOn(ptr_lb);
  ptr_lb_file_name->needsOptionOn(ptr_lb);

  ptr_lb_interval->needsOptionOn(ptr_lb);
  ptr_lb_interval->needsOptionOff(ptr_lb_file);

  ptr_lb_name->needsOptionOn(ptr_lb);
  ptr_lb_name->needsOptionOff(ptr_lb_file);

  ptr_lb_stats_file->needsOptionOn(ptr_lb_stats);
  ptr_lb_stats_dir->needsOptionOn(ptr_lb_stats);
}

void ArgsManager::initializeUserOptions()
{
  //
  // User option flags for convenience; VT will parse these and the app can use
  // them however the apps requires
  //

  auto userOpts = std::string("User Options");

  auto user1 = std::string("User Option 1a (boolean)");
  auto u1 = addFlag("vt_user_1", Args::config.vt_user_1, user1, userOpts);
  u1->setPrintOption(PrintOption::whenSet, nullptr);

  auto testFcnUser1 = [&]() { return Args::config.vt_user_1; };

  auto userint1 = std::string("User Option 1b (int32_t)");
  auto ui1 = addOption("vt_user_int_1",
    Args::config.vt_user_int_1, userint1, userOpts);
  ui1->setPrintOption(PrintOption::whenSet, testFcnUser1);

  auto userstr1 = std::string("User Option 1c (std::string)");
  auto us1 = addOption("vt_user_str_1",
    Args::config.vt_user_str_1, userstr1, userOpts);
  us1->setPrintOption(PrintOption::whenSet, testFcnUser1);

  //---

  auto user2 = std::string("User Option 2a (boolean)");
  auto u2 = addFlag("vt_user_2", Args::config.vt_user_2, user2, userOpts);
  u2->setPrintOption(PrintOption::whenSet, nullptr);

  auto testFcnUser2 = [&]() { return Args::config.vt_user_2; };

  auto userint2 = std::string("User Option 2b (int32_t)");
  auto ui2 = addOption("vt_user_int_2",
    Args::config.vt_user_int_2, userint2, userOpts);
  ui2->setPrintOption(PrintOption::whenSet, testFcnUser2);

  auto userstr2 = std::string("User Option 2c (std::string)");
  auto us2 = addOption("vt_user_str_2",
    Args::config.vt_user_str_2, userstr2, userOpts);
  us2->setPrintOption(PrintOption::whenSet, testFcnUser2);

  //---

  auto user3 = std::string("User Option 3a (boolean)");
  auto u3 = addFlag("vt_user_3", Args::config.vt_user_3, user3, userOpts);
  u3->setPrintOption(PrintOption::whenSet, nullptr);

  auto testFcnUser3 = [&]() { return Args::config.vt_user_3; };

  auto userint3 = std::string("User Option 3b (int32_t)");
  auto ui3 = addOption("vt_user_int_3",
    Args::config.vt_user_int_3, userint3, userOpts);
  ui3->setPrintOption(PrintOption::whenSet, testFcnUser3);

  auto userstr3 = std::string("User Option 3c (std::string)");
  auto us3 = addOption("vt_user_str_3",
    Args::config.vt_user_str_3, userstr3, userOpts);
  us3->setPrintOption(PrintOption::whenSet, testFcnUser3);
}

void ArgsManager::initializeDebug()
{
  /*
   * Flags for controlling debug print output at runtime
   */
  std::unordered_map<
    config::CatEnum, std::tuple<bool*, std::string, std::string>>
    debug_flag;

  debug_flag[config::none] = std::make_tuple<bool*, std::string, std::string>(
    &Args::config.vt_debug_none, std::string("debug_none"),
    std::string(config::PrettyPrintCat<config::none>::str));
  debug_flag[config::gen] = std::make_tuple(
    &Args::config.vt_debug_gen, std::string("debug_gen"),
    std::string(config::PrettyPrintCat<config::gen>::str));
  debug_flag[config::runtime] = std::make_tuple(
    &Args::config.vt_debug_runtime, std::string("debug_runtime"),
    std::string(config::PrettyPrintCat<config::runtime>::str));
  debug_flag[config::active] = std::make_tuple(
    &Args::config.vt_debug_active, std::string("debug_active"),
    std::string(config::PrettyPrintCat<config::active>::str));
  debug_flag[config::term] = std::make_tuple(
    &Args::config.vt_debug_term, std::string("debug_term"),
    std::string(config::PrettyPrintCat<config::term>::str));
  debug_flag[config::termds] = std::make_tuple(
    &Args::config.vt_debug_termds, std::string("debug_term_ds"),
    std::string(config::PrettyPrintCat<config::termds>::str));
  debug_flag[config::barrier] = std::make_tuple(
    &Args::config.vt_debug_barrier, std::string("debug_barrier"),
    std::string(config::PrettyPrintCat<config::barrier>::str));
  debug_flag[config::event] = std::make_tuple(
    &Args::config.vt_debug_event, std::string("debug_event"),
    std::string(config::PrettyPrintCat<config::event>::str));
  debug_flag[config::pipe] = std::make_tuple(
    &Args::config.vt_debug_pipe, std::string("debug_pipe"),
    std::string(config::PrettyPrintCat<config::pipe>::str));
  debug_flag[config::pool] = std::make_tuple(
    &Args::config.vt_debug_pool, std::string("debug_pool"),
    std::string(config::PrettyPrintCat<config::pool>::str));
  debug_flag[config::reduce] = std::make_tuple(
    &Args::config.vt_debug_reduce, std::string("debug_reduce"),
    std::string(config::PrettyPrintCat<config::reduce>::str));
  debug_flag[config::rdma] = std::make_tuple(
    &Args::config.vt_debug_rdma, std::string("debug_rdma"),
    std::string(config::PrettyPrintCat<config::rdma>::str));
  debug_flag[config::rdma_channel] = std::make_tuple(
    &Args::config.vt_debug_rdma_channel, std::string("debug_rdma_channel"),
    std::string(config::PrettyPrintCat<config::rdma_channel>::str));
  debug_flag[config::rdma_state] = std::make_tuple(
    &Args::config.vt_debug_rdma_state, std::string("debug_rdma_state"),
    std::string(config::PrettyPrintCat<config::rdma_state>::str));
  debug_flag[config::param] = std::make_tuple(
    &Args::config.vt_debug_param, std::string("debug_param"),
    std::string(config::PrettyPrintCat<config::param>::str));
  debug_flag[config::handler] = std::make_tuple(
    &Args::config.vt_debug_handler, std::string("debug_handler"),
    std::string(config::PrettyPrintCat<config::handler>::str));
  debug_flag[config::hierlb] = std::make_tuple(
    &Args::config.vt_debug_hierlb, std::string("debug_hierlb"),
    std::string(config::PrettyPrintCat<config::hierlb>::str));
  debug_flag[config::scatter] = std::make_tuple(
    &Args::config.vt_debug_scatter, std::string("debug_scatter"),
    std::string(config::PrettyPrintCat<config::scatter>::str));
  debug_flag[config::sequence] = std::make_tuple(
    &Args::config.vt_debug_sequence, std::string("debug_sequence"),
    std::string(config::PrettyPrintCat<config::sequence>::str));
  debug_flag[config::sequence_vrt] = std::make_tuple(
    &Args::config.vt_debug_sequence_vrt, std::string("debug_sequence_vrt"),
    std::string(config::PrettyPrintCat<config::sequence_vrt>::str));
  debug_flag[config::serial_msg] = std::make_tuple(
    &Args::config.vt_debug_serial_msg, std::string("debug_serial_msg"),
    std::string(config::PrettyPrintCat<config::serial_msg>::str));
  debug_flag[config::trace] = std::make_tuple(
    &Args::config.vt_debug_trace, std::string("debug_trace"),
    std::string(config::PrettyPrintCat<config::trace>::str));
  debug_flag[config::location] = std::make_tuple(
    &Args::config.vt_debug_location, std::string("debug_location"),
    std::string(config::PrettyPrintCat<config::location>::str));
  debug_flag[config::lb] = std::make_tuple(
    &Args::config.vt_debug_lb, std::string("debug_lb"),
    std::string(config::PrettyPrintCat<config::lb>::str));
  debug_flag[config::vrt] = std::make_tuple(
    &Args::config.vt_debug_vrt, std::string("debug_vrt"),
    std::string(config::PrettyPrintCat<config::vrt>::str));
  debug_flag[config::vrt_coll] = std::make_tuple(
    &Args::config.vt_debug_vrt_coll, std::string("debug_vrt_coll"),
    std::string(config::PrettyPrintCat<config::vrt_coll>::str));
  debug_flag[config::worker] = std::make_tuple(
    &Args::config.vt_debug_worker, std::string("debug_worker"),
    std::string(config::PrettyPrintCat<config::worker>::str));
  debug_flag[config::group] = std::make_tuple(
    &Args::config.vt_debug_group, std::string("debug_group"),
    std::string(config::PrettyPrintCat<config::group>::str));
  debug_flag[config::broadcast] = std::make_tuple(
    &Args::config.vt_debug_broadcast, std::string("debug_broadcast"),
    std::string(config::PrettyPrintCat<config::broadcast>::str));
  debug_flag[config::objgroup] = std::make_tuple(
    &Args::config.vt_debug_objgroup, std::string("debug_objgroup"),
    std::string(config::PrettyPrintCat<config::objgroup>::str));

  auto debugGroup =
    std::string("Debug Print Configuration (must be compile-time enabled)");

  auto ptr_all = addFlag("vt_debug_all",
    Args::config.vt_debug_all, "Enable all debug prints", debugGroup);
  ptr_all->setPrintOption(PrintOption::whenSet, nullptr);

  auto ptr_verbose = addFlag("vt_debug_verbose",
    Args::config.vt_debug_verbose, "Enable verbose debug prints",
    debugGroup);
  ptr_verbose->setPrintOption(PrintOption::whenSet, nullptr);

  for (const auto &dcase : debug_flag) {
    auto my_flag = dcase.second;
    auto my_name = std::get<1>(my_flag);
    auto my_desc = std::string("Enable ") + my_name + std::string(" = \"") +
                   std::get<2>(my_flag) + std::string("\"");
    auto& my_status = *(std::get<0>(my_flag));
    auto my_vt_name = std::string("vt_") + my_name;
    auto my_ptr = addFlag(my_vt_name, my_status, my_desc, debugGroup);
    my_ptr->setPrintOption(PrintOption::whenSet, nullptr);
    if (!((vt::config::DefaultConfig::category & dcase.first) != 0)) {
      if (my_status)
        my_ptr->setBannerMsgWarning(my_name);
    }
  }
}

template <typename T, typename _unused>
std::shared_ptr<Anchor<T>> ArgsManager::addFlag(
  const std::string& name, T& anchor_value, const std::string& desc,
  const std::string& grp
) {
  std::string sname;
  try {
    sname = verifyName(name);
  } catch (const std::exception& e) {
    throw;
  }

  auto iter = options_.find(sname);
  if (iter == options_.end()) {
    // @todo: stop using default for warn_override and get this from runtime?
    // @todo: Ask JL about this comment
    auto anchor = std::make_shared<Anchor<T>>(anchor_value, sname, desc, grp);
    options_[sname] = anchor;
    auto aptr = anchor.get();
    // Insert the flag into CLI app
    addFlagToCLI<T>(aptr, sname, anchor_value, desc);
    //
    return anchor;
  } else {
    auto base = iter->second;
    auto anchor = std::static_pointer_cast<Anchor<T>>(base);
    return anchor;
  }
}

template <>
void ArgsManager::addFlagToCLI<bool>(
  Anchor<bool>* aptr, const std::string& sname, bool& anchor_value,
  const std::string& desc
) {
  CLI::callback_t fun = [aptr, &anchor_value](const CLI::results_t& res) {
    anchor_value = true;
    aptr->addGeneralInstance(ContextEnum::commandLine, anchor_value);
    return (res.size() == 1);
  };
  std::string cli_name = std::string("--") + sname;
  auto opt = app_->add_option(cli_name, fun, desc);
  opt->type_size(0);
  opt->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
  opt->type_name(CLI::detail::type_name<bool>());
  //
  std::stringstream out;
  out << anchor_value;
  opt->default_str(out.str());
}


template <>
void ArgsManager::addFlagToCLI<int>(
  Anchor<int>* aptr, const std::string& sname, int& anchor_value,
  const std::string& desc
) {
  CLI::callback_t fun = [aptr, &anchor_value](const CLI::results_t& res) {
    anchor_value = static_cast<int>(res.size());
    aptr->addGeneralInstance(ContextEnum::commandLine, anchor_value);
    return true;
  };
  std::string cli_name = std::string("--") + sname;
  auto opt = app_->add_option(cli_name, fun, desc);
  opt->type_size(0);
  opt->type_name(CLI::detail::type_name<int>());
  //
  std::stringstream out;
  out << anchor_value;
  opt->default_str(out.str());
}

template <typename T>
std::shared_ptr<Anchor<T>> ArgsManager::addOption(
  const std::string& name, T& anchor_value, const std::string& desc,
  const std::string& grp
) {
  std::string sname;
  try {
    sname = verifyName(name);
  } catch (const std::exception& e) {
    throw;
  }
  auto iter = options_.find(sname);
  if (iter == options_.end()) {
    // @todo: stop using default for warn_override and get this from runtime?
    // @todo: Ask JL
    auto anchor = std::make_shared<Anchor<T>>(anchor_value, sname, desc, grp);
    options_[sname] = anchor;
    auto aptr = anchor.get();
    //
    // Insert the option into CLI app
    //
    CLI::callback_t fun = [aptr, &anchor_value](CLI::results_t res) {
      bool myFlag = CLI::detail::lexical_cast(res[0], anchor_value);
      aptr->addGeneralInstance(ContextEnum::commandLine, anchor_value);
      return myFlag;
    };
    std::string cli_name = std::string("--") + sname;
    auto opt = app_->add_option(cli_name, fun, desc, true);
    opt->type_name(CLI::detail::type_name<T>());
    //
    std::stringstream out;
    out << anchor_value;
    opt->default_str(out.str());
    //
    return anchor;
  } else {
    auto base = iter->second;
    auto anchor = std::static_pointer_cast<Anchor<T>>(base);
    return anchor;
  }
}

std::vector<std::string> ArgsManager::getGroupList() const
{
  std::vector<std::string> groups;
  for (const auto& opt : options_) {
    // Add group if it is not already in there
    auto gval = opt.second->getGroup();
    if (std::find(groups.begin(), groups.end(), gval) == groups.end()) {
      groups.push_back(gval);
    }
  }
  std::sort(groups.begin(), groups.end());
  return groups;
}

std::map<std::string, std::shared_ptr<AnchorBase>> ArgsManager::getGroupOptions(
  const std::string& gname
) const {
  std::map<std::string, std::shared_ptr<AnchorBase>> options;
  for (const auto& opt : options_) {
    // Add group if it is not already in there
    auto gval = opt.second->getGroup();
    if (gval == gname) {
      options[opt.first] = opt.second;
    }
  }
  return options;
}

template<typename T>
std::shared_ptr<Anchor<T>> ArgsManager::getOption(const std::string &name) {
  std::string sname;
  try {
    sname = verifyName(name);
  } catch (const std::exception& e) {
    throw;
  }
  auto iter = options_.find(sname);
  if (iter == options_.end()) {
    std::string code = std::string("ArgSetup::getOption") + std::string("::") +
                       std::string(" Name ") + sname
                       + std::string(" Does Not Exist ");
    throw std::runtime_error(code);
  }
  auto base = iter->second;
  auto anchor = std::static_pointer_cast<Anchor<T>>(base);
  return std::shared_ptr<Anchor<T>>(anchor);
}

std::string ArgsManager::outputConfig(
  bool default_also, bool write_description, std::string prefix
) const {
  auto groupList = getGroupList();
  std::stringstream out;
  for (const auto& gname : groupList) {
    auto goptions = getGroupOptions(gname);
    for (const auto& opt : goptions) {
      std::string name = prefix + opt.first;
      std::string value;
      //
      auto option = opt.second;
      //
      if (static_cast<int>(out.tellp()) != 0)
        out << std::endl;
      //
      out << "; Group [" << option->getGroup() << "]" << std::endl;
      //
      if (write_description) {
        out << "; " << CLI::detail::fix_newlines("; ", option->getDescription())
            << std::endl;
      }
      //
      if (default_also) {
        out << "; Default Value = " << option->stringifyDefault() << std::endl;
      }
      //
      if (option->isResolved()) {
        out << "; Specified by " << option->stringifyContext() << std::endl;
      } else {
        out << "; Option/Flag Not Resolved " << std::endl;
      }
      //
      out << option->getName() << " = " << option->stringifyValue();
      out << std::endl;
      //
    }
  }
  return out.str();
}

void ArgsManager::parse(int &argc, char**& argv)
{
  // CLI11 app parser expects to get the arguments in *reverse* order!
  std::vector<std::string> args;
  for (int i = argc - 1; i > 0; i--) {
    args.emplace_back(argv[i]);
  }

  /*
   * Run the parser!
   */
  app_->allow_extras(true);
  try {
    app_->parse(args);
  } catch (CLI::Error& ex) {
    app_->exit(ex);
  }

  // Determine the final colorization setting.
  if (Args::config.vt_no_color) {
    Args::config.colorize_output = false;
  } else {
    // Otherwise, colorize.
    // (Within MPI there is no good method to auto-detect.)
    Args::config.colorize_output = true;
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
  size_t new_argc = ret_args.size() + 1;
  char** new_argv = new char*[new_argc + 1];
  new_argv[0] = argv[0];
  for (size_t ii = 1; ii < new_argc; ii++) {
    new_argv[ii] = argv[ret_idx[ii - 1]];
  }

  // Set them back with all vt arguments elided
  argc = static_cast<int>(new_argc);
  argv = new_argv;
}

void ArgsManager::postParsingReview()
{
#if backend_check_enabled(trace_enabled)
  //--- Overwrite the default trace file and directories
  //--- when they are the empty default values.
  auto ptr_trace_file = getOption<std::string>("vt_trace_file");
  if (ptr_trace_file->getValue().empty()) {
    auto tracePtr = vt::theTrace();
    if (tracePtr) {
      ptr_trace_file->setNewDefaultValue(tracePtr->getTraceName());
      ptr_trace_file->resetToDefault();
    }
  }
  //
  auto ptr_trace_dir = getOption<std::string>("vt_trace_dir");
  if (ptr_trace_dir->getValue().empty()) {
    auto tracePtr = vt::theTrace();
    if (tracePtr) {
      ptr_trace_dir->setNewDefaultValue(tracePtr->getDirectory());
      ptr_trace_dir->resetToDefault();
    }
  }
#endif
}

template <typename T>
std::shared_ptr<Anchor<T>>
ArgsManager::setNewDefaultValue(const std::string& name, const T& anchor_value
) {
  auto optPtr = getOption<T>(name);
  if (optPtr == nullptr) {
    std::string code = std::string("ArgSetup::setNewDefaultValue")
                       + std::string(" Name ") + name
                       + std::string(" Can Not Be Found ");
    throw std::runtime_error(code);
  }
  optPtr->setNewDefaultValue(anchor_value);
  return std::shared_ptr<Anchor<T>>(optPtr);
}


/* ------------------------------------------------- */
// --- Member functions for struct Args
/* ------------------------------------------------- */


/*static*/
void Args::parseAndResolve(int& argc, char**& argv)
{
  checkInitialization();
  try {
    Args::manager_->parse(argc, argv);
    Args::parsed_ = true;
    Args::manager_->resolveOptions();
  } catch (const std::exception& e) {
    throw;
  }
}

/*static*/ template < typename T, typename unused>
std::shared_ptr<Anchor<T>> Args::addFlag(
  const std::string& name, T& anchor_value, const std::string& desc,
  const std::string& grp
) {
  checkInitialization();
  return Args::manager_->addFlag<T>(name, anchor_value, desc, grp);
}

/*static*/ template <typename T>
std::shared_ptr<Anchor<T>> Args::addOption(
  const std::string& name, T& anchor_value, const std::string& desc,
  const std::string& grp
) {
  checkInitialization();
  return Args::manager_->addOption<T>(name, anchor_value, desc, grp);
}

/*static*/
void Args::checkInitialization()
{
  if (!Args::manager_) {
    Args::manager_ = std::make_unique<ArgsManager>("vt");
  }
}

/*static*/ template<typename T>
std::shared_ptr<Anchor<T>> Args::getOption(const std::string &name)
{
  checkInitialization();
  return Args::manager_->getOption<T>(name);
}


/*static*/ template <typename T>
std::shared_ptr<Anchor<T>>
Args::setNewDefaultValue(const std::string& name, const T& anchor_value
) {
  checkInitialization();
  return Args::manager_->setNewDefaultValue<T>(name, anchor_value);
}

/*static*/
std::string Args::outputConfig(
  bool default_also, bool write_description, std::string prefix
) {
  checkInitialization();
  return Args::manager_->outputConfig(default_also, write_description,
    std::move(prefix));
}

/*static*/
void Args::printBanner() {
  if (!parsed_) {
    checkInitialization();
    std::string msg("Parsing not Called. Will print default values.");
    vtWarn(msg);
  }
  //
  Args::manager_->postParsingReview();
  //
  auto groupList = Args::manager_->getGroupList();
  for (const auto& gname : groupList) {
    auto goptions = Args::manager_->getGroupOptions(gname);
    for (const auto& opt : goptions) {
      opt.second->print();
    }
  }
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
  PrintOption po, std::function<bool()> fun
) {
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
                       std::string("::") + std::string(" Default for ")
                       + smap_[ctxt] + std::string(" Can Not Be Added ");
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
