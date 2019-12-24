/*
//@HEADER
// *****************************************************************************
//
//                                    args.h
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

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H

// Do not pull in any VT dependencies here

#include <map>
#include <set>
#include <string>
#include <unordered_map>


namespace CLI {
class App;
}

namespace vt { namespace arguments {

struct Configs {
public:
  bool vt_color = true;
  bool vt_no_color = false;
  bool vt_auto_color = false;
  bool vt_quiet = false;

  // Derived from vt_*_color arguments after parsing.
  bool colorize_output = false;
  int32_t vt_sched_num_progress = 2;
  int32_t vt_sched_progress_han = 0;
  double vt_sched_progress_sec = 0.0;
  bool vt_no_sigint = false;
  bool vt_no_sigsegv = false;
  bool vt_no_terminate = false;

  bool vt_no_warn_stack = false;
  bool vt_no_assert_stack = false;
  bool vt_no_abort_stack = false;
  bool vt_no_stack = false;
  std::string vt_stack_file = {};
  std::string vt_stack_dir = {};
  int32_t vt_stack_mod = 1;

  bool vt_trace = false;
  bool vt_trace_mpi = false;
  std::string vt_trace_file = {};
  std::string vt_trace_dir = {};
  int32_t vt_trace_mod = 1;
  int32_t vt_trace_flush_size = 1;

  bool vt_lb = false;
  bool vt_lb_file = false;
  bool vt_lb_quiet = false;
  std::string vt_lb_file_name = "balance.in";
  std::string vt_lb_name = "NoLB";
  std::string vt_lb_args = {};
  int32_t vt_lb_interval = 1;
  bool vt_lb_stats = false;
  std::string vt_lb_stats_dir = "vt_lb_stats";
  std::string vt_lb_stats_file = "stats";

  bool vt_no_detect_hang = false;
  bool vt_print_no_progress = true;
  bool vt_epoch_graph_on_hang = true;
  bool vt_epoch_graph_terse = false;
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
  bool vt_debug_gossiplb = false;
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
  std::string vt_user_str_1 = {};
  std::string vt_user_str_2 = {};
  std::string vt_user_str_3 = {};
};


/// \brief Enum for the Context where the Parameters is set
enum struct ContextEnum : std::uint8_t { dFault, commandLine, thirdParty };


/// \brief Enum for the Print Status
enum struct PrintOption : std::uint8_t { never = 0, whenSet = 2, always = 255 };


/// \brief Forward declaration
struct Printer;


/// \brief Virtual struct for storing an option/flag
struct AnchorBase : public std::enable_shared_from_this<AnchorBase> {

  /// \brief Count how many instances of the parameter 'name'
  /// have been specified.
  /// \param[in] name Parameter to specify
  virtual int count() const = 0;

  /// \brief Virtual function to print information
  virtual void print() = 0;

  /// \brief Resets to the default value
  virtual void resetToDefault() = 0;

  /// \brief Virtual function to resolve precedence rules
  virtual void resolve() = 0;

  /// \brief Set the printing option for the anchor
  ///
  /// \param[in] po Enum PrintOption for the anchor
  /// \param[in] fun Function returning a boolean to constraint the printing
  ///
  virtual void
  setPrintOption(PrintOption po, std::function<bool()> fun) = 0;

  /// \brief Virtual function returning resolved value
  /// \returns Resolved value
  virtual std::string stringifyValue() const = 0;

  /// \brief Virtual function returning context for resolved value
  /// \returns Context
  virtual std::string stringifyContext() const = 0;

  /// \brief Virtual function returning context for resolved value
  /// \returns Default value
  virtual std::string stringifyDefault() const = 0;

  //--------------------------------------------//
  // Non-virtual member functions
  //--------------------------------------------//

  /// \brief Sets excluded option
  ///
  /// \param[in] opt  Pointer to the option to exclude
  void excludes(const std::shared_ptr<AnchorBase>& opt) {
    excludes_.insert(opt);
    // Help text should be symmetric - excluding a should exclude b
    opt->excludes_.insert(shared_from_this());
    //
    // Ignoring the insert return value, excluding twice (or more) is allowed.
    //
  }

  /// \brief Returns the description of the anchor
  /// \return Description
  std::string getDescription() const { return description_; }

  /// \brief Returns the group name of the anchor
  /// \return Group
  std::string getGroup() const { return group_; }

  /// \brief Returns the name of the anchor
  /// \return Name of the option
  std::string getName() const { return name_; }

  /// \brief Function to determine whether the precedence rules
  /// have been applied or not.
  /// \returns Boolean
  bool isResolved() const { return isResolved_; }

  /// \brief Sets dependence on a specific flag being turned off
  ///
  /// \param[in] opt  Pointer to the flag to depend on
  void needsOptionOff(std::shared_ptr<AnchorBase> const& opt) {
    needsOptOff_.insert(opt);
  }

  /// \brief Sets dependence on a specific flag being turned on
  ///
  /// \param[in] opt  Pointer to the flag to depend on
  void needsOptionOn(std::shared_ptr<AnchorBase> const& opt) {
    needsOptOn_.insert(opt);
  }

  /// \brief Set the group name for the anchor
  ///
  /// \param[in] grp_ String for the name of the group
  void setGroup(const std::string& grp_) { group_ = grp_; }

protected:
  /// \brief Constructor
  ///
  /// \param[in] name Label for the anchor
  /// \param[in] desc Description of the anchor
  ///
  /// \note The anchor will be assigned to a default group, named 'Default'.
  ///
  AnchorBase(std::string name, std::string desc, std::string grp);

  /// \brief Set the printing option for the anchor
  ///
  /// \param[in] po Enum PrintOption for the anchor
  /// \param[in] fun Function returning a boolean to constraint the printing
  ///
  virtual void
  setPrintOptionImpl(PrintOption po, std::function<bool()> fun) = 0;

protected:

  //--- Build enum for ordering the context
  //--- Order of the contexts can be specified
  //--- independently of an instance for that context.
  enum struct OrderContextEnum : std::uint8_t {
    MIN = 0,
    dFault = 1,
    commandLine = 2,
    thirdParty = 3,
    MAX = 255
  };

  //--- Map from 'ContextEnum' flag to an ordered flag.
  static std::map<ContextEnum, OrderContextEnum> emap_;

  //--- Map from 'ContextEnum' flag to a descriptive string.
  static std::map<ContextEnum, std::string> smap_;

  bool isResolved_;

  std::string name_;
  std::string group_;
  std::string description_;

  /// \brief Map ordering the different instances of an 'anchor'
  std::map<ContextEnum, OrderContextEnum> ordering_;

  /// \brief List of options that are needed to be OFF for this option
  std::set<std::shared_ptr<AnchorBase>> needsOptOff_;

  /// \brief List of options that are needed to be ON for this option
  std::set<std::shared_ptr<AnchorBase>> needsOptOn_;

  /// \brief List of options that are excluded with this option
  std::set<std::shared_ptr<AnchorBase>> excludes_;

  /// \brief Printing options for the anchor
  PrintOption statusPrint_ = PrintOption::never;

  /// \brief Pointer to 'Printer' object for displaying message
  /// on the startup banner.
  std::unique_ptr<Printer> screenPrint_;

};


/// \brief Template struct for storing an option/flag
template <typename T>
struct Anchor : public AnchorBase {

public:
  /// \brief Constructor
  ///
  /// \param[in] var Variable to specify and whose initial value
  ///                defines the default value
  /// \param[in] name Label for the parameter of interest
  /// \param[in] desc String describing the anchor
  /// \param[in] grp  String for the group owning the anchor
  ///
  explicit Anchor(
    T& var, const std::string& name, const std::string& desc,
    const std::string& grp = "Default"
  );

  /// \brief Add instance from a third party with a specific value
  ///
  /// \param[in] value Value to specify for the third party context
  /// \param[in] highestPrecedence Boolean determining whether the instance
  /// should have the highest priority (default = false)
  void addInstance(const T& value, bool highestPrecedence = false);

  /// \brief Counts the current number of instances for the option
  ///
  /// \returns Number of instances for the option
  int count() const override {
    return static_cast<int>(specifications_.size());
  }

  /// \brief Sets highest priority (precedence) for a context
  ///
  /// \param[in] origin Context for the highest priority
  ///
  /// \note If a different context had already the highest priority,
  /// this context priority will be reset to its original value.
  ///
  void setHighestPrecedence(const ContextEnum& origin);

  /// \brief Sets a new default value.
  ///
  /// \param ref Value to assign as default
  void setNewDefaultValue(const T& ref);

  /// \brief Set the printing option for the anchor
  ///
  /// \param[in] po Enum PrintOption for the anchor
  /// \param[in] fun Function returning a boolean to constraint the printing
  ///
  void setPrintOption(PrintOption po, std::function<bool()> fun) override;

  /// \brief Returns the default value for the anchor
  ///
  /// \returns Default value of the anchor
  const T getDefaultValue() const;

  /// \brief Returns the value for the current instance
  ///
  /// \returns Value of the current instance
  const T& getValue() const { return value_; }

  /// \brief Virtual function returning resolved value
  /// \returns Resolved value
  std::string stringifyValue() const override;

  /// \brief Virtual function returning context for resolved value
  /// \returns String for resolved context
  std::string stringifyContext() const override;

  /// \brief Virtual function returning default value
  /// \returns String for default value
  std::string stringifyDefault() const override;

  /// \brief Resets to the default value
  void resetToDefault() override;

  //
  //--- Printing routines
  //

  /// \brief Printing routine about the anchor
  void print() override;

  /// \brief Set the printing message for the banner as a warning
  ///
  /// \param[in] String to display for the anchor
  ///
  /// \note The message will be of the form
  /// "Warning: 'NAME' has no effect: compile-time feature 'MSG' is disabled"
  ///
  void setBannerMsgWarning(std::string msg,
    std::function<bool()> fun = nullptr);

protected:
  /// \brief Add instance from a particular context
  ///
  /// \param[in] ctxt Context for the instance
  /// \param[in] value Value specified by the instance
  ///
  void addGeneralInstance(ContextEnum ctxt, const T& value);

  /// \brief Checks whether two 'excluded' options have specifications
  ///         in addition to their default ones.
  void checkExcludes() const;

  /// \brief Resolves the precedence rules among the instances
  void resolve() override;

  /// \brief Set the printing option for the anchor
  ///
  /// \param[in] po Enum PrintOption for the anchor
  /// \param[in] fun Function returning a boolean to constraint the printing
  ///
  void setPrintOptionImpl(PrintOption po, std::function<bool()> fun) override;

//  friend struct ArgSetup;

protected:

  //--- Structure for storing each instance of one anchor
  template <typename U = T>
  struct Instance {

  public:
    /// \brief Constructor
    ///
    /// \param[in] ref Value of the anchor for this instance
    /// \param[in] parent Pointer for the parent anchor
    explicit Instance(const U& rval, AnchorBase* parent)
      : value_(rval), parent_(parent)
    {}

    /// \brief Dummy Constructor
    Instance() : value_(), parent_(nullptr)
    {}

    /// \brief Copy Constructor
    Instance(const Instance<U>& ref)
      : value_(ref.value_), parent_(ref.parent_)
    {}

    /// \brief Destructor
    ~Instance() = default;

    /// \brief Assignment operator
    Instance<U>& operator=(const Instance<U>& ref) {
      value_ = ref.value_;
      parent_ = ref.parent_;
      return *this;
    }

    /// \brief Sets new value for the instance
    ///
    /// \param ref New value to insert for the instance
    void setNewValue(const U& ref) { value_ = ref; }

    /// \brief Returns the value for the current instance
    ///
    /// \returns Value of the current instance
    const U& getValue() const { return value_; }

  protected:
    U value_;
    AnchorBase* parent_ = nullptr;
  };
  //---

  T& value_ = {};
  std::unordered_map<ContextEnum, Instance<T>> specifications_;
  bool hasNewDefault_ = false;
  ContextEnum resolvedContext_ = ContextEnum::dFault;
  Instance<T> resolvedInstance_;
  bool resolvedToDefault_ = false;

};


struct Args {

public:
  static Configs config;

public:

  static int parse(int& argc, char**& argv);

  /// \brief Returns pointer to the option object for specific name.
  ///
  /// \tparam T  Template type for the value of the parameter
  /// \param[in] name Label for the parameter of interest
  /// \return Pointer to the anchor with the specified label
//  template <typename T>
//  std::shared_ptr<Anchor<T>> getOption(const std::string& name) const;

  /// \brief Set new default value for a specified name
  ///
  /// \tparam T  Template type for the value of the parameter
  /// \param[in] name String labelling the option
  /// \param[in] anchor_value Variable to specify and whose initial value
  ///                         defines the default value
  /// \param[in] desc String describing the option
  ///
  /// \return Pointer to the anchor with the specified label
  ///
//  template <typename T>
//  std::shared_ptr<Anchor<T>>
//  setNewDefaultValue(const std::string& name, const T& anchor_value);

  //
  // --- Printing routines
  //

  /// \brief Print the list of options and instances
  ///
//  void printBanner();

  /// \brief Create string to output the configuration, i.e.
  /// the values of all the options.
  ///
  /// \param[in] default_also Boolean to identify whether or not to print
  /// default value
  /// \param[in] write_description Boolean to turn on/off the printing
  /// of the parameter description
  /// \param[in] prefix String containing a prefix to add before each
  /// option name
  ///
  /// \return String describing all the options
  ///
  /// \note Creates a string in the ".INI" formatted
  /// (INI format: https://cliutils.gitlab.io/CLI11Tutorial/chapters/config.html
  /// )
//  std::string outputConfig(
//    bool default_also, bool write_description, std::string prefix) const;

private:

  static bool parsed;

  static void setup(CLI::App *app_);

};

inline bool user1() { return Args::config.vt_user_1; }
inline bool user2() { return Args::config.vt_user_2; }
inline bool user3() { return Args::config.vt_user_3; }

}} /* end namespace vt::arguments */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
