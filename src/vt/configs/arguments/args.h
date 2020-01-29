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
#include <memory>
#include <set>
#include <string>
#include <unordered_map>


namespace CLI {
class App;
}

namespace vt { namespace arguments {

/** \file */

/**
 * \struct Configs args.h vt/configs/arguments/args.h
 *
 * \brief The structure with the sets of simulation parameters
 *
 * This structure lists all the parameters that could be set
 * at the start of a simulation.
 *
 */
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


/// Enum for the context where the parameters is set
enum struct ContextEnum : std::uint8_t { dFault, commandLine, thirdParty };


/// Enum for the print status
enum struct PrintOption : std::uint8_t { never = 0, whenSet = 2, always = 255 };


//--- Forward declaration
struct Printer;


/**
 * \struct AnchorBase args.h vt/configs/arguments/args.h
 *
 * \brief Virtual structure for storing a parameter (option or flag).
 *
 * ...
 *
 */
struct AnchorBase: std::enable_shared_from_this<AnchorBase> {

  /**
   * \brief Count how many instances of the parameter 'name' have been specified.
   *
   * ...
   *
   * \param[in] name Parameter to specify
   *
   * \return Count
   */
  virtual int count() const = 0;

  /**
   * \brief Virtual function to print information
   */
  virtual void print() = 0;

  /**
   * \brief Virtual function to reset to the default value
   */
  virtual void resetToDefault() = 0;

  /**
   * \brief Virtual function to resolve precedence rules
   */
  virtual void resolve() = 0;

  /**
   * \brief Set the printing option for the anchor
   * 
   * \param[in] po Enum PrintOption for the anchor
   * \param[in] fun Function returning a boolean to constraint the printing
   */ 
  virtual void setPrintOption(PrintOption po, std::function<bool()> fun) = 0;

  /**
   * \brief Virtual function returning resolved value
   *
   * \returns Resolved value
   */
  virtual std::string stringifyValue() const = 0;

  /**
   * \brief Virtual function returning context for resolved value
   *
   * \returns Context
   */
  virtual std::string stringifyContext() const = 0;

  /**
   * \brief Virtual function returning context for resolved value
   *
   * \returns Default value
   */
  virtual std::string stringifyDefault() const = 0;

  //--------------------------------------------//
  // Non-virtual member functions
  //--------------------------------------------//

  /**
   * \brief Sets excluded option
   * 
   * \param[in] opt  Pointer to the option to exclude
   */
  void excludes(const std::shared_ptr<AnchorBase>& opt) {
    excludes_.insert(opt);
    // Help text should be symmetric - excluding a should exclude b
    opt->excludes_.insert(shared_from_this());
    //
    // Ignoring the insert return value, excluding twice (or more) is allowed.
    //
  }

  /** 
   * \brief Returns the description of the anchor
   * 
   * ...
   *
   * \return String for the description
   */
  std::string getDescription() const { return description_; }

  /**
   * \brief Returns the group name of the anchor
   *
   * \return String for the group
   */
  std::string getGroup() const { return group_; }

  /**
   * \brief Returns the name of the anchor
   *
   * \return Name of the anchor
   */
  std::string getName() const { return name_; }

  /**
   * \brief Function to determine whether the precedence rules have been applied.
   *
   * \returns Boolean
   */
  bool isResolved() const { return isResolved_; }

  /**
   * \brief Sets dependence on a specific flag being turned off
   * 
   * \param[in] opt Pointer to the flag to depend on
   */
  void needsOptionOff(std::shared_ptr<AnchorBase> const& opt) {
    needsOptOff_.insert(opt);
  }

  /**
   * \brief Sets dependence on a specific flag being turned on
   * 
   * \param[in] opt Pointer to the flag to depend on
   */
  void needsOptionOn(std::shared_ptr<AnchorBase> const& opt) {
    needsOptOn_.insert(opt);
  }

