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

#include <map>
#include <memory>
#include <set>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

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

  bool vt_no_sigint = false;
  bool vt_no_sigsegv = false;
  bool vt_no_terminate = false;

  bool vt_no_warn_stack = false;
  bool vt_no_assert_stack = false;
  bool vt_no_abort_stack = false;
  bool vt_no_stack = false;
  std::string vt_stack_file = "";
  std::string vt_stack_dir = "";
  int32_t vt_stack_mod = 1;

  bool vt_trace = false;
  std::string vt_trace_file = "";
  std::string vt_trace_dir = "";
  int32_t vt_trace_mod = 1;

  bool vt_lb = false;
  bool vt_lb_file = false;
  bool vt_lb_quiet = false;
  std::string vt_lb_file_name = "balance.in";
  std::string vt_lb_name = "NoLB";
  int32_t vt_lb_interval = 1;
  bool vt_lb_stats = false;
  std::string vt_lb_stats_dir = "vt_lb_stats";
  std::string vt_lb_stats_file = "stats";

  bool vt_no_detect_hang = false;
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
  std::string vt_user_str_1 = "";
  std::string vt_user_str_2 = "";
  std::string vt_user_str_3 = "";
};


//--- Build enum for the context
enum struct ContextEnum : std::uint8_t { dFault, commandLine, thirdParty };

/// \brief Virtual struct for storing an option/flag
struct AnchorBase : public std::enable_shared_from_this<AnchorBase> {

  /// \brief Count how many instances of the parameter 'name'
  /// have been specified.
  /// \param[in] name Parameter to specify
  virtual int count() const = 0;

  /// \brief Virtual function determining whether precedence rules
  /// have been resolved
  virtual bool isResolved() const = 0;

  /// \brief Virtual function to print information
  virtual void print() = 0;

  /// \brief Virtual function to resolve precedence rules
  virtual void resolve() = 0;

  /// \brief Virtual function returning resolved value
  /// \returns Resolved value
  virtual std::string stringifyValue() const = 0;

  /// \brief Virtual function returning context for resolved value
  /// \returns Context
  virtual std::string stringifyContext() const = 0;

  /// \brief Virtual function returning context for resolved value
  /// \returns Default value
  virtual std::string stringifyDefault() const = 0;

  /// \brief Default destructor
  virtual ~AnchorBase() = default;

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

  /// \brief Resets to the default value
  virtual void resetToDefault() = 0;

  /// \brief Set the group name for the option
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
  AnchorBase(std::string name, std::string desc)
    : name_(std::move(name)), group_("Default"), description_(std::move(desc)),
      ordering_(), needsOptOff_(), needsOptOn_(), excludes_() {}

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
};

struct Printer;

/// \brief Template struct for storing an option/flag
template <typename T>
struct Anchor : public AnchorBase {

  public:
  /// \brief Constructor
  ///
  /// \param[in] var Variable to specify and whose initial value
  ///                defines the default value
  /// \param[in] name Label for the parameter of interest
  /// \param[in] desc String describing the option
  ///
  explicit Anchor(T& var, const std::string& name, const std::string& desc);

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

  /// \brief Sets a new default value.
  ///
  /// \param ref Value to assign as default
  void setNewDefaultValue(const T& ref);

  /// \brief Sets highest priority (precedence) for a context
  ///
  /// \param[in] origin Context for the highest priority
  ///
  /// \note If a different context had already the highest priority,
  /// this context priority will be reset to its original value.
  ///
  void setHighestPrecedence(const ContextEnum& origin);

  /// \brief Returns the default value for the anchor
  ///
  /// \returns Default value of the anchor
  const T getDefaultValue() const;

  /// \brief Returns the value for the current instance
  ///
  /// \returns Value of the current instance
  const T& getValue() const { return value_; }

