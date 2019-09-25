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


//--- Initialization of static variables for vt::AnchorBase
/* static */ std::map<const context, const AnchorBase::orderCtxt> AnchorBase::emap_ =
         {{std::make_pair(context::dFault, AnchorBase::orderCtxt::dFault),
            std::make_pair(context::commandLine, AnchorBase::orderCtxt::commandLine),
            std::make_pair(context::thirdParty, AnchorBase::orderCtxt::thirdParty)
         }};

/* static */ std::map<const context, const std::string> AnchorBase::smap_ =
   {{std::make_pair(context::dFault, "Default Value"),
       std::make_pair(context::commandLine, "Command Line"),
       std::make_pair(context::thirdParty, "Third-Party Context")
    }};


//--- Initialization of static variables for vt::AnchorBase
/*static*/ ArgSetup Args::setup_ = {"vt"};
/*static*/ Configs Args::config = {};


/*static*/ void Args::initialize()
{

  /*
   * Flags for controlling the colorization of output from vt
   */
  auto quiet  = "Quiet the output from vt (only errors, warnings)";
  auto always = "Always colorize output";
  auto never  = "Never colorize output";
  auto maybe  = "Use isatty to determine colorization of output";
  auto a  = setup_.addFlag("vt_color",      config.vt_color,      always);
  auto b  = setup_.addFlag("vt_no_color",   config.vt_no_color,   never);
  auto c  = setup_.addFlag("vt_auto_color", config.vt_auto_color, maybe);
  auto a1 = setup_.addFlag("vt_quiet",      config.vt_quiet,      quiet);
  auto outputGroup = "Output Control";
  a->setGroup(outputGroup);
  b->setGroup(outputGroup);
  c->setGroup(outputGroup);
  a1->setGroup(outputGroup);
  b->excludes(a);
  b->excludes(c);

  /*
   * Flags for controlling the signals that VT tries to catch
   */
  auto no_sigint      = "Do not register signal handler for SIGINT";
  auto no_sigsegv     = "Do not register signal handler for SIGSEGV";
  auto no_terminate   = "Do not register handler for std::terminate";
  auto d = setup_.addFlag("vt_no_SIGINT",    config.vt_no_sigint,    no_sigint);
  auto e = setup_.addFlag("vt_no_SIGSEGV",   config.vt_no_sigsegv,   no_sigsegv);
  auto f = setup_.addFlag("vt_no_terminate", config.vt_no_terminate, no_terminate);
  auto signalGroup = "Signa Handling";
  d->setGroup(signalGroup);
  e->setGroup(signalGroup);
  f->setGroup(signalGroup);


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
  auto g = setup_.addFlag("vt_no_warn_stack",   config.vt_no_warn_stack,   warn);
  auto h = setup_.addFlag("vt_no_assert_stack", config.vt_no_assert_stack, assert);
  auto i = setup_.addFlag("vt_no_abort_stack",  config.vt_no_abort_stack,  abort);
  auto j = setup_.addFlag("vt_no_stack",        config.vt_no_stack,        stack);
  auto k = setup_.addOption("vt_stack_file",    config.vt_stack_file,      file);
  auto l = setup_.addOption("vt_stack_dir",     config.vt_stack_dir,       dir);
  auto m = setup_.addOption("vt_stack_mod",     config.vt_stack_mod,       mod);
  auto stackGroup = "Dump Stack Backtrace";
  g->setGroup(stackGroup);
  h->setGroup(stackGroup);
  i->setGroup(stackGroup);
  j->setGroup(stackGroup);
  k->setGroup(stackGroup);
  l->setGroup(stackGroup);
  m->setGroup(stackGroup);


  /*
   * Flags to control tracing output
   */
  auto trace  = "Enable tracing (must be compiled with trace_enabled)";
  auto tfile  = "Name of trace files";
  auto tdir   = "Name of directory for trace files";
  auto tmod   = "Output trace file if (node % vt_stack_mod) == 0";
  auto n = setup_.addFlag("vt_trace",        config.vt_trace,       trace);
  auto o = setup_.addOption("vt_trace_file", config.vt_trace_file,  tfile);
  auto p = setup_.addOption("vt_trace_dir",  config.vt_trace_dir,   tdir);
  auto q = setup_.addOption("vt_trace_mod",  config.vt_trace_mod,   tmod);
  auto traceGroup = "Tracing Configuration";
  n->setGroup(traceGroup);
  o->setGroup(traceGroup);
  p->setGroup(traceGroup);
  q->setGroup(traceGroup);


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

  auto r  = setup_.addFlag("vt_debug_all",          config.vt_debug_all,          rp);
  auto r1 = setup_.addFlag("vt_debug_verbose",      config.vt_debug_verbose,      rq);
  auto aa = setup_.addFlag("vt_debug_none",         config.vt_debug_none,         aap);
  auto ba = setup_.addFlag("vt_debug_gen",          config.vt_debug_gen,          bap);
  auto ca = setup_.addFlag("vt_debug_runtime",      config.vt_debug_runtime,      cap);
  auto da = setup_.addFlag("vt_debug_active",       config.vt_debug_active,       dap);
  auto ea = setup_.addFlag("vt_debug_term",         config.vt_debug_term,         eap);
  auto fa = setup_.addFlag("vt_debug_termds",       config.vt_debug_termds,       fap);
  auto ga = setup_.addFlag("vt_debug_barrier",      config.vt_debug_barrier,      gap);
  auto ha = setup_.addFlag("vt_debug_event",        config.vt_debug_event,        hap);
  auto ia = setup_.addFlag("vt_debug_pipe",         config.vt_debug_pipe,         iap);
  auto ja = setup_.addFlag("vt_debug_pool",         config.vt_debug_pool,         jap);
  auto ka = setup_.addFlag("vt_debug_reduce",       config.vt_debug_reduce,       kap);
  auto la = setup_.addFlag("vt_debug_rdma",         config.vt_debug_rdma,         lap);
  auto ma = setup_.addFlag("vt_debug_rdma_channel", config.vt_debug_rdma_channel, map);
  auto na = setup_.addFlag("vt_debug_rdma_state",   config.vt_debug_rdma_state,   nap);
  auto oa = setup_.addFlag("vt_debug_param",        config.vt_debug_param,        oap);
  auto pa = setup_.addFlag("vt_debug_handler",      config.vt_debug_handler,      pap);
  auto qa = setup_.addFlag("vt_debug_hierlb",       config.vt_debug_hierlb,       qap);
  auto ra = setup_.addFlag("vt_debug_scatter",      config.vt_debug_scatter,      rap);
  auto sa = setup_.addFlag("vt_debug_sequence",     config.vt_debug_sequence,     sap);
  auto ta = setup_.addFlag("vt_debug_sequence_vrt", config.vt_debug_sequence_vrt, tap);
  auto ua = setup_.addFlag("vt_debug_serial_msg",   config.vt_debug_serial_msg,   uap);
  auto va = setup_.addFlag("vt_debug_trace",        config.vt_debug_trace,        vap);
  auto wa = setup_.addFlag("vt_debug_location",     config.vt_debug_location,     wap);
  auto xa = setup_.addFlag("vt_debug_lb",           config.vt_debug_lb,           xap);
  auto ya = setup_.addFlag("vt_debug_vrt",          config.vt_debug_vrt,          yap);
  auto za = setup_.addFlag("vt_debug_vrt_coll",     config.vt_debug_vrt_coll,     zap);
  auto ab = setup_.addFlag("vt_debug_worker",       config.vt_debug_worker,       abp);
  auto bb = setup_.addFlag("vt_debug_group",        config.vt_debug_group,        bbp);
  auto cb = setup_.addFlag("vt_debug_broadcast",    config.vt_debug_broadcast,    cbp);
  auto db = setup_.addFlag("vt_debug_objgroup",     config.vt_debug_objgroup,     dbp);
  auto debugGroup = "Debug Print Configuration (must be compile-time enabled)";
  r->setGroup(debugGroup);
  r1->setGroup(debugGroup);
  aa->setGroup(debugGroup);
  ba->setGroup(debugGroup);
  ca->setGroup(debugGroup);
  da->setGroup(debugGroup);
  ea->setGroup(debugGroup);
  fa->setGroup(debugGroup);
  ga->setGroup(debugGroup);
  ha->setGroup(debugGroup);
  ia->setGroup(debugGroup);
  ja->setGroup(debugGroup);
  ka->setGroup(debugGroup);
  la->setGroup(debugGroup);
  ma->setGroup(debugGroup);
  na->setGroup(debugGroup);
  oa->setGroup(debugGroup);
  pa->setGroup(debugGroup);
  qa->setGroup(debugGroup);
  ra->setGroup(debugGroup);
  sa->setGroup(debugGroup);
  ta->setGroup(debugGroup);
  ua->setGroup(debugGroup);
  va->setGroup(debugGroup);
  xa->setGroup(debugGroup);
  wa->setGroup(debugGroup);
  ya->setGroup(debugGroup);
  za->setGroup(debugGroup);
  ab->setGroup(debugGroup);
  bb->setGroup(debugGroup);
  cb->setGroup(debugGroup);
  db->setGroup(debugGroup);

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
  auto s  = setup_.addFlag("vt_lb",              config.vt_lb,            lb);
  auto t  = setup_.addFlag("vt_lb_file",         config.vt_lb_file,       lb_file);
  auto t1 = setup_.addFlag("vt_lb_quiet",        config.vt_lb_quiet,      lb_quiet);
  auto u  = setup_.addOption("vt_lb_file_name",  config.vt_lb_file_name,  lb_file_name);
  auto v  = setup_.addOption("vt_lb_name",       config.vt_lb_name,       lb_name);
  auto w  = setup_.addOption("vt_lb_interval",   config.vt_lb_interval,   lb_interval);
  auto ww = setup_.addFlag("vt_lb_stats",        config.vt_lb_stats,      lb_stats);
  auto wx = setup_.addOption("vt_lb_stats_dir",  config.vt_lb_stats_dir,  lb_stats_dir);
  auto wy = setup_.addOption("vt_lb_stats_file", config.vt_lb_stats_file, lb_stats_file);
  auto debugLB = "Load Balancing";
  s->setGroup(debugLB);
  t->setGroup(debugLB);
  t1->setGroup(debugLB);
  u->setGroup(debugLB);
  v->setGroup(debugLB);
  w->setGroup(debugLB);
  ww->setGroup(debugLB);
  wx->setGroup(debugLB);
  wy->setGroup(debugLB);

  /*
   * Flags for controlling termination
   */

  auto hang         = "Disable termination hang detection";
  auto hang_freq    = "The number of tree traversals before a hang is detected";
  auto ds           = "Force use of Dijkstra-Scholten (DS) algorithm for rooted epoch termination detection";
  auto wave         = "Force use of 4-counter algorithm for rooted epoch termination detection";
  auto x  = setup_.addFlag("vt_no_detect_hang",       config.vt_no_detect_hang,       hang);
  auto x1 = setup_.addFlag("vt_term_rooted_use_ds",   config.vt_term_rooted_use_ds,   ds);
  auto x2 = setup_.addFlag("vt_term_rooted_use_wave", config.vt_term_rooted_use_wave, wave);
  auto y = setup_.addOption("vt_hang_freq",           config.vt_hang_freq, hang_freq);
  auto debugTerm = "Termination";
  x->setGroup(debugTerm);
  x1->setGroup(debugTerm);
  x2->setGroup(debugTerm);
  y->setGroup(debugTerm);

  /*
   * Flags for controlling termination
   */

  auto pause        = "Pause at startup so GDB/LLDB can be attached";
  auto z = setup_.addFlag("vt_pause", config.vt_pause, pause);
  auto launchTerm = "Debugging/Launch";
  z->setGroup(launchTerm);

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
  auto u1  = setup_.addFlag("vt_user_1", config.vt_user_1, user1);
  auto u2  = setup_.addFlag("vt_user_2", config.vt_user_2, user2);
  auto u3  = setup_.addFlag("vt_user_3", config.vt_user_3, user3);
  auto ui1 = setup_.addOption("vt_user_int_1", config.vt_user_int_1, userint1);
  auto ui2 = setup_.addOption("vt_user_int_2", config.vt_user_int_2, userint2);
  auto ui3 = setup_.addOption("vt_user_int_3", config.vt_user_int_3, userint3);
  auto us1 = setup_.addOption("vt_user_str_1", config.vt_user_str_1, userstr1);
  auto us2 = setup_.addOption("vt_user_str_2", config.vt_user_str_2, userstr2);
  auto us3 = setup_.addOption("vt_user_str_3", config.vt_user_str_3, userstr3);
  auto userOpts = "User Options";
  u1->setGroup(userOpts);
  u2->setGroup(userOpts);
  u3->setGroup(userOpts);
  ui1->setGroup(userOpts);
  ui2->setGroup(userOpts);
  ui3->setGroup(userOpts);
  us1->setGroup(userOpts);
  us2->setGroup(userOpts);
  us3->setGroup(userOpts);

}


