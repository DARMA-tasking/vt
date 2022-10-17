/*
//@HEADER
// *****************************************************************************
//
//                                   args.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/utils/demangle/demangle.h"

#include <string>
#include <vector>
#include <sstream>
#include <tuple>
#include <ctime>

#include "CLI/CLI11.hpp"
#include "yaml-cpp/yaml.h"

namespace vt { namespace arguments {

// Temporary variables used only for parsing artifacts.
namespace {

// Set the YAML labels for all CLI args

// Output Control
static const std::string vt_color_label = "Color";
static const std::string vt_no_color_label = "No Color";
static const std::string vt_quiet_label = "Quiet";

// Signal Handling
static const std::string vt_no_sigint_label = "Disable SIGINT";
static const std::string vt_no_sigsegv_label = "Disable SIGSEGV";
static const std::string vt_no_sigbus_label = "Disable SIGBUS";
static const std::string vt_no_terminate_label = "Disable Terminate Signal";

// Memory Usage Reporting
static const std::string vt_memory_reporters_label = "Memory Reporters";
static const std::string vt_print_memory_each_phase_label = "Print Memory Each Phase";
static const std::string vt_print_memory_node_label = "Print Memory On Node";
static const std::string vt_allow_memory_report_with_ps_label = "Allow Memory Report With ps";
static const std::string vt_print_memory_threshold_label = "Print Memory Threshold";
static const std::string vt_print_memory_sched_poll_label = "Print Memory Scheduler Poll";
static const std::string vt_print_memory_footprint_label = "Print Memory Footprint";

// Dump Stack Backtrace
static const std::string vt_no_warn_stack_label = "Enable Stack Output on Warning";
static const std::string vt_no_assert_stack_label = "Enable Stack Output on Assert";
static const std::string vt_no_abort_stack_label = "Enable Stack Output on Abort";
static const std::string vt_no_stack_label = "Enable Stack Output";
static const std::string vt_stack_file_label = "File";
static const std::string vt_stack_dir_label = "Directory";
static const std::string vt_stack_mod_label = "Output Rank Mod";

// Tracing Configuration
static const std::string vt_trace_label = "Enabled";
static const std::string vt_trace_mpi_label = "MPI Type Events";
static const std::string vt_trace_pmpi_label = "MPI Type Events";
static const std::string vt_trace_file_label = "File";
static const std::string vt_trace_dir_label = "Directory";
static const std::string vt_trace_mod_label = "Output Rank Mod";
static const std::string vt_trace_flush_size_label = "Flush Size";
static const std::string vt_trace_gzip_finish_flush_label = "GZip Finish Flush";
static const std::string vt_trace_sys_all_label = "Include All System Events";
static const std::string vt_trace_sys_term_label = "Include Termination Events";
static const std::string vt_trace_sys_location_label = "Include Location Events";
static const std::string vt_trace_sys_collection_label = "Include Collection Events";
static const std::string vt_trace_sys_serial_msg_label = "Include Message Serialization Events";
static const std::string vt_trace_spec_label = "Specification Enabled";
static const std::string vt_trace_spec_file_label = "Spec File";
static const std::string vt_trace_memory_usage_label = "Memory Usage";
static const std::string vt_trace_event_polling_label = "Event Polling";
static const std::string vt_trace_irecv_polling_label = "IRecv Polling";

// Debug Print Configuration
static const std::string vt_debug_level_label = "Level";
static const std::string vt_debug_all_label = "Enable All";
static const std::string vt_debug_none_label = "Disable All";
static const std::string vt_debug_print_flush_label = "Debug Print Flush";
static const std::string vt_debug_replay_label = "Debug Replay";
static const std::string vt_debug_gen_label = "gen";
static const std::string vt_debug_runtime_label = "runtime";
static const std::string vt_debug_active_label = "active";
static const std::string vt_debug_term_label = "term";
static const std::string vt_debug_termds_label = "termds";
static const std::string vt_debug_barrier_label = "barrier";
static const std::string vt_debug_event_label = "event";
static const std::string vt_debug_pipe_label = "pipe";
static const std::string vt_debug_pool_label = "pool";
static const std::string vt_debug_reduce_label = "reduce";
static const std::string vt_debug_rdma_label = "rdma";
static const std::string vt_debug_rdma_channel_label = "rdma_channel";
static const std::string vt_debug_rdma_state_label = "rdma_state";
static const std::string vt_debug_handler_label = "handler";
static const std::string vt_debug_hierlb_label = "hierlb";
static const std::string vt_debug_temperedlb_label = "temperedlb";
static const std::string vt_debug_temperedwmin_label = "temperedwmin";
static const std::string vt_debug_scatter_label = "scatter";
static const std::string vt_debug_serial_msg_label = "serial_msg";
static const std::string vt_debug_trace_label = "trace";
static const std::string vt_debug_location_label = "location";
static const std::string vt_debug_lb_label = "lb";
static const std::string vt_debug_vrt_label = "vrt";
static const std::string vt_debug_vrt_coll_label = "vrt_coll";
static const std::string vt_debug_worker_label = "worker";
static const std::string vt_debug_group_label = "group";
static const std::string vt_debug_broadcast_label = "broadcast";
static const std::string vt_debug_objgroup_label = "objgroup";
static const std::string vt_debug_phase_label = "phase";
static const std::string vt_debug_context_label = "context";
static const std::string vt_debug_epoch_label = "epoch";

// Load Balancing
static const std::string vt_lb_label = "Enabled";
static const std::string vt_lb_quiet_label = "Quiet";
static const std::string vt_lb_file_name_label = "File";
static const std::string vt_lb_show_config_label = "Show Configuration";
static const std::string vt_lb_name_label = "Name";
static const std::string vt_lb_args_label = "Arguments";
static const std::string vt_lb_interval_label = "Interval";
static const std::string vt_lb_keep_last_elm_label = "Keep Last Element";
static const std::string vt_lb_data_label = "Enabled";
static const std::string vt_lb_data_dir_label = "Directory";
static const std::string vt_lb_data_file_label = "File";
static const std::string vt_lb_data_in_label = "Enabled";
static const std::string vt_lb_data_compress_label = "Enable Compression";
static const std::string vt_lb_data_dir_in_label = "Directory";
static const std::string vt_lb_data_file_in_label = "File";
static const std::string vt_lb_statistics_label = "Enabled";
static const std::string vt_lb_statistics_compress_label = "Enable Compression";
static const std::string vt_lb_statistics_file_label = "File";
static const std::string vt_lb_statistics_dir_label = "Directory";
static const std::string vt_lb_self_migration_label = "Enable Self Migration";
static const std::string vt_lb_spec_label = "Enable Specification";
static const std::string vt_lb_spec_file_label = "Specification File";

// Diagnostics
static const std::string vt_diag_enable_label = "Enabled";
static const std::string vt_diag_print_summary_label = "Enable Print Summary";
static const std::string vt_diag_summary_file_label = "Summary File";
static const std::string vt_diag_summary_csv_file_label = "Summary CSV File";
static const std::string vt_diag_csv_base_units_label = "Use CSV Base Units";

// Termination
static const std::string vt_no_detect_hang_label = "No Detect Hangs";
static const std::string vt_term_rooted_use_ds_label = "Use DS for Rooted";
static const std::string vt_term_rooted_use_wave_label = "Use Wave for Rooted";
static const std::string vt_epoch_graph_on_hang_label = "Output Epoch Graph on Hang";
static const std::string vt_epoch_graph_terse_label = "Terse Epoch Graph Output";
static const std::string vt_print_no_progress_label = "Print No Progress";
static const std::string vt_hang_freq_label = "Hang Check Frequency";

// Debugging/Launch
static const std::string vt_pause_label = "Pause";

// Scheduler Configuration
static const std::string vt_sched_num_progress_label = "Num Progress Times";
static const std::string vt_sched_progress_han_label = "Progress Handlers";
static const std::string vt_sched_progress_sec_label = "Progress Seconds";

// Configuration File
static const std::string vt_output_config_label = "Enable Output Config";
static const std::string vt_output_config_file_label = "File";

// Runtime
static const std::string vt_max_mpi_send_size_label = "Max MPI Send Size";
static const std::string vt_no_assert_fail_label = "Disable Assert Failure";
static const std::string vt_throw_on_abort_label = "Throw on Abort";

// Visualization
static const std::string vt_tv_label = "Enabled";
static const std::string vt_tv_config_file_label = "Configuration File";

std::unordered_map<std::string, std::string> user_args_labels = {
  {"vt_user_1", "unused_user_param"},
  {"vt_user_2", "unused_user_param"},
  {"vt_user_3", "unused_user_param"},
  {"vt_user_int_1", "unused_user_param"},
  {"vt_user_int_2", "unused_user_param"},
  {"vt_user_int_3", "unused_user_param"},
  {"vt_user_str_1", "unused_user_param"},
  {"vt_user_str_2", "unused_user_param"},
  {"vt_user_str_3", "unused_user_param"}
};

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

void parseYaml(AppConfig& appConfig,  std::string const& inputFile);
void convertConfigToString(CLI::App& app, AppConfig& appConfig);

std::tuple<int, std::string> parseArguments(
  CLI::App& app, int& argc, char**& argv, AppConfig& appConfig
) {

  std::vector<char*> vt_args;

  // Load up vectors
  // Has the ability to read interleaved arguments for vt, MPI, and others passed through to other libraries or the application.
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
  std::vector<std::string> args_to_parse, yaml_input_arg;
  for (auto it = vt_args.crbegin(); it != vt_args.crend(); ++it) {
    if (util::demangle::DemanglerUtils::splitString(*it,'=')[0] == "--vt_input_config_yaml") {
      yaml_input_arg.push_back(*it);
    } else {
      args_to_parse.push_back(*it);
    }
  }

  // Identify input YAML file first, if present
  if (!yaml_input_arg.empty()) {
    try {
      app.parse(yaml_input_arg);
    } catch (CLI::Error &ex) {
      // Return exit code and message, delaying logic processing of such.
      // The default exit code for 'help' is 0.
      std::stringstream yaml_message_stream;
      int yaml_result = app.exit(ex, yaml_message_stream, yaml_message_stream);
      return std::make_tuple(yaml_result, yaml_message_stream.str());
    }
  }

  // Parse the YAML parameters
  if (appConfig.vt_input_config_yaml != "") {
    parseYaml(appConfig, appConfig.vt_input_config_yaml);
  }

  // Then parse the remaining arguments
  try {
    app.parse(args_to_parse);
  } catch (CLI::Error &ex) {
    std::stringstream message_stream;
    int result = app.exit(ex, message_stream, message_stream);
    return std::make_tuple(result, message_stream.str());
  }

  // If the user specified to output the full configuration, save it in a string
  // so node 0 can output in the runtime once MPI is init'ed
  if (appConfig.vt_output_config) {
    convertConfigToString(app, appConfig);
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

void addYamlArgs(CLI::App& app, AppConfig& appConfig) {
  auto yaml_description = "Read in a YAML config file for VT";
  app.add_option("--vt_input_config_yaml", appConfig.vt_input_config_yaml, yaml_description);
}

void addUserOptionsFromYaml(YAML::Node& inputYaml, AppConfig& appConfig) {
  YAML::Node user_options = inputYaml["User Options"];
  int bool_iter = 1, int_iter = 1, str_iter = 1;
  bool too_many_user_args = false;
  for (YAML::const_iterator it = user_options.begin(); it != user_options.end(); ++it) {
     std::string const& key = it->first.as<std::string>();
    try {
      bool user_input = user_options[key].as<bool>();
      if (bool_iter == 1)
        appConfig.vt_user_1 = user_input;
      else if (bool_iter == 2)
        appConfig.vt_user_2 = user_input;
      else if (bool_iter == 3)
        appConfig.vt_user_3 = user_input;
      else too_many_user_args = true;
      user_args_labels["vt_user_" + std::to_string(bool_iter)] = key;
      bool_iter++;
    } catch (const YAML::RepresentationException&) {
      try {
        int user_input = user_options[key].as<int>();
        if (int_iter == 1)
          appConfig.vt_user_int_1 = user_input;
        else if (int_iter == 2)
          appConfig.vt_user_int_2 = user_input;
        else if (int_iter == 3)
          appConfig.vt_user_int_3 = user_input;
        else
          too_many_user_args = true;
        user_args_labels["vt_user_int_" + std::to_string(int_iter)] = key;
        int_iter++;
      } catch (const YAML::RepresentationException&) {
        std::string user_input = user_options[key].as<std::string>();
        if (str_iter == 1)
          appConfig.vt_user_str_1 = user_input;
        else if (str_iter == 2)
          appConfig.vt_user_str_2 = user_input;
        else if (str_iter == 3)
          appConfig.vt_user_str_3 = user_input;
        else too_many_user_args = true;
        user_args_labels["vt_user_str_" + std::to_string(str_iter)] = key;
        str_iter++;
      }
    }
  }
  vtAbortIf(too_many_user_args, "Only three user-defined arguments of each type (bool, int, or string) are supported.");
}

void parseYaml(AppConfig& appConfig, std::string const& inputFile) {
  // Read the config value from YAML using default (current) value as fallback
  auto update_config = [](
    auto& config_entry, std::string const& label, YAML::Node const& node
  ) {
    using config_type = std::remove_reference_t<decltype(config_entry)>;
    config_entry = node[label].as<config_type>(config_entry);
  };

  // Read in the YAML configuration file
  auto yaml_input = YAML::LoadFile(inputFile);

  // Output control
  YAML::Node output_control = yaml_input["Output Control"];
  update_config(appConfig.vt_color, vt_color_label, output_control);
  update_config(appConfig.vt_no_color, vt_no_color_label, output_control);
  update_config(appConfig.vt_quiet, vt_quiet_label, output_control);

  // Signal handling
  YAML::Node signal_handling = yaml_input["Signal Handling"];
  update_config(appConfig.vt_no_sigint, vt_no_sigint_label, signal_handling);
  update_config(appConfig.vt_no_sigsegv, vt_no_sigsegv_label, signal_handling);
  update_config(appConfig.vt_no_sigbus, vt_no_sigbus_label, signal_handling);
  update_config(appConfig.vt_no_terminate, vt_no_terminate_label, signal_handling);

  // Memory Usage Reporting
  YAML::Node memory_usage_reporting = yaml_input["Memory Usage Reporting"];
  std::string default_vt_memory_reporters =
  #if (vt_feature_mimalloc != 0)
  "mimalloc,"
  #endif
  "mstats,machinfo,selfstat,selfstatm,sbrk,mallinfo,getrusage,ps";
  update_config(appConfig.vt_memory_reporters, vt_memory_reporters_label, memory_usage_reporting);
  update_config(appConfig.vt_print_memory_each_phase, vt_print_memory_each_phase_label, memory_usage_reporting);
  update_config(appConfig.vt_print_memory_node, vt_print_memory_node_label, memory_usage_reporting);
  update_config(appConfig.vt_allow_memory_report_with_ps, vt_allow_memory_report_with_ps_label, memory_usage_reporting);
  if (memory_usage_reporting[vt_print_memory_threshold_label]) {
    appConfig.vt_print_memory_at_threshold = true;
    update_config(appConfig.vt_print_memory_threshold, vt_print_memory_threshold_label, memory_usage_reporting);
  } else {
    appConfig.vt_print_memory_at_threshold = false;
  }
  update_config(appConfig.vt_print_memory_sched_poll, vt_print_memory_sched_poll_label, memory_usage_reporting);
  update_config(appConfig.vt_print_memory_footprint, vt_print_memory_footprint_label, memory_usage_reporting);

  // Dump Stack Backtrace
  YAML::Node dump_stack_backtrace = yaml_input["Dump Stack Backtrace"];
  update_config(appConfig.vt_no_warn_stack, vt_no_warn_stack_label, dump_stack_backtrace);
  update_config(appConfig.vt_no_assert_stack, vt_no_assert_stack_label, dump_stack_backtrace);
  update_config(appConfig.vt_no_abort_stack, vt_no_abort_stack_label, dump_stack_backtrace);
  update_config(appConfig.vt_no_stack, vt_no_stack_label, dump_stack_backtrace);
  update_config(appConfig.vt_stack_file, vt_stack_file_label, dump_stack_backtrace);
  update_config(appConfig.vt_stack_dir, vt_stack_dir_label, dump_stack_backtrace);
  update_config(appConfig.vt_stack_mod, vt_stack_mod_label, dump_stack_backtrace);

  // Tracing Configuration
  YAML::Node tracing_configuration = yaml_input["Tracing Configuration"];
  update_config(appConfig.vt_trace, vt_trace_label, tracing_configuration);

  appConfig.vt_trace_mpi = tracing_configuration[vt_trace_mpi_label].as<std::string>("").find("internal") != std::string::npos;
  appConfig.vt_trace_pmpi = tracing_configuration[vt_trace_pmpi_label].as<std::string>("").find("external") != std::string::npos;

  update_config(appConfig.vt_trace_file, vt_trace_file_label, tracing_configuration);
  update_config(appConfig.vt_trace_dir, vt_trace_dir_label, tracing_configuration);
  update_config(appConfig.vt_trace_mod, vt_trace_mod_label, tracing_configuration);
  update_config(appConfig.vt_trace_flush_size, vt_trace_flush_size_label, tracing_configuration);
  update_config(appConfig.vt_trace_gzip_finish_flush, vt_trace_gzip_finish_flush_label, tracing_configuration);
  update_config(appConfig.vt_trace_sys_all, vt_trace_sys_all_label, tracing_configuration);
  update_config(appConfig.vt_trace_sys_term, vt_trace_sys_term_label, tracing_configuration);
  update_config(appConfig.vt_trace_sys_location, vt_trace_sys_location_label, tracing_configuration);
  update_config(appConfig.vt_trace_sys_collection, vt_trace_sys_collection_label, tracing_configuration);
  update_config(appConfig.vt_trace_sys_serial_msg, vt_trace_sys_serial_msg_label, tracing_configuration);
  update_config(appConfig.vt_trace_spec, vt_trace_spec_label, tracing_configuration);
  update_config(appConfig.vt_trace_spec_file, vt_trace_spec_file_label, tracing_configuration);
  update_config(appConfig.vt_trace_memory_usage, vt_trace_memory_usage_label, tracing_configuration);
  update_config(appConfig.vt_trace_event_polling, vt_trace_event_polling_label, tracing_configuration);
  update_config(appConfig.vt_trace_irecv_polling, vt_trace_irecv_polling_label, tracing_configuration);

  // Debug Print Configuration
  YAML::Node debug_print_configuration = yaml_input["Debug Print Configuration"];
  update_config(appConfig.vt_debug_level, vt_debug_level_label, debug_print_configuration);
  update_config(appConfig.vt_debug_all, vt_debug_all_label, debug_print_configuration);
  update_config(appConfig.vt_debug_none, vt_debug_none_label, debug_print_configuration);

  std::vector<std::string> debug_enables;
  if (debug_print_configuration["Enable"]) {
      debug_enables = debug_print_configuration["Enable"].as<std::vector<std::string>>();
  }

  // All vt_debug_ categories are false by default (no need to check current value)
  auto is_debug_enabled = [&debug_enables]( std::string const& debug_type) {
    return std::find(debug_enables.begin(), debug_enables.end(), debug_type) != debug_enables.end();
  };

  appConfig.vt_debug_gen = is_debug_enabled(vt_debug_gen_label);
  appConfig.vt_debug_runtime = is_debug_enabled(vt_debug_runtime_label);
  appConfig.vt_debug_active = is_debug_enabled(vt_debug_active_label);
  appConfig.vt_debug_term = is_debug_enabled(vt_debug_term_label);
  appConfig.vt_debug_termds = is_debug_enabled(vt_debug_termds_label);
  appConfig.vt_debug_barrier = is_debug_enabled(vt_debug_barrier_label);
  appConfig.vt_debug_event = is_debug_enabled(vt_debug_event_label);
  appConfig.vt_debug_pipe = is_debug_enabled(vt_debug_pipe_label);
  appConfig.vt_debug_pool = is_debug_enabled(vt_debug_pool_label);
  appConfig.vt_debug_reduce = is_debug_enabled(vt_debug_reduce_label);
  appConfig.vt_debug_rdma = is_debug_enabled(vt_debug_rdma_label);
  appConfig.vt_debug_rdma_channel = is_debug_enabled(vt_debug_rdma_channel_label);
  appConfig.vt_debug_rdma_state = is_debug_enabled(vt_debug_rdma_state_label);
  appConfig.vt_debug_handler = is_debug_enabled(vt_debug_handler_label);
  appConfig.vt_debug_hierlb = is_debug_enabled(vt_debug_hierlb_label);
  appConfig.vt_debug_temperedlb = is_debug_enabled(vt_debug_temperedlb_label);
  appConfig.vt_debug_temperedwmin = is_debug_enabled(vt_debug_temperedwmin_label);
  appConfig.vt_debug_scatter = is_debug_enabled(vt_debug_scatter_label);
  appConfig.vt_debug_serial_msg = is_debug_enabled(vt_debug_serial_msg_label);
  appConfig.vt_debug_trace = is_debug_enabled(vt_debug_trace_label);
  appConfig.vt_debug_location = is_debug_enabled(vt_debug_location_label);
  appConfig.vt_debug_lb = is_debug_enabled(vt_debug_lb_label);
  appConfig.vt_debug_vrt = is_debug_enabled(vt_debug_vrt_label);
  appConfig.vt_debug_vrt_coll = is_debug_enabled(vt_debug_vrt_coll_label);
  appConfig.vt_debug_worker = is_debug_enabled(vt_debug_worker_label);
  appConfig.vt_debug_group = is_debug_enabled(vt_debug_group_label);
  appConfig.vt_debug_broadcast = is_debug_enabled(vt_debug_broadcast_label);
  appConfig.vt_debug_objgroup = is_debug_enabled(vt_debug_objgroup_label);
  appConfig.vt_debug_phase = is_debug_enabled(vt_debug_phase_label);
  appConfig.vt_debug_context = is_debug_enabled(vt_debug_context_label);
  appConfig.vt_debug_epoch = is_debug_enabled(vt_debug_epoch_label);

  update_config(appConfig.vt_debug_print_flush, vt_debug_print_flush_label, debug_print_configuration);
  update_config(appConfig.vt_debug_replay, vt_debug_replay_label, debug_print_configuration);

  // Load Balancing
  YAML::Node load_balancing = yaml_input["Load Balancing"];
  update_config(appConfig.vt_lb, vt_lb_label, load_balancing);
  update_config(appConfig.vt_lb_quiet, vt_lb_quiet_label, load_balancing);
  update_config(appConfig.vt_lb_file_name, vt_lb_file_name_label, load_balancing);
  update_config(appConfig.vt_lb_show_config, vt_lb_show_config_label, load_balancing);
  update_config(appConfig.vt_lb_name, vt_lb_name_label, load_balancing);
  update_config(appConfig.vt_lb_args, vt_lb_args_label, load_balancing);
  update_config(appConfig.vt_lb_interval, vt_lb_interval_label, load_balancing);
  update_config(appConfig.vt_lb_keep_last_elm, vt_lb_keep_last_elm_label, load_balancing);
  update_config(appConfig.vt_lb_self_migration, vt_lb_self_migration_label, load_balancing);
  update_config(appConfig.vt_lb_spec, vt_lb_spec_label, load_balancing);
  update_config(appConfig.vt_lb_spec_file, vt_lb_spec_file_label, load_balancing);


  YAML::Node lb_output = load_balancing["LB Data Output"];
  update_config(appConfig.vt_lb_data, vt_lb_data_label, lb_output);
  update_config(appConfig.vt_lb_data_dir, vt_lb_data_dir_label, lb_output);
  update_config(appConfig.vt_lb_data_file, vt_lb_data_file_label, lb_output);

  YAML::Node lb_input = load_balancing["LB Data Input"];
  update_config(appConfig.vt_lb_data_in, vt_lb_data_in_label, lb_input);
  update_config(appConfig.vt_lb_data_compress, vt_lb_data_compress_label, lb_input);
  update_config(appConfig.vt_lb_data_dir_in, vt_lb_data_dir_in_label, lb_input);
  update_config(appConfig.vt_lb_data_file_in, vt_lb_data_file_in_label, lb_input);

  YAML::Node lb_stats = load_balancing["LB Statistics"];
  update_config(appConfig.vt_lb_statistics, vt_lb_statistics_label, lb_stats);
  update_config(appConfig.vt_lb_statistics_compress, vt_lb_statistics_compress_label, lb_stats);
  update_config(appConfig.vt_lb_statistics_file, vt_lb_statistics_file_label, lb_stats);
  update_config(appConfig.vt_lb_statistics_dir, vt_lb_statistics_dir_label, lb_stats);

  // Diagnostics
  YAML::Node diagnostics = yaml_input["Diagnostics"];
  update_config(appConfig.vt_diag_enable, vt_diag_enable_label, diagnostics);
  update_config(appConfig.vt_diag_print_summary, vt_diag_print_summary_label, diagnostics);
  update_config(appConfig.vt_diag_summary_file, vt_diag_summary_file_label, diagnostics);
  update_config(appConfig.vt_diag_summary_csv_file, vt_diag_summary_csv_file_label, diagnostics);
  update_config(appConfig.vt_diag_csv_base_units, vt_diag_csv_base_units_label, diagnostics);

  // Termination
  YAML::Node termination = yaml_input["Termination"];
  update_config(appConfig.vt_no_detect_hang, vt_no_detect_hang_label, termination);
  update_config(appConfig.vt_term_rooted_use_ds, vt_term_rooted_use_ds_label, termination);
  update_config(appConfig.vt_term_rooted_use_wave, vt_term_rooted_use_wave_label, termination);
  update_config(appConfig.vt_epoch_graph_on_hang, vt_epoch_graph_on_hang_label, termination);
  update_config(appConfig.vt_epoch_graph_terse, vt_epoch_graph_terse_label, termination);
  update_config(appConfig.vt_print_no_progress, vt_print_no_progress_label, termination);
  update_config(appConfig.vt_hang_freq, vt_hang_freq_label, termination);

  // Debugging/Launch
  YAML::Node launch = yaml_input["Launch"];
  update_config(appConfig.vt_pause, vt_pause_label, launch);

  // User Options
  addUserOptionsFromYaml(yaml_input, appConfig);

  // Scheduler Configuration
  YAML::Node scheduler_configuration = yaml_input["Scheduler Configuration"];
  update_config(appConfig.vt_sched_num_progress, vt_sched_num_progress_label, scheduler_configuration);
  update_config(appConfig.vt_sched_progress_han, vt_sched_progress_han_label, scheduler_configuration);
  update_config(appConfig.vt_sched_progress_sec, vt_sched_progress_sec_label, scheduler_configuration);

  // Configuration File
  YAML::Node configuration_file = yaml_input["Configuration File"];
  update_config(appConfig.vt_output_config, vt_output_config_label, configuration_file);
  update_config(appConfig.vt_output_config_file, vt_output_config_file_label, configuration_file);

  // Runtime
  YAML::Node runtime = yaml_input["Runtime"];
  update_config(appConfig.vt_max_mpi_send_size, vt_max_mpi_send_size_label, runtime);
  update_config(appConfig.vt_no_assert_fail, vt_no_assert_fail_label, runtime);
  update_config(appConfig.vt_throw_on_abort, vt_throw_on_abort_label, runtime);

  // Visualization
  YAML::Node viz = yaml_input["Visualization"];
  update_config(appConfig.vt_tv, vt_tv_label, viz);
  update_config(appConfig.vt_tv_config_file, vt_tv_config_file_label, viz);
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
  auto abort  = "Do not dump stack traces when vtAbort(..) is invoked";
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
  auto dfp = "Enable debug_replay       = \"" debug_pp(replay)       "\"";

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
  auto df = app.add_flag("--vt_debug_replay",       appConfig.vt_debug_replay,       dfp);

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
  df->group(debugGroup);

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
  auto lb_data_hist = "Minimal number of historical LB data phases to retain";
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
  auto lb_first_phase_info = "Force LB to run on the first phase (phase 0)";
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
  auto dr = app.add_option("--vt_lb_data_retention", appConfig.vt_lb_data_retention, lb_data_hist);
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
  auto lb_first_phase = app.add_flag("--vt_lb_run_lb_first_phase", appConfig.vt_lb_run_lb_first_phase, lb_first_phase_info);

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
  dr->group(debugLB);
  yx->group(debugLB);
  yy->group(debugLB);
  yz->group(debugLB);
  zz->group(debugLB);
  lbasm->group(debugLB);
  lbspec->group(debugLB);
  lbspecfile->group(debugLB);
  lb_first_phase->group(debugLB);

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

void addTVArgs(CLI::App& app, AppConfig& appConfig) {
  auto tv_enabled = "Enable vt-tv visualization/mesh streaming";
  auto tv_file = "File name for YAML vt-tv configuraton file";

  auto a1 = app.add_flag("--vt_tv", appConfig.vt_tv, tv_enabled);
  auto a2 = app.add_option(
          "--vt_tv_config_file", appConfig.vt_tv_config_file, tv_file
  );

  auto configTV = "vt-tv Configuration";
  a1->group(configTV);
  a2->group(configTV);
}

void addThreadingArgs(
  [[maybe_unused]] CLI::App& app,
  [[maybe_unused]] AppConfig& appConfig
) {
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

using variantArg_t = std::variant<bool, int32_t, int64_t, double, std::size_t, std::string>;

void addVariantToNode(YAML::Node& node, std::string& key, variantArg_t& variant_val) {
    // Get the yaml_val from the variant
    if (std::holds_alternative<bool>(variant_val)) {
      node[key] = std::get<bool>(variant_val);
    } else if (std::holds_alternative<int32_t>(variant_val)) {
      node[key] = std::get<int32_t>(variant_val);
    } else if (std::holds_alternative<int64_t>(variant_val)) {
      node[key] = std::get<int64_t>(variant_val);
    } else if (std::holds_alternative<double>(variant_val)) {
      node[key] = std::get<double>(variant_val);
    } else if (std::holds_alternative<std::size_t>(variant_val)) {
      node[key] = std::get<std::size_t>(variant_val);
    } else if (std::holds_alternative<std::string>(variant_val)) {
      node[key] = std::get<std::string>(variant_val);
    } else {
      throw std::runtime_error("Argument type not recognized.");
    }
}

std::string convertConfigToYamlString(AppConfig& appConfig) {
  // First, create a converter vector of tuples {Node(s), Key, Value}
  std::vector<std::tuple<std::string, std::string, variantArg_t>> cli_to_yaml_args = {
      // Output Control
      {"Output Control", vt_color_label, static_cast<variantArg_t>(appConfig.vt_color)},
      {"Output Control", vt_no_color_label, static_cast<variantArg_t>(not appConfig.vt_no_color)},
      {"Output Control", vt_quiet_label, static_cast<variantArg_t>(appConfig.vt_quiet)},

      // Signal Handling
      {"Signal Handling", vt_no_sigint_label, static_cast<variantArg_t>(appConfig.vt_no_sigint)},
      {"Signal Handling", vt_no_sigsegv_label, static_cast<variantArg_t>(appConfig.vt_no_sigsegv)},
      {"Signal Handling", vt_no_sigbus_label, static_cast<variantArg_t>(appConfig.vt_no_sigbus)},
      {"Signal Handling", vt_no_terminate_label, static_cast<variantArg_t>(appConfig.vt_no_terminate)},

      // Memory Usage Reporting
      {"Memory Usage Reporting", vt_memory_reporters_label, static_cast<variantArg_t>(appConfig.vt_memory_reporters)},
      {"Memory Usage Reporting", vt_print_memory_each_phase_label, static_cast<variantArg_t>(appConfig.vt_print_memory_each_phase)},
      {"Memory Usage Reporting", vt_print_memory_node_label, static_cast<variantArg_t>(appConfig.vt_print_memory_node)},
      {"Memory Usage Reporting", vt_allow_memory_report_with_ps_label, static_cast<variantArg_t>(appConfig.vt_allow_memory_report_with_ps)},
      {"Memory Usage Reporting", vt_print_memory_threshold_label, static_cast<variantArg_t>(appConfig.vt_print_memory_threshold)},
      {"Memory Usage Reporting", vt_print_memory_sched_poll_label, static_cast<variantArg_t>(appConfig.vt_print_memory_sched_poll)},
      {"Memory Usage Reporting", vt_print_memory_footprint_label, static_cast<variantArg_t>(appConfig.vt_print_memory_footprint)},

      // Dump Stack Backtrace
      {"Dump Stack Backtrace", vt_no_warn_stack_label, static_cast<variantArg_t>(appConfig.vt_no_warn_stack)},
      {"Dump Stack Backtrace", vt_no_assert_stack_label, static_cast<variantArg_t>(appConfig.vt_no_assert_stack)},
      {"Dump Stack Backtrace", vt_no_abort_stack_label, static_cast<variantArg_t>(appConfig.vt_no_abort_stack)},
      {"Dump Stack Backtrace", vt_no_stack_label, static_cast<variantArg_t>(appConfig.vt_no_stack)},
      {"Dump Stack Backtrace", vt_stack_file_label, static_cast<variantArg_t>(appConfig.vt_stack_file)},
      {"Dump Stack Backtrace", vt_stack_dir_label, static_cast<variantArg_t>(appConfig.vt_stack_dir)},
      {"Dump Stack Backtrace", vt_stack_mod_label, static_cast<variantArg_t>(appConfig.vt_stack_mod)},

      // Tracing Configuration
      {"Tracing Configuration", vt_trace_label, static_cast<variantArg_t>(appConfig.vt_trace)},
      {"Tracing Configuration", vt_trace_mpi_label, static_cast<variantArg_t>(appConfig.vt_trace_mpi)},
      {"Tracing Configuration", vt_trace_pmpi_label, static_cast<variantArg_t>(appConfig.vt_trace_pmpi)},
      {"Tracing Configuration", vt_trace_file_label, static_cast<variantArg_t>(appConfig.vt_trace_file)},
      {"Tracing Configuration", vt_trace_dir_label, static_cast<variantArg_t>(appConfig.vt_trace_dir)},
      {"Tracing Configuration", vt_trace_mod_label, static_cast<variantArg_t>(appConfig.vt_trace_mod)},
      {"Tracing Configuration", vt_trace_flush_size_label, static_cast<variantArg_t>(appConfig.vt_trace_flush_size)},
      {"Tracing Configuration", vt_trace_gzip_finish_flush_label, static_cast<variantArg_t>(appConfig.vt_trace_gzip_finish_flush)},
      {"Tracing Configuration", vt_trace_sys_all_label, static_cast<variantArg_t>(appConfig.vt_trace_sys_all)},
      {"Tracing Configuration", vt_trace_sys_term_label, static_cast<variantArg_t>(appConfig.vt_trace_sys_term)},
      {"Tracing Configuration", vt_trace_sys_location_label, static_cast<variantArg_t>(appConfig.vt_trace_sys_location)},
      {"Tracing Configuration", vt_trace_sys_collection_label, static_cast<variantArg_t>(appConfig.vt_trace_sys_collection)},
      {"Tracing Configuration", vt_trace_sys_serial_msg_label, static_cast<variantArg_t>(appConfig.vt_trace_sys_serial_msg)},
      {"Tracing Configuration", vt_trace_spec_label, static_cast<variantArg_t>(appConfig.vt_trace_spec)},
      {"Tracing Configuration", vt_trace_spec_file_label, static_cast<variantArg_t>(appConfig.vt_trace_spec_file)},
      {"Tracing Configuration", vt_trace_memory_usage_label, static_cast<variantArg_t>(appConfig.vt_trace_memory_usage)},
      {"Tracing Configuration", vt_trace_event_polling_label, static_cast<variantArg_t>(appConfig.vt_trace_event_polling)},
      {"Tracing Configuration", vt_trace_irecv_polling_label, static_cast<variantArg_t>(appConfig.vt_trace_irecv_polling)},

      // Debug Print Configuration
      {"Debug Print Configuration", vt_debug_level_label, static_cast<variantArg_t>(appConfig.vt_debug_level)},
      {"Debug Print Configuration", vt_debug_all_label, static_cast<variantArg_t>(appConfig.vt_debug_all)},
      {"Debug Print Configuration", vt_debug_none_label, static_cast<variantArg_t>(appConfig.vt_debug_none)},
      {"Debug Print Configuration", vt_debug_print_flush_label, static_cast<variantArg_t>(appConfig.vt_debug_print_flush)},
      {"Debug Print Configuration/Enable", vt_debug_gen_label, static_cast<variantArg_t>(appConfig.vt_debug_gen)},
      {"Debug Print Configuration/Enable", vt_debug_runtime_label, static_cast<variantArg_t>(appConfig.vt_debug_runtime)},
      {"Debug Print Configuration/Enable", vt_debug_active_label, static_cast<variantArg_t>(appConfig.vt_debug_active)},
      {"Debug Print Configuration/Enable", vt_debug_term_label, static_cast<variantArg_t>(appConfig.vt_debug_term)},
      {"Debug Print Configuration/Enable", vt_debug_termds_label, static_cast<variantArg_t>(appConfig.vt_debug_termds)},
      {"Debug Print Configuration/Enable", vt_debug_barrier_label, static_cast<variantArg_t>(appConfig.vt_debug_barrier)},
      {"Debug Print Configuration/Enable", vt_debug_event_label, static_cast<variantArg_t>(appConfig.vt_debug_event)},
      {"Debug Print Configuration/Enable", vt_debug_pipe_label, static_cast<variantArg_t>(appConfig.vt_debug_pipe)},
      {"Debug Print Configuration/Enable", vt_debug_pool_label, static_cast<variantArg_t>(appConfig.vt_debug_pool)},
      {"Debug Print Configuration/Enable", vt_debug_reduce_label, static_cast<variantArg_t>(appConfig.vt_debug_reduce)},
      {"Debug Print Configuration/Enable", vt_debug_rdma_label, static_cast<variantArg_t>(appConfig.vt_debug_rdma)},
      {"Debug Print Configuration/Enable", vt_debug_rdma_channel_label, static_cast<variantArg_t>(appConfig.vt_debug_rdma_channel)},
      {"Debug Print Configuration/Enable", vt_debug_rdma_state_label, static_cast<variantArg_t>(appConfig.vt_debug_rdma_state)},
      {"Debug Print Configuration/Enable", vt_debug_handler_label, static_cast<variantArg_t>(appConfig.vt_debug_handler)},
      {"Debug Print Configuration/Enable", vt_debug_hierlb_label, static_cast<variantArg_t>(appConfig.vt_debug_hierlb)},
      {"Debug Print Configuration/Enable", vt_debug_temperedlb_label, static_cast<variantArg_t>(appConfig.vt_debug_temperedlb)},
      {"Debug Print Configuration/Enable", vt_debug_temperedwmin_label, static_cast<variantArg_t>(appConfig.vt_debug_temperedwmin)},
      {"Debug Print Configuration/Enable", vt_debug_scatter_label, static_cast<variantArg_t>(appConfig.vt_debug_scatter)},
      {"Debug Print Configuration/Enable", vt_debug_serial_msg_label, static_cast<variantArg_t>(appConfig.vt_debug_serial_msg)},
      {"Debug Print Configuration/Enable", vt_debug_trace_label, static_cast<variantArg_t>(appConfig.vt_debug_trace)},
      {"Debug Print Configuration/Enable", vt_debug_location_label, static_cast<variantArg_t>(appConfig.vt_debug_location)},
      {"Debug Print Configuration/Enable", vt_debug_lb_label, static_cast<variantArg_t>(appConfig.vt_debug_lb)},
      {"Debug Print Configuration/Enable", vt_debug_vrt_label, static_cast<variantArg_t>(appConfig.vt_debug_vrt)},
      {"Debug Print Configuration/Enable", vt_debug_vrt_coll_label, static_cast<variantArg_t>(appConfig.vt_debug_vrt_coll)},
      {"Debug Print Configuration/Enable", vt_debug_worker_label, static_cast<variantArg_t>(appConfig.vt_debug_worker)},
      {"Debug Print Configuration/Enable", vt_debug_group_label, static_cast<variantArg_t>(appConfig.vt_debug_group)},
      {"Debug Print Configuration/Enable", vt_debug_broadcast_label, static_cast<variantArg_t>(appConfig.vt_debug_broadcast)},
      {"Debug Print Configuration/Enable", vt_debug_objgroup_label, static_cast<variantArg_t>(appConfig.vt_debug_objgroup)},
      {"Debug Print Configuration/Enable", vt_debug_phase_label, static_cast<variantArg_t>(appConfig.vt_debug_phase)},
      {"Debug Print Configuration/Enable", vt_debug_context_label, static_cast<variantArg_t>(appConfig.vt_debug_context)},
      {"Debug Print Configuration/Enable", vt_debug_epoch_label, static_cast<variantArg_t>(appConfig.vt_debug_epoch)},

      // Load Balancing
      {"Load Balancing", vt_lb_label, static_cast<variantArg_t>(appConfig.vt_lb)},
      {"Load Balancing", vt_lb_quiet_label, static_cast<variantArg_t>(appConfig.vt_lb_quiet)},
      {"Load Balancing", vt_lb_file_name_label, static_cast<variantArg_t>(appConfig.vt_lb_file_name)},
      {"Load Balancing", vt_lb_show_config_label, static_cast<variantArg_t>(appConfig.vt_lb_show_config)},
      {"Load Balancing", vt_lb_name_label, static_cast<variantArg_t>(appConfig.vt_lb_name)},
      {"Load Balancing", vt_lb_args_label, static_cast<variantArg_t>(appConfig.vt_lb_args)},
      {"Load Balancing", vt_lb_interval_label, static_cast<variantArg_t>(appConfig.vt_lb_interval)},
      {"Load Balancing", vt_lb_keep_last_elm_label, static_cast<variantArg_t>(appConfig.vt_lb_keep_last_elm)},
      {"Load Balancing/LB Data Output", vt_lb_data_label, static_cast<variantArg_t>(appConfig.vt_lb_data)},
      {"Load Balancing/LB Data Output", vt_lb_data_dir_label, static_cast<variantArg_t>(appConfig.vt_lb_data_dir)},
      {"Load Balancing/LB Data Output", vt_lb_data_file_label, static_cast<variantArg_t>(appConfig.vt_lb_data_file)},
      {"Load Balancing/LB Data Input", vt_lb_data_in_label, static_cast<variantArg_t>(appConfig.vt_lb_data_in)},
      {"Load Balancing/LB Data Input", vt_lb_data_compress_label, static_cast<variantArg_t>(appConfig.vt_lb_data_compress)},
      {"Load Balancing/LB Data Input", vt_lb_data_dir_in_label, static_cast<variantArg_t>(appConfig.vt_lb_data_dir_in)},
      {"Load Balancing/LB Data Input", vt_lb_data_file_in_label, static_cast<variantArg_t>(appConfig.vt_lb_data_file_in)},
      {"Load Balancing/LB Statistics", vt_lb_statistics_label, static_cast<variantArg_t>(appConfig.vt_lb_statistics)},
      {"Load Balancing/LB Statistics", vt_lb_statistics_compress_label, static_cast<variantArg_t>(appConfig.vt_lb_statistics_compress)},
      {"Load Balancing/LB Statistics", vt_lb_statistics_file_label, static_cast<variantArg_t>(appConfig.vt_lb_statistics_file)},
      {"Load Balancing/LB Statistics", vt_lb_statistics_dir_label, static_cast<variantArg_t>(appConfig.vt_lb_statistics_dir)},
      {"Load Balancing", vt_lb_self_migration_label, static_cast<variantArg_t>(appConfig.vt_lb_self_migration)},
      {"Load Balancing", vt_lb_spec_label, static_cast<variantArg_t>(appConfig.vt_lb_spec)},
      {"Load Balancing", vt_lb_spec_file_label, static_cast<variantArg_t>(appConfig.vt_lb_spec_file)},

      // Diagnostics
      {"Diagnostics", vt_diag_enable_label, static_cast<variantArg_t>(appConfig.vt_diag_enable)},
      {"Diagnostics", vt_diag_print_summary_label, static_cast<variantArg_t>(appConfig.vt_diag_print_summary)},
      {"Diagnostics", vt_diag_summary_file_label, static_cast<variantArg_t>(appConfig.vt_diag_summary_file)},
      {"Diagnostics", vt_diag_summary_csv_file_label, static_cast<variantArg_t>(appConfig.vt_diag_summary_csv_file)},
      {"Diagnostics", vt_diag_csv_base_units_label, static_cast<variantArg_t>(appConfig.vt_diag_csv_base_units)},

      // Termination
      {"Termination", vt_no_detect_hang_label, static_cast<variantArg_t>(appConfig.vt_no_detect_hang)},
      {"Termination", vt_term_rooted_use_ds_label, static_cast<variantArg_t>(appConfig.vt_term_rooted_use_ds)},
      {"Termination", vt_term_rooted_use_wave_label, static_cast<variantArg_t>(appConfig.vt_term_rooted_use_wave)},
      {"Termination", vt_epoch_graph_on_hang_label, static_cast<variantArg_t>(appConfig.vt_epoch_graph_on_hang)},
      {"Termination", vt_epoch_graph_terse_label, static_cast<variantArg_t>(appConfig.vt_epoch_graph_terse)},
      {"Termination", vt_print_no_progress_label, static_cast<variantArg_t>(appConfig.vt_print_no_progress)},
      {"Termination", vt_hang_freq_label, static_cast<variantArg_t>(appConfig.vt_hang_freq)},

      // Debugging/Launch
      {"Launch", vt_pause_label, static_cast<variantArg_t>(appConfig.vt_pause)},

      // User Options
      {"User Options", user_args_labels["vt_user_1"], static_cast<variantArg_t>(appConfig.vt_user_1)},
      {"User Options", user_args_labels["vt_user_2"], static_cast<variantArg_t>(appConfig.vt_user_2)},
      {"User Options", user_args_labels["vt_user_3"], static_cast<variantArg_t>(appConfig.vt_user_3)},
      {"User Options", user_args_labels["vt_user_int_1"], static_cast<variantArg_t>(appConfig.vt_user_int_1)},
      {"User Options", user_args_labels["vt_user_int_2"], static_cast<variantArg_t>(appConfig.vt_user_int_2)},
      {"User Options", user_args_labels["vt_user_int_3"], static_cast<variantArg_t>(appConfig.vt_user_int_3)},
      {"User Options", user_args_labels["vt_user_str_1"], static_cast<variantArg_t>(appConfig.vt_user_str_1)},
      {"User Options", user_args_labels["vt_user_str_2"], static_cast<variantArg_t>(appConfig.vt_user_str_2)},
      {"User Options", user_args_labels["vt_user_str_3"], static_cast<variantArg_t>(appConfig.vt_user_str_3)},

      // Scheduler Configuration
      {"Scheduler Configuration", vt_sched_num_progress_label, static_cast<variantArg_t>(appConfig.vt_sched_num_progress)},
      {"Scheduler Configuration", vt_sched_progress_han_label, static_cast<variantArg_t>(appConfig.vt_sched_progress_han)},
      {"Scheduler Configuration", vt_sched_progress_sec_label, static_cast<variantArg_t>(appConfig.vt_sched_progress_sec)},

      // Configuration File
      {"Configuration File", vt_output_config_label, static_cast<variantArg_t>(appConfig.vt_output_config)},
      {"Configuration File", vt_output_config_file_label, static_cast<variantArg_t>(appConfig.vt_output_config_file)},

      // Runtime
      {"Runtime", vt_max_mpi_send_size_label, static_cast<variantArg_t>(appConfig.vt_max_mpi_send_size)},
      {"Runtime", vt_no_assert_fail_label, static_cast<variantArg_t>(appConfig.vt_no_assert_fail)},
      {"Runtime", vt_throw_on_abort_label, static_cast<variantArg_t>(appConfig.vt_throw_on_abort)},

      // Visualization
      {"Visualization", vt_tv_label, static_cast<variantArg_t>(appConfig.vt_tv)},
      {"Visualization", vt_tv_config_file_label, static_cast<variantArg_t>(appConfig.vt_tv_config_file)}
  };

  // Create an empty node that we will populate
  YAML::Node output_config_yaml;

  // Then convert to YAML
  for (const auto& yaml_data : cli_to_yaml_args) {

    // Unpack the yaml data
    auto [yaml_node, yaml_key, yaml_val] = yaml_data;

    // First, explicitly handle the Debug Print Configuration list
    if (yaml_node == "Debug Print Configuration/Enable") {
      if (std::get<bool>(yaml_val)) {
        output_config_yaml["Debug Print Configuration"]["Enable"].push_back(yaml_key);
      }
    }
    // Then handle the User Defined parameters
    else if (yaml_node == "User Options" and yaml_key != "unused_user_param") {
      auto current_node = output_config_yaml["User Options"];
      addVariantToNode(current_node, yaml_key, yaml_val);
    }
    // Then handle any nested nodes (with "/" in them)
    else if (yaml_node.find("/") != yaml_node.npos) {
      auto nodes = util::demangle::DemanglerUtils::splitString(yaml_node, '/');
      auto current_node = output_config_yaml[nodes[0]][nodes[1]];
      addVariantToNode(current_node, yaml_key, yaml_val);
    }
    // The rest are straightforward
    else {
      auto current_node = output_config_yaml[yaml_node];
      addVariantToNode(current_node, yaml_key, yaml_val);
    }
  }
  std::ostringstream yaml_stream;
  yaml_stream << output_config_yaml;
  return yaml_stream.str();
}

void convertConfigToString(CLI::App& app, AppConfig& appConfig) {
  auto output_file = appConfig.vt_output_config_file;
  auto config_file_ending = output_file.substr(output_file.size()-4);
  if (config_file_ending == ".yml" || config_file_ending == "yaml") {
    appConfig.vt_output_config_str = convertConfigToYamlString(appConfig);
  } else {
    appConfig.vt_output_config_str = app.config_to_str(true, true);
  }
}

} /* end anon namespace */

/*static*/ std::unique_ptr<ArgConfig>
ArgConfig::construct(std::unique_ptr<ArgConfig> arg) {
  return arg;
}

class VtFormatter : public CLI::Formatter {
public:
  std::string make_usage(
    const CLI::App *, [[maybe_unused]] std::string name
  ) const override {
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
"\n"
"VT arguments can also be provided via configuration file. Pass a TOML or ini config\n"
"file with --vt_input_config, or a YAML config file with --vt_input_config_yaml.\n"
"Command line arguments will overwrite any configuration file (and a TOML or ini\n"
"file will overwrite a YAML configuration).\n"
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

  // Allow a input config file
  app.set_config(
    "--vt_input_config",
    "", // no default file name
    "Read in an ini or toml config file for VT",
    false // not required
  );

  // Set up CLI parsing
  addYamlArgs(app, appConfig);
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
  addTVArgs(app, appConfig);
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
