/*
//@HEADER
// *****************************************************************************
//
//                               args_utils.h
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
#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_UTILS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_UTILS_H

#include <string>

namespace vt { namespace arguments {

/**
 * \file
 */

/**
 * \brief Function to convert variable into string
 * 
 * \param[in] val Value to convert into string
 *
 * \return String representation of variable
 *
 */
template <typename T>
std::string getDisplayValue(const T& val) {
  std::string res;
  return res;
}

/**
 * \brief Function to convert variable into string
 *
 * \param[in] val Boolean value to convert into string
 *
 * \return String representation of variable
 */
template <>
std::string getDisplayValue<bool>(const bool& val) {
  return val ? std::string("ON") : std::string("OFF");
}

/**
 * \brief Function to convert variable into string
 *
 * \param[in] val Integral value to convert into string
 *
 * \return String representation of variable
 */
template <>
std::string getDisplayValue<int>(const int& val) {
  return std::to_string(val);
}

/**
 * \brief Function to convert variable into string
 *
 * \param[in] val String filename to convert.
 *
 * \return String representation of variable
 */
template <>
std::string getDisplayValue<std::string>(const std::string& val) {
  std::string res = std::string("\"") + val + std::string("\"");
  return res;
}

/**
 * \brief Function to verify that a string is acceptable
 *
 * \param[in] name String to verifyName
 *
 * \return 'Cleaned-up' string
 *
 * \note This function removes any '-' characters at the start
 * and verify that the remaining characters are acceptable.
 */
std::string verifyName(const std::string &name);

/**
 * \struct Printer args_utils.h vt/configs/arguments/args_utils.h
 *
 * Virtual class to manage the display of an anchor.
 */
struct Printer {
  public:
  /**
   * \brief Function to trigger the printing of message
   */
  virtual void output() = 0;

  /**
   * \brief Virtual destructor 
   */
  virtual ~Printer() = default;
};


// Forward declaration
template <typename T>
struct Anchor;


/**
 * \struct PrinterOn args_utils.h vt/configs/arguments/args_utils.h
 *
 * Class to manage the display of an active anchor.
 */
template <typename T>
struct PrintOn : public Printer {
public:
  /**
   * \brief Constructor to specify a print message when the anchor is on (or active).
   *
   * \param[in] opt: Pointer to the anchor
   * \param[in] msg_str: Message when the anchor is on (active).
   */
  PrintOn(Anchor<T>* opt, std::string msg_str);

  /**
   * \brief Constructor to specify a print message when the anchor is on (or active).
   *
   * \param[in] opt: Pointer to the anchor
   * \param[in] msg_str: Message when the anchor is on (active).
   * \param[in] fun Boolean function to control the printing 
   */
  PrintOn(Anchor<T>* opt, std::string msg_str, std::function<bool()> fun);

  /**
   * \brief Function to trigger the printing of message
   */
  void output() override;

  /**
   * \brief Destructor
   */
  ~PrintOn() override = default;

  protected:
  /// Pointer to the anchor
  Anchor<T>* option_ = nullptr;
  /// Message to display
  std::string msg_str_;
  /// Conditional function 
  std::function<bool()> condition_ = nullptr;
};


/**
 * \struct PrinterOnOff args_utils.h vt/configs/arguments/args_utils.h
 *
 * Class to manage the display of an active (or inactive) anchor.
 */
template <typename T>
struct PrintOnOff : public Printer {
  public:
  /**
   * \brief Constructor to specify a print message when the anchor is on (or active).
   *
   * \param[in] opt: Pointer to the anchor
   * \param[in] msg_on: Message when the anchor is on (active).
   * \param[in] msg_off: Message when the anchor is off (inactive).
   */
  PrintOnOff(Anchor<T>* opt, std::string msg_on, std::string msg_off);

  /**
   * \brief Constructor to specify a print message when the anchor is on (or active).
   *
   * \param[in] opt: Pointer to the anchor
   * \param[in] msg_on: Message when the anchor is on (active).
   * \param[in] msg_off: Message when the anchor is off (inactive).
   * \param[in] fun Boolean function to control the printing 
   */
  PrintOnOff(Anchor<T>* opt, std::string msg_on, std::string msg_off,
    std::function<bool()> fun);

  /**
   * \brief Function to trigger the printing of message
   */
  void output() override;

  /**
   * \brief Destructor
   */
  ~PrintOnOff() override = default;

  protected:
  /// Pointer to the anchor
  Anchor<T>* option_ = nullptr;
  /// Message to display if active
  std::string msg_on_;
  /// Message to display if inactive
  std::string msg_off_;
  /// Conditional function 
  std::function<bool()> condition_ = nullptr;
};

/**
 * \struct Warning args_utils.h vt/configs/arguments/args_utils.h
 *
 * Class to manage the display of a warning for an anchor.
 */
struct Warning : public Printer {
  public:
  /**
   * \brief Constructor to specify a warning for an anchor.
   *
   * \param[in] opt: Pointer to the anchor
   * \param[in] warning: Warning message
   */
  Warning(Anchor<bool>* opt, std::string warning);