/* ------------------------------------------------- */

//--- Utility functions
template< typename T>
std::string display(const T &val) { std::string res; return res; }

template<>
std::string display<bool>(const bool &val) {
   return val ? std::string("T") : std::string("F");
}

template<>
std::string display<int>(const int &val) {
   return std::to_string(val);
}

template<>
std::string display<std::string>(const std::string &val) {
   std::string res = std::string("\"") + val + std::string("\"");
   return res;
}

/* ------------------------------------------------- */

template< typename T >
std::string Anchor<T>::valueToString() const {
   std::string val;
   if (!isResolved_)
      return val;
   //
   val = display<T>(value_);
   //
   return val;
}

template< typename T>
std::string Anchor<T>::valueContext() const {
   std::string val;
   if (!isResolved_)
      return val;
   return smap_[resolved_ctxt_];
}

template< typename T>
std::string Anchor<T>::valueDefault() const {
   std::string val;
   for (auto item : specifications_) {
      if (item.first == context::dFault) {
         val = display<T>(item.second.getValue());
         break;
      }
   }
   return val;
}

/* ------------------------------------------------- */

template< typename T>
void Anchor<T>::addGeneralInstance(
      context ctxt,
      const T &value)
{
   //---
   // Does not allow to specify a 'dFault' instance
   if (ctxt == context::dFault) {
      std::string code = std::string(__func__)
                         + std::string("::")
                         + std::string(" Default for ") + smap_[ctxt]
                         + std::string(" Can Not Be Added ");
      throw std::runtime_error(code);
   }
   //---
   if (specifications_.count(ctxt) > 0) {
      std::string code = std::string(__func__)
                         + std::string(" Context ") + smap_[ctxt]
                         + std::string(" Already Inserted ");
      throw std::runtime_error(code);
   }
   //----
   auto myCase = Instance<T>(value, this);
   specifications_.insert(std::make_pair(ctxt, myCase));
   if ((ordering_.count(ctxt) == 0) || (ordering_[ctxt] != orderCtxt::MAX))
      ordering_[ctxt] = emap_[ctxt];
}

