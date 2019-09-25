/*
//@HEADER
// ************************************************************************
//
//                          args.h
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

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H

#include <map>
#include <unordered_map>

#include "vt/config.h"

#include "fmt/format.h"
#include "CLI/CLI11.hpp"

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
enum class context : std::uint8_t {dFault, commandLine, thirdParty};


struct AnchorBase;


template< typename T>
struct Instance {

public:

   /// \brief Constructor
   ///
   /// \param[in] ref
   /// \param[in] parent Pointer for the parent anchor
   explicit Instance(const T &ref, AnchorBase *parent)
      : value_(ref), parent_(parent)
   { }

   /// \brief Dummy Constructor
   Instance() : value_(), parent_(nullptr) { }

   /// \brief Sets new value for the instance
   ///
   /// \param ref New value to insert for the instance
   void setNewValue(const T &ref) { value_ = ref; }

   /// \brief Returns the value for the current instance
   ///
   /// \returns Value of the current instance
   const T& getValue() const { return value_; }

protected:
   T value_ = {};
   AnchorBase *parent_ = nullptr;
};


/// \brief Virtual class for storing an option/flag
struct AnchorBase : public std::enable_shared_from_this<AnchorBase> {

   ~AnchorBase() = default;

   /// \brief Count how many instances of the parameter 'name'
   /// have been specified.
   /// \param[in] name Parameter to specify
   virtual int count() const = 0;

   /// \brief Sets excluded option
   ///
   /// \param[in] opt  Pointer to the option to exclude
   void excludes(const std::shared_ptr<AnchorBase> &opt) {
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

// Function to add if 'needed'
//      void needs(std::shared_ptr<AnchorBase> const &opt) {
//         needs_.insert(opt);
//      }

  /// \brief Virtual determining whether precedence rules are
  /// resolved
  virtual bool isResolved() const = 0;

   /// \brief Virtual function to resolve precedence rules
   virtual void resolve() = 0;

   /// \brief Set the group name for the option
   ///
   /// \param[in] grp_ String for the name of the group
   void setGroup(const std::string &grp_) { group_ = grp_; }

   /// \brief Virtual function returning resolved value
   /// \returns Resolved value
   virtual std::string valueToString() const = 0;

   /// \brief Virtual function returning context for resolved value
   /// \returns Context
   virtual std::string valueContext() const = 0;

   /// \brief Virtual function returning context for resolved value
   /// \returns Default value
   virtual std::string valueDefault() const = 0;

protected:

   /// \brief Constructor
   ///
   /// \param name Label for the anchor
   /// \param desc Description of the anchor
   ///
   AnchorBase(std::string name, std::string desc) :
      name_(std::move(name)),
      group_("Default"),
      description_(std::move(desc)),
      ordering_(),
      needs_(),
      excludes_()
   { }

protected:

   //--- Build enum for ordering the context
   //--- Order of the contexts can be specified
   //--- independently of an instance for that context.
   enum class orderCtxt : std::uint8_t {MIN = 0,
      dFault = 1, commandLine = 2, thirdParty = 3,
      MAX = 255};

   //--- Map from 'context' flag to an ordered flag.
   static std::map<const context, const orderCtxt> emap_;

   //--- Map from 'context' flag to a descriptive string.
   static std::map<const context, const std::string> smap_;

   std::string name_ = {};
   std::string group_ = {};
   std::string description_ = {};

   /// \brief Map ordering the different instances of an 'anchor'
   std::map< const context, orderCtxt > ordering_;

   /// \brief List of options that are required with this option
   std::set< std::shared_ptr<AnchorBase> > needs_;

   /// \brief List of options that are excluded with this option
   std::set< std::shared_ptr<AnchorBase> > excludes_;

};


/// \brief Template class for storing an option/flag
template< typename T>
struct Anchor : public AnchorBase {

public:

   /// \brief Constructor
   ///
   /// \param[in] var Variable to specify and whose initial value
   ///                defines the default value
   /// \param[in] name Label for the parameter of interest
   /// \param[in] desc String describing the option
   ///
   explicit Anchor(
      T& var,
      const std::string &name,
      const std::string &desc
   ) : AnchorBase(name, desc),
       value_(var),
       specifications_(),
       hasNewDefault_(false),
       isResolved_(false),
       resolved_ctxt_(context::dFault),
       resolved_instance_(),
       resolved_to_default_(false),
       always_print_(false),
       always_print_startup_(false),
       print_value_(nullptr),
       print_condition_(nullptr)
   {
      auto myCase = Instance<T>(var, this);
      specifications_.insert(std::make_pair(context::dFault, myCase));
      ordering_[context::dFault] = orderCtxt::dFault;
   }

   /// \brief Add instance from a third party with a specific value
   ///
   /// \param[in] value Value to specify for the third party context
   /// \param[in] highestPrecedence Boolean determining whether the instance
   /// should have the highest priority (default = false)
   void addInstance(
      const T &value,
      bool highestPrecedence = false
   )
   {
      try {
         addGeneralInstance(context::thirdParty, value);
      }
      catch (const std::exception &e) {
         throw;
      }
      if (highestPrecedence)
         setHighestPrecedence(context::thirdParty);
   }

   /// \brief Counts the current number of instances for the option
   ///
   /// \returns Number of instances for the option
   int count() const override { return static_cast<int>(specifications_.size()); }

   /// \brief Sets a new default value.
   ///
   /// \param ref Value to assign as default
   void setNewDefaultValue(const T &ref)
   {
      //--- Context dFault always has one instance
      //--- So the find is always successful.
      auto iter = specifications_.find(context::dFault);
      (iter->second).setNewValue(ref);
      hasNewDefault_ = true;
   }

   /// \brief Sets highest priority (precedence) for a context
   ///
   /// \param[in] origin Context for the highest priority
   ///
   /// \note If a different context had already the highest priority,
   /// this context priority will be reset to its original value.
   ///
   void setHighestPrecedence(const context &origin)
   {
      // Reset the 'current' highest precedence
      for (auto &entry : ordering_) {
         if (entry.second == orderCtxt::MAX)
            entry.second = emap_[entry.first];
      }
      ordering_[origin] = orderCtxt::MAX;
   }

   //
   // Information about resolved instance
   //

   /// \brief Function to determine whether the precedence rules
   /// have been applied or not.
   /// \returns Boolean
   bool isResolved() const override { return isResolved_; }

   /// \brief Virtual function returning resolved value
   /// \returns Resolved value
   std::string valueToString() const override;

   /// \brief Virtual function returning context for resolved value
   /// \returns Context
   std::string valueContext() const override;

   /// \brief Virtual function returning default value
   /// \returns Default value
   std::string valueDefault() const override;

   //
   //--- Printing routines
   //

   void printAlways(bool always) {
      always_print_startup_ = always;
      print_condition_ = [](T const&) -> bool { return true; };
   }

   void printIf(std::function<void()> fn) {
      print_condition_ = [=](T const&) -> bool { fn(); };
   }

   void printWhen(std::function<bool(T const& value)> cond) {
      print_condition_ = cond;
   }

   void setPrinter(std::function<std::string(T const& value)> in) {
      print_value_ = in;
   }

   template <typename U>
   using IsBoolTrait =
      typename std::enable_if<std::is_same<U, bool>::value, T>::type;

   template <typename U>
   using IsNotBoolTrait =
      typename std::enable_if<not std::is_same<U, bool>::value, T>::type;

   template <typename U = T>
   void printWhenOff(
      std::string off_str, IsBoolTrait<U>* __attribute__((unused)) u = nullptr
   ) {
      print_condition_ = [](T const& val) -> bool { return val == false; };
      print_value_ = [=](T const&) -> std::string { return off_str; };
   }

   template <typename U = T>
   void printWhenOn(
      std::string on_str, IsBoolTrait<U>* __attribute__((unused)) u = nullptr
   ) {
      print_condition_ = [](T const& val) -> bool { return val == true; };
      print_value_ = [=](T const&) -> std::string { return on_str; };
   }

   template <typename U = T>
   void printWhenOnOff(
      std::string on_str, std::string off_str,
      IsBoolTrait<U>* __attribute__((unused)) u = nullptr
   ) {
      printAlways(true);
      print_value_ = [=](T const& val) -> std::string {
         return val ? on_str : off_str;
      };
   }

   template <typename U = T>
   void printWhenValueNot(
      T in_val, std::string val_str,
      IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
   ) {
      print_condition_ = [=](T const& val) -> bool { return val != in_val; };
      print_value_ = [=](T const& val) -> std::string {
             return fmt::format(val_str, val);
      };
   }

   template <typename U = T>
   void printValueWhenNotDefault(
      std::string val_str,
      IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
   ) {
      printValueWhenDefaultTest(false, val_str);
   }

   template <typename U = T>
   void printValueWhenDefault(
      std::string val_str,
      IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
   ) {
      printValueWhenDefaultTest(true, val_str);
   }

   template <typename U = T>
   void printValueWhenDefaultTest(
      bool is_default, std::string val_str,
      IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
   ) {
      print_condition_ = [=](T const& val) -> bool {
         return is_default == resolved_to_default_;
      };
      print_value_ = [=](T const& val) -> std::string {
         return fmt::format(val_str, val);
      };
   }

protected:

   /// \brief Add instance from a particular context
   ///
   /// \param[in] ctxt Context for the instance
   /// \param[in] value Value specified by the instance
   ///
   void addGeneralInstance(context ctxt, const T &value);

   /// \brief Checks whether two 'excluded' options have specifications
   ///         in addition to their default ones.
   void checkExcludes() const;

   /// \brief Resolves the precedence rules among the instances
   void resolve() override;

   friend struct ArgSetup;

protected:

   T& value_ = {};
   std::map< context, Instance<T> > specifications_ = {};
   bool hasNewDefault_ = false;
   bool isResolved_ = false;
   context resolved_ctxt_ = context::dFault;
   Instance<T> resolved_instance_ = {};
   bool resolved_to_default_ = false;

   //--- Parameters related to printing
   bool always_print_ = false;
   bool always_print_startup_ = false;
   std::function<std::string(T const& value)> print_value_ = nullptr;
   std::function<bool(T const& value)> print_condition_    = nullptr;

};


/// \brief Structure to store all the different configuration parameters
///
struct ArgSetup {

public:

   /// \brief Constructor
   /// \param[in] Description of the application as a string
   ArgSetup(std::string description = "") :
     app_(description), options_(), parsed(false)
   { }

   /// \brief Returns pointer to the option object for specific name.
   ///
   /// \tparam T  Template type for the value of the parameter
   /// \param[in] name Label for the parameter of interest
   /// \return Pointer to the anchor with the specified label
   template<typename T>
   std::shared_ptr<Anchor<T> > getOption(const std::string &name) const;

    /// \brief Add flag for integer parameter
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
    template<typename T,
       typename std::enable_if<
          !CLI::is_bool<T>::value && std::is_integral<T>::value,
          CLI::detail::enabler
       >::type = CLI::detail::dummy
    >
    std::shared_ptr< Anchor<T>> addFlag(const std::string &name,
                                        T& anchor_value,
                                        const std::string &desc = {}
    )
    {
       std::string sname;
       try {
          sname = verifyName(name);
       }
       catch (const std::exception &e) {
          throw;
       }
       //
       // Implementation for an integral flag
       //
       auto iter = options_.find(sname);
       if (iter == options_.end()) {
          // @todo: stop using default for warn_override and get this from runtime?
          auto anchor = std::make_shared< Anchor<T> >(anchor_value, sname, desc);
          options_[sname] = anchor;
          //
          // Insert the flag into CLI app
          //
          CLI::callback_t fun = [&anchor,&anchor_value](const CLI::results_t &res) {
             anchor_value = static_cast<T>(res.size());
             anchor->addGeneralInstance(context::commandLine, anchor_value);
             return true;
          };
          std::string cli_name = std::string("--") + sname;
          auto opt = app_.add_option(cli_name, fun, desc, true);
          opt->type_size(0);
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

    /// \brief Add flag for boolean parameter
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
    template<typename T,
       typename std::enable_if< CLI::is_bool<T>::value, CLI::detail::enabler
       >::type = CLI::detail::dummy
    >
    std::shared_ptr<Anchor<T>> addFlag(const std::string &name,
                                       T& anchor_value,
                                       const std::string &desc = {}
    )
    {
       std::string sname;
       try {
          sname = verifyName(name);
       }
       catch (const std::exception &e) {
          throw;
       }
       //
       // Implementation for a boolean flag
       //
       auto iter = options_.find(sname);
       if (iter == options_.end()) {
          // @todo: stop using default for warn_override and get this from runtime?
          auto anchor = std::make_shared< Anchor<T> >(anchor_value, sname, desc);
          options_[sname] = anchor;
          //
          // Insert the flag into CLI app
          //
          CLI::callback_t fun = [&anchor,&anchor_value](const CLI::results_t &res) {
             anchor_value = true;
             anchor->addGeneralInstance(context::commandLine, anchor_value);
             return (res.size() == 1);
          };
          std::string cli_name = std::string("--") + sname;
          auto opt = app_.add_option(cli_name, fun, desc);
          opt->type_size(0);
          opt->multi_option_policy(CLI::MultiOptionPolicy::TakeLast);
          //
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
    std::shared_ptr<Anchor<T>> addOption(const std::string &name,
                                         T& anchor_value,
                                         const std::string &desc = ""
    );

   /// \brief Parse the command line inputs and resolve the precedence rules
   ///
   /// \param[in,out] argc Number of arguments
   /// \param[in,out] argv Array of arguments
   ///
   /// \note When exiting, argc and argv will be modified in presence of redundancy.
   ///
   /// \note At exit, the configuration parameters will be set.
   ///
   void parseResolve(int &argc, char** &argv) {
      try {
         parse(argc, argv);
         resolveOptions();
      }
      catch (const std::exception &e) {
         throw;
      }
   }

    //
    // --- Printing routines
    //

    /// \brief Print the list of options and instances
    ///
    void print()
    {
//         for (const auto &opt : options_) {
//            //opt.second->printWhenOn();
//         }
    }

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
   std::shared_ptr<Anchor<T>> setNewDefaultValue(
      const std::string &name,
      const T& anchor_value
   );

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
   /// (INI format: https://cliutils.gitlab.io/CLI11Tutorial/chapters/config.html )
   std::string to_config(bool default_also,
      bool write_description, std::string prefix
   ) const;

protected:

   /// \brief Parse the command line arguments
   ///
   /// \param[in,out] argc Number of arguments
   /// \param[in,out] argv Array of arguments
   ///
   /// \note On exit, argc and argv will be modified in presence of redundancy.
   ///
   int parse(int& argc, char**& argv);

   /// \brief Resolve the different options by applying all precedence rules
   ///
   void resolveOptions() {
      for (const auto &opt : options_) {
         opt.second->resolve();
      }
   }

   /// \brief Function to verify that a string is acceptable
   ///
   /// \param[in] name String to verifyName
   ///
   /// \return 'Cleaned-up' string
   ///
   /// \note This function removes any '-' characters at the start
   /// and verify that the remaining characters are acceptable.
   ///
   std::string verifyName(const std::string &name) const;

private:

  CLI::App app_;
  std::unordered_map<std::string, std::shared_ptr<AnchorBase> > options_ = {};

public:

  bool parsed;

};


struct Args {

public:

  static ArgSetup setup_;
  static Configs config;

public:

  /// \brief Routine to initialize the configuration parameters
  /// and to set their default values.
  static void initialize();

};


inline bool user1() { return Args::config.vt_user_1; }
inline bool user2() { return Args::config.vt_user_2; }
inline bool user3() { return Args::config.vt_user_3; }


}} /* end namespace vt::arguments */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