  /**
   * \brief Constructor to specify a warning for an anchor.
   *
   * \param[in] opt Pointer to the anchor
   * \param[in] warning Warning message
   * \param[in] fun Boolean function to control warning
   */
  Warning(Anchor<bool>* opt, std::string warning, std::function<bool()> fun);

  /**
   * \brief Function to trigger the printing of message
   */
  void output() override;

  /**
   * \brief Destructor
   */
  ~Warning() override = default;

  protected:
  /// Pointer to the anchor
  Anchor<bool>* option_;
  /// Warning message related to compilation flag
  std::string compile_;
  /// Conditional function 
  std::function<bool()> condition_ = nullptr;
};


/**
 * \struct ArgsManager args_utils.h vt/configs/arguments/args_utils.h
 *
 * This structure is part of the implementation for managing
 * the parsing and the resolution of precedence rules for 
 * command-line parameters, default values, and parameters from input file.
 */
struct ArgsManager {

  /**
   * \brief Copy constructor
   */
  explicit ArgsManager(const std::string& name);

  /**
   * \brief Add flag for boolean or integral flag
   * 
   * \tparam T  Template type for the value of the parameter
   * \param[in] name Label for the parameter of interest
   * \param[in] anchor_value Variable to specify and whose initial value
   *                         defines the default value
   * \param[in] desc String describing the anchor 
   * \param[in] grp String describing the group containing the anchor 
   * 
   * \return Pointer to the anchor with the specified label
   * 
   * \note Matches the argument interface as CLI::App
   */ 
  template <
    typename T,
    typename = typename std::enable_if<
      std::is_same<T, bool>::value || std::is_same<T, int>::value, T>
    >
  std::shared_ptr<Anchor<T>> addFlag(const std::string& name, T& anchor_value,
    const std::string& desc = {},
    const std::string& grp = "Default");

  /**
   * \brief Add a flag to CLI setup
   * 
   * \param[in] Pointer to the specific anchor
   * \param[in] sname String labelling the anchor 
   * \param[in] anchor_value Variable to specify and whose initial value
   *                        defines the default value
   * \param[in] desc String describing the anchor 
   */ 
  template <typename T>
  void addFlagToCLI(Anchor<T>* aptr, const std::string& sname, T& anchor_value,
    const std::string& desc) {}

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
  std::shared_ptr<Anchor<T>> addOption(const std::string& name,T& anchor_value,
    const std::string& desc = "",
    const std::string& grp = "Default");

  /**
   * \brief Gather the list of all group names
   * 
   * \return Vector of group names
   */
  std::vector<std::string> getGroupList() const;

  /**
   * \brief Get the list of options for a given group name
   * 
   * \param[in] gname Name of group to study
   *
   * \return Map from string anchors to their pointers in the group
   */
  std::map<std::string, std::shared_ptr<AnchorBase>> getGroupOptions(
    const std::string& gname) const;

  /**
   * \brief Returns pointer to the option object for specific name.
   * 
   * \tparam T  Template type for the value of the parameter
   * \param[in] name Label for the parameter of interest
   *
   * \return Pointer to the anchor with the specified label
   */
  template<typename T>
  std::shared_ptr<Anchor<T>> getOption(const std::string &name);

  /**
   * \brief Create string to output the configuration, i.e.
   *
   * The values of all the parameters are stored in a string
   * with the ".INI" format
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
  */ 
  std::string outputConfig(bool default_also, bool write_description,
    std::string prefix) const;

  /**
   * \brief Parse the command line arguments
   * 
   * \param[in,out] argc Number of arguments
   * \param[in,out] argv Array of arguments
   * 
   * \note On exit, argc and argv will be modified in presence of redundancy.
   */ 
  void parse(int &argc, char**& argv);

  /**
   * \brief Routine to review some default parameters
   * after parsing and before printing
   */ 
  void postParsingReview();

  /**
   * \brief Resolve the different options by applying all precedence rules
   */ 
  void resolveOptions() {
    for (const auto& opt : options_) {
      opt.second->resolve();
    }
  }

  /**
   * \brief Set new default value for a specified name
   * 
   * \tparam T  Template type for the value of the parameter
   * \param[in] name String labelling the anchor
   * \param[in] anchor_value Variable to specify and whose initial value
   *                         defines the default value
   * 
   * \return Pointer to the anchor with the specified label
   */ 
  template <typename T>
  std::shared_ptr<Anchor<T>> setNewDefaultValue(const std::string& name,
    const T& anchor_value);

protected:

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the output
   */
  void initializeOutputControl();

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the signal handling 
   */
  void initializeSignalHandling();

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the termination 
   */
  void initializeTermination();

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the stack group
   */
  void initializeStackGroup();

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the tracing group
   */
  void initializeTracing();

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the load balancing group
   */
  void initializeLoadBalancing();

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the user-options group
   */
  void initializeUserOptions();

  /**
   * \brief Function to define default and command-line instances
   * for parameters controlling the debugging group
   */
  void initializeDebug();

public:
  /// Pointer to the CLI interface
  std::unique_ptr<CLI::App> app_;
  /// Map from a string to its anchor pointer
  std::map<std::string, std::shared_ptr<AnchorBase>> options_;
};


}} // namespace vt::arguments

#endif