/* ------------------------------------------------- */

template< typename T>
void Anchor<T>::checkExcludes() const {
   for(auto opt_ex : excludes_) {
      // Check whether the 'excluded' options have specifications
      // in addition to their default ones.
      if ((opt_ex->count() > 1) && (this->count() > 1)) {
         std::string code = std::string(__func__)
                            + std::string("::") + name_
                            + std::string(" excludes ")
                            + opt_ex->getName();
         throw std::runtime_error(code);
      }
   }
}

/* ------------------------------------------------- */

template< typename T>
void Anchor<T>::resolve() {
   if (isResolved_)
      return;
   //
   try {
      checkExcludes();
   }
   catch (const std::exception &e) {
      throw;
   }
   //
   int cmax = 0;
   context resolved_ctxt = context::dFault;
   for (const auto &iter : specifications_) {
      auto my_ctxt = iter.first;
      if ((ordering_.count(my_ctxt) > 0) &&
          (static_cast<int>(ordering_[my_ctxt]) > cmax)) {
         resolved_ctxt = my_ctxt;
         resolved_instance_ = iter.second;
         cmax = static_cast<int>(ordering_[my_ctxt]);
      }
   }
   //
   value_ = resolved_instance_.getValue();
   if ((cmax <= 1) || (resolved_ctxt == context::dFault))
      resolved_to_default_ = true;
   //
   isResolved_ = true;
}