  /**
   * \brief Set the group name for the anchor
   *
   * \param[in] grp_ String for the name of the group
   */
  void setGroup(const std::string& grp_) { group_ = grp_; }

  /**
   * \brief Set the printing option for the anchor
   * 
   * \param[in] po Enum PrintOption for the anchor
   * \param[in] fun Function returning a boolean to constraint the printing
   */ 
  virtual void setPrintOptionImpl(PrintOption po, std::function<bool()> fun)=0;

protected:

  /**
   * \brief Constructor
   * 
   * \param[in] name Label for the anchor
   * \param[in] desc Description of the anchor
   * 
   * \note The anchor will be assigned to a default group, named 'Default'.
   */
  AnchorBase(std::string name, std::string desc, std::string grp);

protected:

  /// Enum for ordering the context
  ///
  /// Order of the contexts can be specified independently of 
  /// an instance for that context.
  ///
  enum struct OrderContextEnum : std::uint8_t {
    MIN = 0,
    dFault = 1,
    commandLine = 2,
    thirdParty = 3,
    MAX = 255
  };

  /// Map from 'ContextEnum' flag to an ordered flag.
  static std::map<ContextEnum, OrderContextEnum> emap_;

  /// Map from 'ContextEnum' flag to a descriptive string.
  static std::map<ContextEnum, std::string> smap_;

  /// Boolean to indicate whether precedence rules have been applied.
  bool isResolved_;

  /// String name for the anchor
  std::string name_;

  /// String name for the group containing the anchor
  std::string group_;

  /// String name for the description of the anchor 
  std::string description_;

  /// Map ordering the different instances of an 'anchor'
  std::map<ContextEnum, OrderContextEnum> ordering_;

  /// Set of anchors that are needed to be OFF for this anchors
  std::set<std::shared_ptr<AnchorBase>> needsOptOff_;

  /// Set of anchors that are needed to be ON for this anchors
  std::set<std::shared_ptr<AnchorBase>> needsOptOn_;

  /// Set of anchors that are excluded with this anchors
  std::set<std::shared_ptr<AnchorBase>> excludes_;

  /// Printing options for the anchor
  PrintOption statusPrint_ = PrintOption::never;

  /// Pointer to 'Printer' object for displaying message on the startup banner.
  std::unique_ptr<Printer> screenPrint_;

};


/**
 * /struct Anchor args.h vt/configs/arguments/args.h
 *
 * \brief Template struct for storing an option/flag
 *
 * ...
 *
 */
template <typename T>
struct Anchor : public AnchorBase {

public:
  /**
   * \brief Constructor
   * 
   * \param[in] var Variable to specify and whose initial value
   *                defines the default value
   * \param[in] name Label for the parameter of interest
   * \param[in] desc String describing the anchor
   * \param[in] grp  String for the group owning the anchor
   *
   */ 
  explicit Anchor(
    T& var, const std::string& name, const std::string& desc,
    const std::string& grp = "Default"
  );

  /**
   *  \brief Add instance with a specific value
   *
   * The routine adds a specific value for an instance of the anchor.
   *
   * \param[in] value Value to specify for the third party context
   * \param[in] highestPrecedence Boolean determining whether the instance
   * should have the highest priority (default = false)
   */
  void addInstance(const T& value, bool highestPrecedence = false);

  /**
   * \brief Counts the current number of instances for the anchor 
   * 
   * \returns Number of instances for the anchor
   */
  int count() const override {
    return static_cast<int>(specifications_.size());
  }

  /**
   * \brief Sets highest priority (precedence) for a context
   * 
   * \param[in] origin Context for the highest priority
   * 
   * \note If a different context had already the highest priority,
   * this context priority will be reset to its original value.
   */ 
  void setHighestPrecedence(const ContextEnum& origin);

  /**
   * \brief Sets a new default value.
   * 
   * \param[in] ref Value to assign as default
   */
  void setNewDefaultValue(const T& ref);