  /// \brief Function to determine whether the precedence rules
  /// have been applied or not.
  /// \returns Boolean
  bool isResolved() const override { return isResolved_; }

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
  /// \param[in] msg_on String to display for the anchor when it is active
  /// \param[in] fun  Additional condition to print or not the message
  ///
  /// \note The ON message will be of the form
  /// "Option: flag 'NAME' on: 'MSG_ON'"
  ///
  void setBannerMsgOn(std::string msg_on, std::function<bool()> fun = nullptr);

  /// \brief Set the printing message for the banner as a warning
  ///
  /// \param[in] msg_on String to display when the anchor is active
  /// \param[in] msg_off String to display when the anchor is off
  /// \param[in] fun Additional condition to print or not the message
  ///
  /// \note The ON message will be of the form
  /// "Option: flag 'NAME' on: 'MSG_ON'"
  /// \note The OFF message will be of the form
  /// "Default: 'MSG_OFF', use 'NAME' to disable"
  ///
  void setBannerMsgOnOff(
    std::string msg_on, std::string msg_off,
    std::function<bool()> fun = nullptr);

  /// \brief Set the printing message for the banner as a warning
  ///
  /// \param[in] String to display for the anchor
  ///
  /// \note The message will be of the form
  /// "Warning: 'NAME' has no effect: compile-time feature 'MSG' is disabled"
  ///
  void
  setBannerMsgWarning(std::string msg, std::function<bool()> fun = nullptr);

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

  friend struct ArgSetup;

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
      : value_(rval), parent_(parent) {}

    /// \brief Dummy Constructor
    Instance() : value_(), parent_(nullptr) {}

    /// \brief Copy Constructor
    Instance(const Instance<U>& ref)
      : value_(ref.value_), parent_(ref.parent_) {}

    /// \brief Destructor
    ~Instance() {}

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
  bool isResolved_ = false;
  ContextEnum resolvedContext_ = ContextEnum::dFault;
  Instance<T> resolvedInstance_;
  bool resolvedToDefault_ = false;

  //--- Pointer to 'Printer' object for displaying
  //--- message on the startup banner.
  std::unique_ptr<Printer> azerty_ = nullptr;
};


/// \brief Structure to store all the different configuration parameters
///
/// \note This struct provides the interface between VT configuration parameters
/// and an external code (like a third-party code linking with VT).
///
/// \note When an external code wants to specify a VT configuration parameter
/// (for example, after reading its own input file), the developer should write
///
///   auto ptr = Args::setup.getOption(NAME);
///   if (ptr)
///     ptr->addInstance(VALUE);
///
///  When the external code wants to specify a new default value, we write
///
///   auto ptr = Args::setup.getOption(NAME);
///   if (ptr)
///     ptr->setNewDefaultValue(VALUE);
///
struct ArgSetup {

  public:
  /// \brief Constructor
  /// \param[in] Description of the application as a string
  ArgSetup(std::string description = "");

  /// \brief Returns pointer to the option object for specific name.
  ///
  /// \tparam T  Template type for the value of the parameter
  /// \param[in] name Label for the parameter of interest
  /// \return Pointer to the anchor with the specified label
  template <typename T>
  std::shared_ptr<Anchor<T>> getOption(const std::string& name) const;

  /// \brief Add flag for boolean or integral flag
  ///
  /// \tparam T  Template type for the value of the parameter
  /// \param[in] name Label for the parameter of interest
  /// \param[in] anchor_value Variable to specify and whose initial value
  ///                         defines the default value
  /// \param[in] desc String describing the option
  ///
  /// \return Pointer to the anchor with the specified label
  ///
  /// \note Creates two instances one for the default value
  /// (with the current value in 'anchor_value') and
  /// one for the command line.
  ///
  /// \note Matches the argument interface as CLI::App
  ///
  template <
    typename T,
    typename = typename std::enable_if<
      std::is_same<T, bool>::value || std::is_same<T, int>::value, T>>
  std::shared_ptr<Anchor<T>> addFlag(
    const std::string& name, T& anchor_value, const std::string& desc = {});