/* ------------------------------------------------- */

std::string ArgSetup::verifyName(const std::string &name) const
{
  int ipos = 0;
  while (name[ipos] == '-') { ipos++; }
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


template <typename T>
std::shared_ptr<Anchor<T>> ArgSetup::addOption(
    const std::string &name,
    T& anchor_value,
    const std::string &desc
)
{
   std::string sname;
   try {
      sname = verifyName(name);
   }
   catch (const std::exception &e) {
      throw;
   }
   auto iter = options_.find(sname);
   if (iter == options_.end()) {
      // @todo: stop using default for warn_override and get this from runtime?
      auto anchor = std::make_shared<Anchor<T> >(anchor_value, sname, desc);
      options_[sname] = anchor;
      //
      // Insert the option into CLI app
      //
      CLI::callback_t fun = [&anchor,&anchor_value](CLI::results_t res) {
         bool myFlag = CLI::detail::lexical_cast(res[0], anchor_value);
         anchor->addGeneralInstance(context::commandLine, anchor_value);
         return myFlag;
      };
      std::string cli_name = std::string("--") + sname;
      auto opt = app_.add_option(cli_name, fun, desc, true);
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


/* ------------------------------------------------- */


std::string ArgSetup::to_config(
  bool default_also,
  bool write_description,
  std::string prefix
) const
{
   std::stringstream out;
   for (const auto &opt : options_) {
      std::string name = prefix + opt.first;
      std::string value;
      //
      auto option = opt.second;
      //
      if (static_cast<int>(out.tellp()) != 0)
         out << std::endl;
      //
      if (write_description) {
         out << "; "
             << CLI::detail::fix_newlines("; ", option->getDescription())
             << std::endl;
      }
      //
      out << "; Group [" << option->getGroup() << "]" << std::endl;
      //
      if (default_also) {
         out << "; Default Value = " << option->valueDefault()
            << std::endl;
      }
      //
      if (option->isResolved()) {
          out << "; Specified by " << option->valueContext() << std::endl;
      }
      //
      out << option->getName() << " = " << option->valueToString();
      out << std::endl;
      //
   }
   return out.str();
}


/* ------------------------------------------------- */


template <typename T>
std::shared_ptr<Anchor<T>> ArgSetup::setNewDefaultValue(
   const std::string &name,
   const T& anchor_value
) {
   auto optPtr = this->getOption<T>(name);
   if (optPtr == nullptr) {
      std::string code = std::string(__func__)
      + std::string(" Name ") + name
      + std::string(" Can Not Be Found ");
      throw std::runtime_error(code);
   }
   optPtr->setNewDefaultValue(anchor_value);
   return optPtr;
}


/* ------------------------------------------------- */


int ArgSetup::parse(int& argc, char**& argv)
{

  if (parsed)
    return 0;

  std::vector<std::string> args;
  for (auto i = 0; i < argc; i++) {
    args.push_back(std::string(argv[i]));
  }

  // fmt::print("argc={}, argv={}\n", argc, print_ptr(argv));

  /*
   * Run the parser!
   */
  app_.allow_extras(true);
  try {
    app_.parse(args);
  } catch (CLI::Error &ex) {
    return app_.exit(ex);
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

  return 0;

}


}} /* end namespace vt::arguments */
