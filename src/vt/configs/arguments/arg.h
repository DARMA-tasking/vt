/*
//@HEADER
// *****************************************************************************
//
//                                    arg.h
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

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARG_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARG_H

#include "vt/config.h"

#include <type_traits>
#include <functional>
#include <string>
#include <memory>
#include <tuple>
#include <list>
#include <unordered_map>

#include <CLI/CLI11.hpp>
#include <fmt/format.h>

namespace vt { namespace arguments {

struct Group {
  explicit Group(std::string const& in_name)
    : name_(in_name)
  { }

  Group(std::string const& in_name, std::string const& in_feature, bool enabled)
    : name_(in_name),
      feature_tag_name_(in_feature),
      feature_enabled_(enabled)
  { }

  std::string const& name() const { return name_; }
  std::string const& featureTagName() const { return feature_tag_name_; }
  bool featureEnabled() const { return feature_enabled_; }

private:
  std::string name_             = "";
  std::string feature_tag_name_ = "";
  bool feature_enabled_         = true;
};

struct AnchorBase {
  virtual void resolve() = 0;
};

template <typename T, typename ArgT>
struct Anchor : AnchorBase {
  using OverrideFnType =
    std::function<
      std::string(std::string opt, T v1, T v2, std::string c1, std::string c2)
    >;

  explicit Anchor(
    T& in_anchor, OverrideFnType in_warn = nullptr, bool has_base = true
  ) : anchor_(&in_anchor),
      base_default_(anchor_),
      has_base_default_(has_base),
      warn_override_(in_warn)
  { }
  Anchor(T& in_anchor, T in_base_default, OverrideFnType in_warn = nullptr)
    : anchor_(&in_anchor),
      base_default_(in_base_default),
      has_base_default_(true),
      warn_override_(in_warn)
  { }

  void addHigherPrecedence(std::shared_ptr<ArgT> arg) { overloads_.push_back(arg);  }
  void addLowerPrecedence(std::shared_ptr<ArgT> arg)  { overloads_.push_front(arg); }

  void resolve() override {
    // Current value 3-tuple as we walk the overloads in low-to-high
    // precedence order
    T           cur_value      = has_base_default_ ? base_default_ : T{};
    std::string cur_context    = "System no-value default";
    bool        cur_is_default = true;

    auto update_cur = [&](std::shared_ptr<ArgT> arg, bool new_value_is_default) {
      vtAssertExpr(not new_value_is_default or cur_is_default);
      cur_value = arg->value();
      cur_context = arg->context();
      cur_is_default = new_value_is_default;
    };

    for (auto arg : overloads_) {
      if (arg->defaultEnabled() and arg->isDefaultValue()) {
        // If the latest value is a default value, update the current value
        // with a higher precedence default value
        if (cur_is_default) {
          update_cur(arg, true);
        }
        // Otherwise, ignore any other higher precedence default value
      } else {
        if (cur_is_default) {
          // Update the value for the first time; the new value is not a
          // default, all other default values shall be ignored
          update_cur(arg, false);
        } else {
          bool const is_same = cur_value == arg->value();

          // Abort if the argument does *not* allow precedence and there is
          // another non-default, non-equal value previously specified in the
          // configuration
          if (not arg->allowsPrecedence() and not is_same) {
            auto ret = fmt::format(
              "Multiple, non-default differing values specified for argument {} "
              " which does not allow precedence overloads: v1={} v2={}",
              arg->name(), cur_value, arg->value()
            );
            vtAbort(ret);
          }

          // Inform the runtime that we are overriding a non-default value
          if (not is_same) {
            if (warn_override_ != nullptr) {
              warn_override_(
                arg->name(), cur_value, arg->value(), cur_context, arg->context()
              );
            }
          }

          update_cur(arg, false);
        }
      }

      *anchor_               = cur_value;
      resolved_context_      = cur_context;
      resolved_to_a_default_ = cur_is_default;
      is_resolved_ = true;
    }
  }

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

  // template <typename U = T>
  // void printValueWhenNotDefault(
  //   std::string val_str,
  //   IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
  // ) {
  //   printValueWhenDefaultTest(false, val_str);
  // }

  // template <typename U = T>
  // void printValueWhenDefault(
  //   std::string val_str,
  //   IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
  // ) {
  //   printValueWhenDefaultTest(true, val_str);
  // }

  // template <typename U = T>
  // void printValueWhenDefaultTest(
  //   bool is_default, std::string val_str,
  //   IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
  // ) {
  //   print_condition_ = [=](T const& val) -> bool {
  //     return is_default ? resolved_to_a_default_ : not resolved_to_a_default_;
  //   };
  //   print_value_ = [=](T const& val) -> std::string {
  //     return fmt::format(val_str, val);
  //   };
  // }


  T const* baseDefault() const { return &base_default_; }
  bool resolved()          const { return is_resolved_; }
  bool resolvedToDefault() const { return resolved_to_a_default_; }
  std::string const& resolvedContext() const { return resolved_context_; }
  T const* value() const { return anchor_; }
  void group(std::shared_ptr<Group> const& in) { group_ = in; }
  std::shared_ptr<Group> getGroup() const { return group_; }

private:
  std::list<std::shared_ptr<ArgT>> overloads_             = {};
  T* anchor_                                              = nullptr;
  T base_default_                                         = {};
  bool has_base_default_                                  = true;
  OverrideFnType warn_override_                           = nullptr;
  std::function<std::string(T const& value)> print_value_ = nullptr;
  std::function<bool(T const& value)> print_condition_    = nullptr;
  std::shared_ptr<Group> group_                           = nullptr;
  bool always_print_startup_                              = false;
  std::string resolved_context_                           = "";
  bool resolved_to_a_default_                             = false;
  bool is_resolved_                                       = false;
};

struct ArgumentBase {
  virtual CLI::Option* add(CLI::App& app) = 0;

  CLI::Option* getOption() const { return opt_; }
protected:
  CLI::Option* opt_ = nullptr;
};

struct ArgumentCLITag { };

template <typename T>
struct Argument : ArgumentBase {
  using AnchorType = std::shared_ptr<Anchor<T, Argument<T>>>;

  Argument(
    std::string in_arg, AnchorType in_anchor, std::string in_desc,
    std::string in_context, T in_value, bool specifies_a_default_value
  ) : arg_(in_arg),
      desc_(in_desc),
      context_(in_context),
      default_enabled_(true),
      value_(in_value),
      default_value_(
        specifies_a_default_value ? in_value : *in_anchor->baseDefault()
      ),
      anchor_(in_anchor),
      is_cli_arg_(false)
  { }

  Argument(
    ArgumentCLITag, std::string in_arg, AnchorType in_anchor,
    std::string in_desc, bool in_default_enabled = true,
    T* default_value = nullptr
  ) : arg_(in_arg),
      desc_(in_desc),
      context_("command-line"),
      default_enabled_(in_default_enabled),
      default_value_(
        in_default_enabled ?
        (default_value ? *default_value : *in_anchor->baseDefault()) :
        T{}
      ),
      anchor_(in_anchor),
      is_cli_arg_(true)
  { }

  CLI::Option* add(CLI::App& app) override { return addTyped(app); }

  template <typename U>
  using IsBoolTrait =
    typename std::enable_if<std::is_same<U, bool>::value, T>::type;
  template <typename U>
  using IsNotBoolTrait =
    typename std::enable_if<not std::is_same<U, bool>::value, T>::type;

  template <typename U = T>
  CLI::Option* addTyped(
    CLI::App& app, IsBoolTrait<U>* __attribute__((unused)) u = nullptr
  ) {
    std::string const app_arg = makeAppArg();
    auto opt = app.add_flag(app_arg, value_, desc_);
    return setupOption(opt);
  }

  template <typename U = T>
  CLI::Option* addTyped(
    CLI::App& app, IsNotBoolTrait<U>* __attribute__((unused)) u = nullptr
  ) {
    std::string const app_arg = makeAppArg();
    auto opt = app.add_option(app_arg, value_, desc_, default_enabled_);
    return setupOption(opt);
  }

  static constexpr bool isFlag() { return std::is_same<T,bool>::value; }

  void setAnchor(AnchorType in_anchor) { anchor_ = in_anchor; }
  void setContext(std::string const& ctx) { context_ = ctx; }

  // Match CLI11 interface for setting group to easy interface update
  void group(std::shared_ptr<Group> const& in) {
    group_ = in;
    anchor_->group(in);
  }

  template <typename U>
  void excludes(std::shared_ptr<Argument<U>> const& in) {
    opt_->excludes(in->opt_);
  }

  std::shared_ptr<Group> getGroup() const { return group_; }
  AnchorType anchor()               const { return anchor_; }
  std::string const& name()         const { return arg_; }
  std::string const& shortName()    const { return short_arg_; }
  std::string const& desc()         const { return desc_; }
  std::string const& context()      const { return context_; }
  T const& value()                  const { return value_; }
  T const& defaultValue()           const { return default_value_; }
  bool allowsPrecedence()           const { return allows_precedence_; }
  bool isDefaultValue()             const { return value() == defaultValue(); }
  bool defaultEnabled()             const { return default_enabled_; }
  bool hasGroup()                   const { return group_ != nullptr; }

protected:
  CLI::Option* setupOption(CLI::Option* opt) {
    opt->required(required_);
    if (group_ != nullptr) {
      opt->group(group_->name());
    }
    opt_ = opt;
    return opt;
  }

  std::string makeAppArg() const {
    std::string app_arg = arg_;
    if (short_arg_ != "") {
      app_arg = short_arg_ + std::string(",") + arg_;
    }
    return app_arg;
  }

private:
  std::string short_arg_                                  = "";
  std::string arg_                                        = "";
  std::string desc_                                       = "";
  std::string context_                                    = "";
  std::shared_ptr<Group> group_                           = nullptr;
  bool default_enabled_                                   = false;
  bool required_                                          = false;
  bool allows_precedence_                                 = true;
  T value_                                                = {};
  T default_value_                                        = {};
  AnchorType anchor_                                      = nullptr;
  bool is_cli_arg_                                        = false;
};

struct Config {
  template <typename T>
  using AnchorType = std::shared_ptr<Anchor<T, Argument<T>>>;

  explicit Config(CLI::App* in_app)
    : app_(in_app)
  { }

  template <typename T>
  AnchorType<T> getOption(std::string name) {
    auto iter = options_.find(name);
    if (iter == options_.end()) {
      return nullptr;
    } else {
      auto base = iter->second;
      auto anchor = std::static_pointer_cast<Anchor<T, Argument<T>>>(base);
      return anchor;
    }
  }

  template <typename T>
  AnchorType<T> addOption(std::string name, T& anchor_value) {
    auto iter = options_.find(name);
    if (iter == options_.end()) {
      // @todo: stop using default for warn_override and get this from runtime?
      auto anchor = std::make_shared<Anchor<T, Argument<T>>>(anchor_value);
      options_[name] = anchor;
      option_order_.push_back(anchor);
      return anchor;
    } else {
      auto base = iter->second;
      auto anchor = std::static_pointer_cast<Anchor<T, Argument<T>>>(base);
      return anchor;
    }
  }

  // Match the same argument interface as CLI::App
  template <typename T>
  std::shared_ptr<Argument<T>> addCmd(
    std::string name, T& value, std::string desc
  ) {
    vtAssert(app_ != nullptr, "Must have valid CLI::App");
    auto arg = addOverload<T>(name, desc, "", value, false, true);
    arg->add(*app_);
    return arg;
  }

  template <typename T>
  std::shared_ptr<Argument<T>> addValueOverload(
    std::string name, std::string desc, std::string context, T& value
  ) {
    return addOverload<T>(name, desc, context, value, false);
  }

  template <typename T>
  std::shared_ptr<Argument<T>> addDefaultOverload(
    std::string name, std::string desc, std::string context, T& default_value
  ) {
    return addOverload<T>(name, desc, context, default_value, true);
  }

  template <typename T>
  std::shared_ptr<Argument<T>> addOverload(
    std::string name, std::string desc, std::string context, T& value,
    bool is_default_value, bool is_cli_arg = false, bool higher_precedence = true
  ) {
    auto iter = options_.find(name);
    vtAssertExpr(iter != options_.end());
    if (iter != options_.end()) {
      auto base = iter->second;
      auto anchor = std::static_pointer_cast<Anchor<T, Argument<T>>>(base);
      std::shared_ptr<Argument<T>> arg = nullptr;
      if (is_cli_arg) {
        arg = std::make_shared<Argument<T>>(ArgumentCLITag{}, name, anchor, desc);
      } else {
        arg = std::make_shared<Argument<T>>(name, anchor, desc, context, value, false);
      }
      if (higher_precedence) {
        anchor->addHigherPrecedence(arg);
      } else {
        anchor->addLowerPrecedence(arg);
      }
      return arg;
    }
    return nullptr;
  }

  void resolveOptions() {
    for (auto opt : option_order_) {
      opt->resolve();
    }
  }

  CLI::App* getApp() const { return app_; }

private:
  CLI::App* app_ = nullptr;
  std::unordered_map<std::string, std::shared_ptr<AnchorBase>> options_;
  std::list<std::shared_ptr<AnchorBase>> option_order_;
};

}} /* end namespace vt::arguments */

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARG_H*/
