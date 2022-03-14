/*
//@HEADER
// *****************************************************************************
//
//                              runtime_banner.cc
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

#include "vt/runtime/runtime.h"

#include "vt/config.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/configs/generated/vt_git_revision.h"
#include "vt/utils/mpi_limits/mpi_max_tag.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/balance/read_lb.h"
#include "vt/utils/memory/memory_usage.h"
#include "vt/utils/memory/memory_units.h"
#include "vt/scheduler/scheduler.h"

#include <memory>
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <sstream>
#include <unistd.h>

#include <mpi.h>

namespace vt { namespace runtime {

void Runtime::printStartupBanner() {
  // If --vt_quiet is set, immediately exit printing nothing during startup
  if (getAppConfig()->vt_quiet) {
    return;
  }

  NodeType const nodes = theContext->getNumNodes();
  WorkerCountType const workers = theContext->getNumWorkers();
  bool const has_workers = theContext->hasWorkers();

  std::string is_interop_str =
    is_interop_ ?
      std::string(" interop=") + std::string(is_interop_ ? "true:" : "false:") :
      std::string("");
  std::string init = "Runtime Initializing:" + is_interop_str;
  std::string mode = std::string("mode: ");
  std::string mode_type =
    std::string(num_workers_ == no_workers ? "single" : "multi") +
    std::string("-thread per rank");
  std::string thd = !has_workers ? std::string("") :
    std::string(", worker threading: ") +
    std::string(
      #if vt_check_enabled(openmp)
        "OpenMP"
      #elif vt_check_enabled(stdthread)
        "std::thread"
      #else
        ""
      #endif
   );
  std::string cnt = !has_workers ? std::string("") :
    (std::string(", ") + std::to_string(workers) + std::string(" workers/node"));
  std::string node_str = nodes == 1 ? "node" : "nodes";
  std::string all_node = std::to_string(nodes) + " " + node_str + cnt;

  char hostname[1024];
  gethostname(hostname, 1024);

  auto green    = debug::green();
  auto red      = debug::red();
  auto reset    = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta  = debug::magenta();
  auto vt_pre   = debug::vtPre();
  auto emph     = [=](std::string s) -> std::string { return debug::emph(s); };
  auto reg      = [=](std::string s) -> std::string { return debug::reg(s);  };

  std::vector<std::string> features;

#if vt_check_enabled(bit_check_overflow)
  features.push_back(vt_feature_str_bit_check_overflow);
#endif
#if vt_check_enabled(trace_enabled)
  features.push_back(vt_feature_str_trace_enabled);
#endif
#if vt_check_enabled(lblite)
  features.push_back(vt_feature_str_lblite);
#endif
#if vt_check_enabled(openmp)
  features.push_back(vt_feature_str_openmp);
#endif
#if vt_check_enabled(production_build)
  features.push_back(vt_feature_str_production_build);
#endif
#if vt_check_enabled(priorities)
  features.push_back(vt_feature_str_priorities);
#endif
#if vt_check_enabled(fcontext)
  features.push_back(vt_feature_str_fcontext);
#endif
#if vt_check_enabled(stdthread)
  features.push_back(vt_feature_str_stdthread);
#endif
#if vt_check_enabled(mpi_rdma)
  features.push_back(vt_feature_str_mpi_rdma);
#endif
#if vt_check_enabled(print_term_msgs)
  features.push_back(vt_feature_str_print_term_msgs);
#endif
#if vt_check_enabled(no_pool_alloc_env)
  features.push_back(vt_feature_str_no_pool_alloc_env);
#endif
#if vt_check_enabled(memory_pool)
  features.push_back(vt_feature_str_memory_pool);
#endif
#if vt_check_enabled(mpi_access_guards)
  features.push_back(vt_feature_str_mpi_access_guards);
#endif
#if vt_check_enabled(zoltan)
  features.push_back(vt_feature_str_zoltan);
#endif
#if vt_check_enabled(mimalloc)
  features.push_back(vt_feature_str_mimalloc);
#endif
#if vt_check_enabled(diagnostics)
  features.push_back(vt_feature_str_diagnostics);
#endif

  std::string dirty = "";
  if (vt_git_clean_status == "DIRTY") {
    dirty = red + std::string("*dirty*") + reset;
  }

  auto const max_tag = util::MPI_Attr::getMaxTag();
  auto const max_tag_str = std::to_string(max_tag);
  auto const version_tuple = util::MPI_Attr::getVersion();
  auto const version = std::to_string(std::get<0>(version_tuple));
  auto const subversion = std::to_string(std::get<1>(version_tuple));

  auto vt_version_string = fmt::format("{}.{}.{}", vt_version_major, vt_version_minor, vt_version_patch);
  std::array< std::string, 11 > info_lines = {
      fmt::format("{}Version: {}\n", green, emph(vt_version_string)),

      fmt::format("{} {}{}\n", reg(init), reg(mode), emph(mode_type + thd)),
      fmt::format("{}Program: {} ({})\n", green,
                             emph(getAppConfig()->prog_name), emph(getAppConfig()->argv_prog_name)),
      fmt::format("{}Running on: {}\n", green, emph(all_node)),
      fmt::format("{}Machine Hostname: {}\n", green, emph(hostname)),
      fmt::format("{}MPI Version: {}.{}\n", green, emph(version), emph(subversion)),
      fmt::format("{}MPI Max tag: {}\n", green, emph(max_tag_str)),

      fmt::format("{}Build SHA: {}\n", green, emph(vt_git_sha1)),
      fmt::format("{}Build Ref: {}\n", green, emph(vt_git_refspec)),
      fmt::format("{}Description: {} {}\n", green, emph(vt_git_description), dirty),
      fmt::format("{}Compile-time Features Enabled:{}\n", green, reset)
  };

  for (auto &&line: info_lines)
  {
    fmt::print("{}{}{}", vt_pre, line, reset);
  }

  for (size_t i = 0; i < features.size(); i++) {
    fmt::print("{}\t{}\n", vt_pre, emph(features.at(i)));
  }

  auto warn_cr = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Warning:{} {}{}{} has no effect: compile-time"
      " feature {}{}{} is disabled{}\n", red, reset, magenta, opt, reset,
      magenta, compile, reset, reset
    );
  };
  auto opt_on = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Option:{} flag {}{}{} on: {}{}\n",
      green, reset, magenta, opt, reset, compile, reset
    );
  };
  auto opt_on_value = [=](std::string opt, std::string val, std::string compile) -> std::string {
    return fmt::format(
      "{}Option:{} flag {}{}{} on with value \"{}\": {}{}\n",
      green, reset, magenta, opt, reset, val, compile, reset
    );
  };
  auto opt_off = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Option:{} flag {}{}{} not set: {}{}\n",
      green, reset, magenta, opt, reset, compile, reset
    );
  };
  auto opt_inverse = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Default:{} {}, use {}{}{} to disable{}\n",
      green, reset, compile, magenta, opt, reset, reset
    );
  };
  auto opt_to_enable = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Default:{} {}, use {}{}{} to enable{}\n",
      green, reset, compile, magenta, opt, reset, reset
    );
  };
  auto opt_remove = [=](std::string opt, std::string compile) -> std::string {
    return fmt::format(
      "{}Option:{} {}, remove {}{}{} to enable{}\n",
      green, reset, compile, magenta, opt, reset, reset
    );
  };

  auto f8 = fmt::format("{}Runtime Configuration:{}\n", green, reset);
  fmt::print("{}{}{}", vt_pre, f8, reset);

  #if !vt_check_enabled(lblite)
    if (getAppConfig()->vt_lb) {
      auto f9 = warn_cr("--vt_lb", "lblite");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
      vtAbort("Load balancing enabled with --vt_lb, but disabled at compile time");
    }
    if (getAppConfig()->vt_lb_stats) {
      auto f9 = warn_cr("--vt_lb_stats", "lblite");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
    }
  #endif

  {
    auto f11 = fmt::format(
      "Running MPI progress {} times each invocation",
      getAppConfig()->vt_sched_num_progress
    );
    auto f12 = opt_on("--vt_sched_num_progress", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  {
    auto f11 = fmt::format(
      "Running MPI progress function at least every {} handler(s) executed",
      getAppConfig()->vt_sched_progress_han
    );
    auto f12 = opt_on("--vt_sched_progress_han", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_sched_progress_sec != 0.0) {
    auto f11 = fmt::format(
      "Running MPI progress function at least every {} seconds",
      getAppConfig()->vt_sched_progress_sec
    );
    auto f12 = opt_on("--vt_sched_progress_sec", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_lb) {
    auto f9 = opt_on("--vt_lb", "Load balancing enabled");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);
    if (getAppConfig()->vt_lb_file_name != "") {
      auto f12 = fmt::format("Reading LB specification from file \"{}\"",
        getAppConfig()->vt_lb_file_name);
      auto f11 = opt_on("--vt_lb_file_name", f12);
      fmt::print("{}\t{}{}", vt_pre, f11, reset);

      if (getAppConfig()->vt_lb_show_spec) {
        auto s = opt_on("--vt_lb_show_spec", "Showing LB specification");
        fmt::print("{}\t{}", vt_pre, s);
        fmt::print(vrt::collection::balance::ReadLBSpec::toString());
      }
    } else {
      auto a3 = fmt::format("Load balancer name: \"{}\"", getAppConfig()->vt_lb_name);
      auto a4 = opt_on("--vt_lb_name", a3);
      fmt::print("{}\t{}{}", vt_pre, a4, reset);
      auto a1 =
        fmt::format("Load balancing interval = {}", getAppConfig()->vt_lb_interval);
      auto a2 = opt_on("--vt_lb_interval", a1);
      fmt::print("{}\t{}{}", vt_pre, a2, reset);

      // Check validity of LB passed to VT
      bool found = false;
      for (auto&& lb : vrt::collection::balance::get_lb_names()) {
        if (getAppConfig()->vt_lb_name == lb.second) {
          found = true;
          break;
        }
      }
      if (not found) {
        auto str = fmt::format(
          "Could not find valid LB named: \"{}\"", getAppConfig()->vt_lb_name
        );
        vtAbort(str);
      }
    }
  }

  if (getAppConfig()->vt_lb_stats) {
    auto f9 = opt_on("--vt_lb_stats", "Load balancing statistics collection");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);

    if (getAppConfig()->vt_lb_stats_compress) {
      auto f10 = opt_on("--vt_lb_stats_compress", "Compressing statistics files");
      fmt::print("{}\t{}{}", vt_pre, f10, reset);
    } else {
      auto f10 = opt_off("--vt_lb_stats_compress", "Not compressing statistics files");
      fmt::print("{}\t{}{}", vt_pre, f10, reset);
    }

    auto const fname = getAppConfig()->vt_lb_stats_file;
    if (fname != "") {
      auto f11 = fmt::format("LB stats file name \"{}\"", fname);
      auto f12 = opt_on("--vt_lb_stats_file", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }

    auto const fdir = getAppConfig()->vt_lb_stats_dir;
    if (fdir != "") {
      auto f11 = fmt::format("LB stats directory \"{}\"", fdir);
      auto f12 = opt_on("--vt_lb_stats_dir", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }

    auto const fnamein = getAppConfig()->vt_lb_stats_file_in;
    if (fnamein != "") {
      auto f11 = fmt::format("LB stats file name in \"{}\"", fnamein);
      auto f12 = opt_on("--vt_lb_stats_file_in", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }

    auto const fdirin = getAppConfig()->vt_lb_stats_dir_in;
    if (fdirin != "") {
      auto f11 = fmt::format("LB stats directory in \"{}\"", fdirin);
      auto f12 = opt_on("--vt_lb_stats_dir_in", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }


  #if !vt_check_enabled(trace_enabled)
    if (getAppConfig()->vt_trace) {
      auto f9 = warn_cr("--vt_trace", "trace_enabled");
      fmt::print("{}\t{}{}", vt_pre, f9, reset);
    }
  #endif

  #if vt_check_enabled(trace_enabled)
  if (getAppConfig()->vt_trace) {
    auto f9 = opt_on("--vt_trace", "Tracing enabled");
    fmt::print("{}\t{}{}", vt_pre, f9, reset);
    if (getAppConfig()->vt_trace_file != "") {
      auto f11 = fmt::format("Trace file name \"{}\"", getAppConfig()->vt_trace_file);
      auto f12 = opt_on("--vt_trace_file", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    } else {
      if (theTrace) {
        auto f11 = fmt::format("Trace file \"{}\"", theTrace->getTraceName());
        auto f12 = opt_inverse("--vt_trace_file", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
    }
    if (getAppConfig()->vt_trace_dir != "") {
      auto f11 = fmt::format("Directory \"{}\"", getAppConfig()->vt_trace_dir);
      auto f12 = opt_on("--vt_trace_dir", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    } else {
      if (theTrace) {
        auto f11 = fmt::format(
          "Trace directory \"{}\"", theTrace->getDirectory()
        );
        auto f12 = opt_inverse("--vt_trace_dir", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
    }
    if (getAppConfig()->vt_trace_mod != 0) {
      auto f11 = fmt::format("Output every {} files ", getAppConfig()->vt_trace_mod);
      auto f12 = opt_on("--vt_trace_mod", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (getAppConfig()->vt_trace_flush_size != 0) {
      auto f11 = fmt::format("Flush output incrementally with a buffer of,"
                             " at least, {} record(s)",
                             getAppConfig()->vt_trace_flush_size);
      auto f12 = opt_on("--vt_trace_flush_size", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    } else {
      auto f11 = fmt::format("Flushing traces at end of run");
      auto f12 = opt_inverse("--vt_trace_flush_size", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (not getAppConfig()->vt_trace_sys_all) {
      if (getAppConfig()->vt_trace_sys_term) {
        auto f11 = fmt::format("Tracing all system termination messages");
        auto f12 = opt_on("--vt_trace_sys_term", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
      if (getAppConfig()->vt_trace_sys_location) {
        auto f11 = fmt::format("Tracing all system location messages");
        auto f12 = opt_on("--vt_trace_sys_location", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
      if (getAppConfig()->vt_trace_sys_collection) {
        auto f11 = fmt::format("Tracing all system collection messages");
        auto f12 = opt_on("--vt_trace_sys_collection", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
      if (getAppConfig()->vt_trace_sys_serial_msg) {
        auto f11 = fmt::format("Tracing all system serialization messages");
        auto f12 = opt_on("--vt_trace_sys_serial_msg", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
    } else {
      auto f11 = fmt::format("Tracing all system messages");
      auto f12 = opt_on("--vt_trace_sys_all", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (getAppConfig()->vt_trace_spec) {
      {
        auto f11 = fmt::format("Using trace enable specification for phases");
        auto f12 = opt_on("--vt_trace_spec", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
      if (getAppConfig()->vt_trace_spec_file == "") {
        auto warn_trace_file = fmt::format(
          "{}Warning:{} {}{}{} has no effect: no specification file given"
          " option {}{}{} is empty{}\n", red, reset, magenta,
          "--vt_trace_spec",
          reset, magenta, "--vt_trace_spec_file", reset, reset
        );
        fmt::print("{}\t{}{}", vt_pre, warn_trace_file, reset);
      } else {
        auto f11 = fmt::format(
          "Using trace specification file \"{}\"",
          getAppConfig()->vt_trace_spec_file
        );
        auto f12 = opt_inverse("--vt_trace_spec", f11);
        fmt::print("{}\t{}{}", vt_pre, f12, reset);
      }
    }
    if (getAppConfig()->vt_trace_memory_usage) {
      auto f11 = fmt::format("Tracing memory usage");
      auto f12 = opt_on("--vt_trace_memory_usage", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (getAppConfig()->vt_trace_mpi) {
      auto f11 = fmt::format("Tracing MPI invocations (select internal calls)");
      auto f12 = opt_on_value("--vt_trace_mpi", "internal", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (getAppConfig()->vt_trace_pmpi) {
      auto f11 = fmt::format("Tracing MPI invocations (external calls)");
      auto f12 = opt_on_value("--vt_trace_mpi", "external", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (getAppConfig()->vt_trace_event_polling) {
      auto f11 = fmt::format("Tracing event polling (inc. MPI Isend requests)");
      auto f12 = opt_on("--vt_trace_event_polling", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
    if (getAppConfig()->vt_trace_irecv_polling) {
      auto f11 = fmt::format("Tracing MPI Irecv polling");
      auto f12 = opt_on("--vt_trace_irecv_polling", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }
  #endif


  if (getAppConfig()->vt_term_rooted_use_ds) {
    auto f11 = fmt::format("Forcing the use of Dijkstra-Scholten for rooted TD");
    auto f12 = opt_on("--vt_term_rooted_use_ds", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_term_rooted_use_wave) {
    auto f11 = fmt::format("Forcing the use of 4-counter wave-based for rooted TD");
    auto f12 = opt_on("--vt_term_rooted_use_wave", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_print_no_progress) {
    auto f11 = fmt::format("Printing warnings when progress is stalls");
    auto f12 = opt_on("--vt_print_no_progress", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_epoch_graph_terse) {
    auto f11 = fmt::format("Printing terse epoch graphs when hang detected");
    auto f12 = opt_on("--vt_epoch_graph_terse", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Printing verbose epoch graphs when hang detected");
    auto f12 = opt_inverse("--vt_epoch_graph_terse", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_epoch_graph_on_hang) {
    auto f11 = fmt::format("Epoch graph output enabled if hang detected");
    auto f12 = opt_on("--vt_epoch_graph_on_hang", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_detect_hang) {
    auto f11 = fmt::format("Disabling termination hang detection");
    auto f12 = opt_on("--vt_no_detect_hang", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Termination hang detection enabled by default");
    auto f12 = opt_inverse("--vt_no_detect_hang", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (!getAppConfig()->vt_no_detect_hang) {
    if (getAppConfig()->vt_hang_freq != 0) {
      auto f11 = fmt::format(
        "Printing stall warning every {} tree traversals ", getAppConfig()->vt_hang_freq
      );
      auto f12 = opt_on("--vt_hang_detect", f11);
      fmt::print("{}\t{}{}", vt_pre, f12, reset);
    }
  }

  if (getAppConfig()->vt_no_sigint) {
    auto f11 = fmt::format("Disabling SIGINT signal handling");
    auto f12 = opt_on("--vt_no_SIGINT", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("SIGINT signal handling enabled by default");
    auto f12 = opt_inverse("--vt_no_SIGINT", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_sigsegv) {
    auto f11 = fmt::format("Disabling SIGSEGV signal handling");
    auto f12 = opt_on("--vt_no_SIGSEGV", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("SIGSEGV signal handling enabled by default");
    auto f12 = opt_inverse("--vt_no_SIGSEGV", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_sigbus) {
    auto f11 = fmt::format("Disabling SIGBUS signal handling");
    auto f12 = opt_on("--vt_no_SIGBUS", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("SIGBUS signal handling enabled by default");
    auto f12 = opt_inverse("--vt_no_SIGBUS", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_terminate) {
    auto f11 = fmt::format("Disabling std::terminate signal handling");
    auto f12 = opt_on("--vt_no_terminate", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("std::terminate signal handling enabled by default");
    auto f12 = opt_inverse("--vt_no_terminate", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_color) {
    auto f11 = fmt::format("Color output disabled");
    auto f12 = opt_on("--vt_no_color", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Color output enabled");
    auto f12 = opt_inverse("--vt_no_color", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_stack) {
    auto f11 = fmt::format("Disabling all stack dumps");
    auto f12 = opt_on("--vt_no_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  } else {
    auto f11 = fmt::format("Stack dumps enabled by default");
    auto f12 = opt_inverse("--vt_no_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_warn_stack) {
    auto f11 = fmt::format("Disabling all stack dumps on vtWarn(..)");
    auto f12 = opt_on("--vt_no_warn_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_assert_stack) {
    auto f11 = fmt::format("Disabling all stack dumps on vtAssert(..)");
    auto f12 = opt_on("--vt_no_assert_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_no_abort_stack) {
    auto f11 = fmt::format("Disabling all stack dumps on vtAbort(..)");
    auto f12 = opt_on("--vt_no_abort_stack", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_stack_file != "") {
    auto f11 = fmt::format(
      "Output stack dumps with file name {}", getAppConfig()->vt_stack_file
    );
    auto f12 = opt_on("--vt_stack_file", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_stack_dir != "") {
    auto f11 = fmt::format("Output stack dumps to {}", getAppConfig()->vt_stack_dir);
    auto f12 = opt_on("--vt_stack_dir", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_stack_mod != 0) {
    auto f11 = fmt::format(
      "Output stack dumps every {} files ", getAppConfig()->vt_stack_mod
    );
    auto f12 = opt_on("--vt_stack_mod", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_pause) {
    auto f11 = fmt::format("Enabled debug pause at startup");
    auto f12 = opt_on("--vt_pause", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_output_config) {
    auto f11 = fmt::format("Enabled configuration output");
    auto f12 = opt_on("--vt_output_config", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);

    if (getAppConfig()->vt_output_config_file != "") {
      auto f13 = fmt::format(
        "Config file name \"{}\"", getAppConfig()->vt_output_config_file
      );
      auto f14 = opt_on("--vt_output_config_file", f13);
      fmt::print("{}\t{}{}", vt_pre, f14, reset);
    }
  }

  if (getAppConfig()->vt_memory_reporters != "") {
    auto f11 = fmt::format(
      "Memory usage checker precedence: {}",
      getAppConfig()->vt_memory_reporters
    );
    auto f12 = opt_on("--vt_memory_reporters", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);

    auto working_reporters = theMemUsage->getWorkingReporters();
    if (working_reporters.size() > 0) {
      std::string working_str = "";
      for (std::size_t i = 0; i < working_reporters.size(); i++) {
        working_str += working_reporters[i];
        if (i != working_reporters.size() - 1) {
          working_str += ",";
        }
      }
      auto f13 = fmt::format(
        "{}Working memory reporters:{} {}{}{}\n",
        green, reset, magenta, working_str, reset
      );
      fmt::print("{}\t{}{}", vt_pre, f13, reset);

      auto all_usage_str = theMemUsage->getUsageAll();
      if (all_usage_str != "") {
        auto f14 = fmt::format(
          "{}Initial memory usage:{} {}\n",
          green, reset, all_usage_str
        );
        fmt::print("{}\t{}{}", vt_pre, f14, reset);
      }
    }

    if (getAppConfig()->vt_print_memory_each_phase) {
      auto f15 = fmt::format("Printing memory usage each phase");
      auto f16 = opt_on("--vt_print_memory_each_phase", f15);
      fmt::print("{}\t{}{}", vt_pre, f16, reset);

      auto f17 = fmt::format(
        "Printing memory usage from node: {}", getAppConfig()->vt_print_memory_node
      );
      auto f18 = opt_on("--vt_print_memory_node", f17);
      fmt::print("{}\t{}{}", vt_pre, f18, reset);
    } else {
      auto f15 = fmt::format("Memory usage printing disabled");
      auto f16 = opt_to_enable("--vt_print_memory_each_phase", f15);
      fmt::print("{}\t{}{}", vt_pre, f16, reset);
    }

    if (getAppConfig()->vt_print_memory_at_threshold) {
      auto f15 = fmt::format("Printing memory usage at threshold increment");
      auto f16 = opt_on("--vt_print_memory_at_threshold", f15);
      fmt::print("{}\t{}{}", vt_pre, f16, reset);

      auto f17 = fmt::format(
        "Printing memory usage using threshold: {}",
        getAppConfig()->vt_print_memory_threshold
      );
      auto f18 = opt_on("--vt_print_memory_threshold", f17);
      fmt::print("{}\t{}{}", vt_pre, f18, reset);

      theMemUsage->convertBytesFromString(getAppConfig()->vt_print_memory_threshold);

      auto f19 = fmt::format(
        "Polling for memory usage threshold every {} scheduler calls",
        getAppConfig()->vt_print_memory_sched_poll
      );
      auto f20 = opt_on("--vt_print_memory_sched_poll", f19);
      fmt::print("{}\t{}{}", vt_pre, f20, reset);
    }
  }

#if vt_check_enabled(diagnostics)
  if (getAppConfig()->vt_diag_enable) {
#   if vt_check_enabled(diagnostics_runtime)
    auto f11 = fmt::format("Diagnostics are enabled by default");
#   else
    auto f11 = fmt::format("Diagnostics are enabled");
#   endif
    auto f12 = opt_on("--vt_diag_enable", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);

    if (getAppConfig()->vt_diag_summary_file != "") {
      auto f13 = fmt::format(
        "Outputting diagnostics summary file \"{}\"",
        getAppConfig()->vt_diag_summary_file
      );
      auto f14 = opt_on("--vt_diag_summary_file", f13);
      fmt::print("{}\t{}{}", vt_pre, f14, reset);
    }

    if (getAppConfig()->vt_diag_summary_csv_file != "") {
      auto f13 = fmt::format(
        "Outputting CSV diagnostics summary file \"{}\"",
        getAppConfig()->vt_diag_summary_csv_file
      );
      auto f14 = opt_on("--vt_diag_summary_csv_file", f13);
      fmt::print("{}\t{}{}", vt_pre, f14, reset);
    }
  } else {
#   if vt_check_enabled(diagnostics_runtime)
    auto f11 = fmt::format("Diagnostics are disabled");
#   else
    auto f11 = fmt::format("Diagnostics are disabled by default");
#   endif
    auto f12 = opt_to_enable("--vt_diag_enable", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }
#endif

#if vt_check_enabled(fcontext)
  if (not getAppConfig()->vt_ult_disable) {
    auto f11 = fmt::format("Handlers running in user-level threads are enabled");

    auto f12 = opt_off("--vt_ult_disable", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);

    auto const ret = util::memory::getBestMemoryUnit(
      getAppConfig()->vt_ult_stack_size
    );
    auto f13 = fmt::format(
      "User-level stack size is {} {}",
      std::get<1>(ret), std::get<0>(ret)
    );
    auto f14 = opt_on("--vt_ult_stack_size", f13);
    fmt::print("{}\t{}{}", vt_pre, f14, reset);
  } else {
    auto f11 = fmt::format("Handlers running in user-level threads are disabled");
    auto f12 = opt_remove("--vt_ult_disable", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }
#endif

  // Limit to between 256 B and 1 GiB. If its too small a VT envelope won't fit;
  // if its too large we overflow an integer passed to MPI.
  if (getAppConfig()->vt_max_mpi_send_size < 256) {
    vtAbort("Max size for MPI send must be greater than 256 B");
  } else if (getAppConfig()->vt_max_mpi_send_size > 1ull << 30) {
    vtAbort("Max size for MPI send must not be greater than 1 GiB (overflow)");
  } else {
    auto const bytes = getAppConfig()->vt_max_mpi_send_size;
    auto const ret = util::memory::getBestMemoryUnit(bytes);
    auto f_max = fmt::format(
      "Splitting messages greater than {} {}",
      std::get<1>(ret), std::get<0>(ret)
    );
    auto f_max_arg = opt_on("--vt_max_mpi_send_size", f_max);
    fmt::print("{}\t{}{}", vt_pre, f_max_arg, reset);
  }

  {
    std::string print_level = "";
    auto const& level = getAppConfig()->vt_debug_level;
    if (level == "0" or level == "terse") {
      print_level = debug::yellow() + std::string{"terse"} + reset;
    } else if (level == "1" or level == "normal") {
      print_level = debug::yellow() + std::string{"normal"} + reset;
    } else if (level == "2" or level == "verbose") {
      print_level = debug::yellow() + std::string{"verbose"} + reset;
#if !vt_check_enabled(debug_verbose)
      auto w1 = warn_cr("--vt_debug_level=verbose", "vt_debug_verbose");
      fmt::print("{}\t{}{}", vt_pre, w1, reset);
#endif
    }
    auto f11 = fmt::format("Debug print mode set to {}", print_level);
    auto f12 = opt_on("--vt_debug_level", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_debug_all) {
    auto f11 = fmt::format("All debug prints are on (if enabled compile-time)");
    auto f12 = opt_on("--vt_debug_all", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

  if (getAppConfig()->vt_debug_print_flush) {
    auto f11 = fmt::format("Flushing stdout after all VT prints is enabled");
    auto f12 = opt_on("--vt_debug_print_flush", f11);
    fmt::print("{}\t{}{}", vt_pre, f12, reset);
  }

#define vt_runtime_debug_warn_compile(opt)                              \
  do {                                                                  \
    if (vt_check_enabled(production_build) and getAppConfig()->vt_debug_ ## opt) { \
      auto f9 = warn_cr("--vt_debug_" #opt, "vt_debug_print");             \
      fmt::print("{}\t{}{}", vt_pre, f9, reset);                        \
    }                                                                   \
  } while (0);

  vt_runtime_debug_warn_compile(none)
  vt_runtime_debug_warn_compile(gen)
  vt_runtime_debug_warn_compile(runtime)
  vt_runtime_debug_warn_compile(active)
  vt_runtime_debug_warn_compile(term)
  vt_runtime_debug_warn_compile(termds)
  vt_runtime_debug_warn_compile(barrier)
  vt_runtime_debug_warn_compile(event)
  vt_runtime_debug_warn_compile(pipe)
  vt_runtime_debug_warn_compile(pool)
  vt_runtime_debug_warn_compile(reduce)
  vt_runtime_debug_warn_compile(rdma)
  vt_runtime_debug_warn_compile(rdma_channel)
  vt_runtime_debug_warn_compile(rdma_state)
  vt_runtime_debug_warn_compile(param)
  vt_runtime_debug_warn_compile(handler)
  vt_runtime_debug_warn_compile(hierlb)
  vt_runtime_debug_warn_compile(temperedlb)
  vt_runtime_debug_warn_compile(scatter)
  vt_runtime_debug_warn_compile(sequence)
  vt_runtime_debug_warn_compile(sequence_vrt)
  vt_runtime_debug_warn_compile(serial_msg)
  vt_runtime_debug_warn_compile(trace)
  vt_runtime_debug_warn_compile(location)
  vt_runtime_debug_warn_compile(vrt)
  vt_runtime_debug_warn_compile(vrt_coll)
  vt_runtime_debug_warn_compile(worker)
  vt_runtime_debug_warn_compile(group)
  vt_runtime_debug_warn_compile(broadcast)
  vt_runtime_debug_warn_compile(objgroup)
  vt_runtime_debug_warn_compile(phase)
  vt_runtime_debug_warn_compile(context)
  vt_runtime_debug_warn_compile(epoch)

  auto arg_str = [](std::vector<char*> const& args) -> std::string {
    std::stringstream ss;
    for (auto&& arg : args) {
      ss << "\"" << arg << "\", ";
    }
    std::string s = ss.str();
    return s.empty() ? s : s.substr(0, s.size() - 2);
  };

  auto f88 = fmt::format("{}Pass-through Arguments:{}\n", green, reset);
  fmt::print("{}{}{}", vt_pre, f88, reset);

  std::vector<char*> const& passthru_args = getAppConfig()->passthru_args;

  if (passthru_args.empty()) {
    auto f11 = fmt::format("None. All arguments handled.\n");
    fmt::print("{}\t{}{}", vt_pre, f11, reset);
  } else {
    auto f11 = fmt::format(
      "Application args: [{}]\n",
      arg_str(passthru_args)
    );
    fmt::print("{}\t{}{}", vt_pre, f11, reset);
  }

  if (getAppConfig()->vt_print_memory_footprint) {
    printMemoryFootprint();
  }

  //fmt::print("{}\n", reset);
  fmt::print(reset);

  // Enqueue a check for later in case arguments are modified before work
  // actually executes
  theSched->enqueue([this]{
    this->checkForArgumentErrors();
  });
}

void Runtime::printShutdownBanner(
  term::TermCounterType const& num_units, std::size_t const coll_epochs
) {
  // If --vt_quiet is set, immediately exit printing nothing during shutdown
  if (getAppConfig()->vt_quiet) {
    return;
  }
  auto green    = debug::green();
  auto reset    = debug::reset();
  auto bd_green = debug::bd_green();
  auto magenta  = debug::magenta();
  auto f1 = fmt::format("{}Runtime Finalizing{}", green, reset);
  auto f2 = fmt::format("{}Total work units processed:{} ", green, reset);
  auto f3 = fmt::format("{}Total collective epochs processed:{} ", green, reset);
  auto vt_pre = bd_green + std::string("vt") + reset + ": ";
  std::string fin = "";
  std::string units = std::to_string(num_units);
  fmt::print("{}{}{}{}{}\n", vt_pre, f3, magenta, coll_epochs, reset);
  fmt::print("{}{}{}{}{}\n", vt_pre, f2, magenta, units, reset);
  fmt::print("{}{}{}\n", vt_pre, f1, reset);
}

void Runtime::checkForArgumentErrors() {
  #if !vt_check_enabled(lblite)
    if (getAppConfig()->vt_lb) {
      vtAbort("Load balancing enabled with --vt_lb, but disabled at compile time");
    }
  #endif
}

}} //end namespace vt::runtime