  /**
   * \brief Set the printing option for the anchor
   *
   * ...
   * 
   * \param[in] po Enum PrintOption for the anchor
   * \param[in] fun Function returning a boolean to constraint the printing
   */ 
  void setPrintOption(PrintOption po, std::function<bool()> fun) override;

  /**
   * \brief Returns the default value for the anchor
   * 
   * \returns Default value of the anchor
   */
  const T getDefaultValue() const;

  /**
   * \brief Returns the value for the current instance
   * 
   * \returns Value of the current instance
   */
  const T& getValue() const { return value_; }

  /**
   * \brief Virtual function returning resolved value
   * \returns Resolved value
   */
  std::string stringifyValue() const override;

  /**
   * \brief Virtual function returning context for resolved value
   *
   * \returns String for resolved context
   */
  std::string stringifyContext() const override;

  /**
   * \brief Virtual function returning default value
   *
   * \returns String for default value
   */
  std::string stringifyDefault() const override;

  /**
   * \brief Resets to the default value
   */
  void resetToDefault() override;

  //
  //--- Printing routines
  //

  /**
   * \brief Printing routine about the anchor
   */
  void print() override;

  /**
   * \brief Set the printing message for the banner as a warning
   * 
   * \param[in] String to display for the anchor
   * 
   * \note The message will be of the form
   * "Warning: 'NAME' has no effect: compile-time feature 'MSG' is disabled"
   */ 
  void setBannerMsgWarning(std::string msg, std::function<bool()> fun=nullptr);

  /**
   * \brief Add instance from a particular context
   * 
   * \param[in] ctxt Context for the instance
   * \param[in] value Value specified by the instance
   */ 
  void addGeneralInstance(ContextEnum ctxt, const T& value);

  /**
   * \brief Checks whether two 'excluded' options have specifications
   *         in addition to their default ones.
   */
  void checkExcludes() const;

  /**
   * \brief Resolves the precedence rules among the instances
   */
  void resolve() override;

  /**
   * \brief Set the printing option for the anchor
   * 
   * \param[in] po Enum PrintOption for the anchor
   * \param[in] fun Function returning a boolean to constraint the printing
   */ 
  void setPrintOptionImpl(PrintOption po, std::function<bool()> fun) override;

protected:

  /**
   *
   * \brief Structure for storing each instance of an anchor
   *
   */
  template <typename U = T>
  struct Instance {

  public:

    /**
     * \brief Constructor
     * 
     * \param[in] ref Value of the anchor for this instance
     * \param[in] parent Pointer for the parent anchor
     *
     */
    explicit Instance(const U& rval, AnchorBase* parent)
      : value_(rval), parent_(parent)
    {}

    /** 
     * \brief Dummy Constructor
     *
     */
    Instance() : value_(), parent_(nullptr)
    {}

    /**
     *  \brief Copy Constructor
     *
     * \param[in] ref Reference to existing instance
     *
     */
    Instance(const Instance<U>& ref)
      : value_(ref.value_), parent_(ref.parent_)
    {}

    /**
     * \brief Destructor
     */
    ~Instance() = default;

    /**
     *  \brief Assignment operator
     *
     * \param[in] ref Reference to existing instance
     *
     */
    Instance<U>& operator=(const Instance<U>& ref) {
      value_ = ref.value_;
      parent_ = ref.parent_;
      return *this;
    }

    /**
     *  \brief Sets new value for the instance
     * 
     * \param[in] ref New value to insert for the instance
     *
     */
    void setNewValue(const U& ref) { value_ = ref; }

    /**
     *  \brief Returns the value for the current instance
     *
     * \returns Value of the current instance
     */
    const U& getValue() const { return value_; }

  protected:
    /// Parameter-value for this instance
    U value_;
    /// Pointer to the anchor corresponding to that instance
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

//---Forward declaration
struct ArgsManager;

/**
 * \struct Args args.h vt/configs/arguments/args.h
 *
 * \brief The main class for configuring the set of simulation parameters.
 *
 */
struct Args {

public:

  /// Static set of configuration parameters
  static Configs config;

public:

  /**
   * \brief Parse the command line inputs and resolve the precedence rules
   * 
   * \param[in,out] argc Number of arguments
   * \param[in,out] argv Array of arguments
   * 
   * \note When exiting, argc and argv will be modified in presence of
   * redundancy.
   * \note At exit, the configuration parameters will be set.
   *
   */ 
  static void parseAndResolve(int& argc, char**& argv);

  /**
   * \brief Returns pointer to the option object for specific name.
   *
   * ...
   * 
   * \tparam T  Template type for the value of the parameter
   * \param[in] name Label for the parameter of interest
   *
   * \return Pointer to the anchor with the specified label
   */
  template <typename T>
  static std::shared_ptr<Anchor<T>> getOption(const std::string& name);

  /**
   * \brief Add flag for boolean or integral flag
   * 
   * \tparam T  Template type for the value of the parameter
   * \param[in] name Label for the parameter of interest
   * \param[in] anchor_value Variable to specify and whose initial value
   *                         defines the default value
   * \param[in] desc String describing the option
   * \param[in] grp  String for the group owning the anchor
   * 
   * \return Pointer to the anchor with the specified label
   * 
   * \note Matches the argument interface as CLI::App
   *
   */ 
  template <
    typename T,
    typename = typename std::enable_if<
      std::is_same<T, bool>::value || std::is_same<T, int>::value, T
      > >
  static std::shared_ptr<Anchor<T>> addFlag(const std::string& name,
    T& anchor_value, const std::string& desc = {},
    const std::string& grp = "Default");

  /**
   * \brief Add option for a specified name
   *
   * \tparam T  Template type for the value of the parameter
   * \param[in] name String labelling the option
   * \param[in] anchor_value Variable to specify and whose initial value
   *                         defines the default value
   * \param[in] desc String describing the option
   * \param[in] grp  String for the group owning the anchor
   * 
   * \return Pointer to the anchor with the specified label
   * 
   * \note Matches the argument interface as CLI::App
   */
  template <typename T>
  static std::shared_ptr<Anchor<T>> addOption(const std::string& name,
    T& anchor_value, const std::string& desc = "",
    const std::string& grp = "Default");

  /**
   * \brief Set new default value for a specified name
   * 
   * \tparam T  Template type for the value of the parameter
   * \param[in] name String labelling the option
   * \param[in] anchor_value Variable to specify and whose initial value
   *                         defines the default value
   *
   * \return Pointer to the anchor with the specified label
   */ 
  template <typename T>
  static std::shared_ptr<Anchor<T>> setNewDefaultValue(const std::string& name,
    const T& anchor_value);

  //
  // --- Printing routines
  //

  /**
   * \brief Print the list of options and instances
   */
  static void printBanner();

  /**
   * \brief Create string to output the configuration
   *
   * This routine generates a string of values for the current configuration.
   * The string stores a ".INI"-formatted output.
   * (INI format: https://cliutils.gitlab.io/CLI11Tutorial/chapters/config.html )
   *
   * \param[in] default_also Boolean to identify whether or not to print
   * default value
   * \param[in] write_description Boolean to turn on/off the printing
   * of the parameter description
   * \param[in] prefix String containing a prefix to add before each
   * option name
   * 
   * \return String describing all the options
   * 
   */
  static std::string outputConfig(bool default_also, bool write_description,
    std::string prefix);

private:

  static bool parsed_;
  static std::unique_ptr<ArgsManager> manager_;

  static void checkInitialization();

};

/**
 * \brief Returns value of user-specified flag
 */
inline bool user1() { return Args::config.vt_user_1; }

/**
 * \brief Returns value of user-specified flag
 */
inline bool user2() { return Args::config.vt_user_2; }

/**
 * \brief Returns value of user-specified flag
 */
inline bool user3() { return Args::config.vt_user_3; }

}} /* end namespace vt::arguments */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