  /// \brief Add option for a specified name
  ///
  /// \tparam T  Template type for the value of the parameter
  /// \param[in] name String labelling the option
  /// \param[in] anchor_value Variable to specify and whose initial value
  ///                         defines the default value
  /// \param[in] desc String describing the option
  ///
  /// \return Pointer to the anchor with the specified label
  ///
  /// \note Creates two instances one for the default value
  /// (with the current value in 'anchor_value') and
  /// one for the command line.
  ///
  /// \note Matches the argument interface as CLI::App
  ///
  template <typename T>
  std::shared_ptr<Anchor<T>> addOption(
    const std::string& name, T& anchor_value, const std::string& desc = "");

  /// \brief Parse the command line inputs and resolve the precedence rules
  ///
  /// \param[in,out] argc Number of arguments
  /// \param[in,out] argv Array of arguments
  ///
  /// \note When exiting, argc and argv will be modified in presence of
  /// redundancy.
  ///
  /// \note At exit, the configuration parameters will be set.
  ///
  void parseResolve(int& argc, char**& argv);

  //
  // --- Printing routines
  //

  /// \brief Print the list of options and instances
  ///
  void printBanner();

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
  template <typename T>
  std::shared_ptr<Anchor<T>>
  setNewDefaultValue(const std::string& name, const T& anchor_value);

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
  std::string outputConfig(
    bool default_also, bool write_description, std::string prefix) const;

  protected:
  /// \brief Add a flag to CLI setup
  ///
  /// \param[in] Pointer to the specific anchor
  /// \param[in] sname String labelling the option
  /// \param[in] anchor_value Variable to specify and whose initial value
  ///                         defines the default value
  /// \param[in] desc String describing the option
  ///
  template <typename T>
  void addFlagToCLI(
    Anchor<T>* aptr, const std::string& sname, T& anchor_value,
    const std::string& desc) {}

  /// \brief Gather the list of all group names
  ///
  /// \returns List of group names
  std::vector<std::string> getGroupList() const;

  /// \brief Get the list of options for a given group name
  ///
  /// \param[in] gname Name of group to study
  /// \returns List of options in the group
  std::map<std::string, std::shared_ptr<AnchorBase>>
  getGroupOptions(const std::string& gname) const;

  /// \brief Parse the command line arguments
  ///
  /// \param[in,out] argc Number of arguments
  /// \param[in,out] argv Array of arguments
  ///
  /// \note On exit, argc and argv will be modified in presence of redundancy.
  ///
  void parse(int& argc, char**& argv);

  /// \brief Resolve the different options by applying all precedence rules
  ///
  void resolveOptions() {
    for (const auto& opt : options_) {
      opt.second->resolve();
    }
  }

  /// \brief Setup printing banner messages
  ///
  void prepareOptionsPrinting();

  /// \brief Function to verify that a string is acceptable
  ///
  /// \param[in] name String to verifyName
  ///
  /// \return 'Cleaned-up' string
  ///
  /// \note This function removes any '-' characters at the start
  /// and verify that the remaining characters are acceptable.
  ///
  std::string verifyName(const std::string& name) const;

  private:
  std::unique_ptr<CLI::App> app_;
  std::map<std::string, std::shared_ptr<AnchorBase>> options_ = {};

  public:
  bool parsed;
};


struct Args {

  public:
  static ArgSetup setup;
  static Configs config;

  public:
  /// \brief Routine to initialize the configuration parameters
  /// and to set their default values.
  static void initialize();

  private:
  /// \brief Routine to initialize the debug parameters
  /// and to set their default values.
  static void initializeDebug();
};


inline bool user1() { return Args::config.vt_user_1; }
inline bool user2() { return Args::config.vt_user_2; }
inline bool user3() { return Args::config.vt_user_3; }


}} // end namespace vt::arguments

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
