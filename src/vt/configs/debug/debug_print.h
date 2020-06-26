/*
//@HEADER
// *****************************************************************************
//
//                                debug_print.h
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

#if !defined INCLUDED_VT_CONFIGS_DEBUG_DEBUG_PRINT_H
#define INCLUDED_VT_CONFIGS_DEBUG_DEBUG_PRINT_H

#include "vt/configs/types/types_headers.h"
#include "vt/configs/debug/debug_config.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/configs/debug/debug_var_unused.h"

#include <fmt/format.h>

/*
   === Debug file/line/func functionality ===

#define debug_file_line_args ,__FILE__, __LINE__

#define debug_file_arg
  debug_test(backend,line_file, debug_file_line_args, )
#define debug_file_fmt
  debug_test(backend,line_file, "{}:{} ", )

#define vt_type_print_colorize(debug_type)
  vt_print_colorize_impl(::vt::debug::green(), debug_pretty_print(debug_type), ":")

*/

/// Colorize the first string followed by the second in normal color.
/// Honors colorization checks.
#define vt_print_colorize_impl(color, str, str2)                        \
  (::vt::debug::colorizeOutput() ?                                      \
   (color + std::string(str) + ::vt::debug::reset() +                   \
    std::string(str2)) :                                                \
   std::string(str) + std::string(str2))

#define vt_print_colorize                                               \
  vt_print_colorize_impl(::vt::debug::bd_green(), "vt", ":")

#define vt_proc_print_colorize(proc)                                    \
  vt_print_colorize_impl(::vt::debug::blue(), "[" + std::to_string(proc) + "]", "")

#define vt_debug_argument_option(opt)                                      \
  ::vt::theArgConfig()->vt_debug_ ## opt

#define vt_debug_all_option ::vt::theArgConfig()->vt_debug_all

#define vt_debug_print_impl(force, inconfig, inmode, cat, ctx, ...)     \
  vt::config::ApplyOp<                                                  \
    vt::config::DebugPrintOp,                                           \
    inconfig,                                                           \
    vt::config::CatEnum::cat,                                           \
    vt::config::CtxEnum::ctx,                                           \
    vt::config::ModeEnum::inmode                                        \
  >::apply(vt_debug_argument_option(cat) or force, __VA_ARGS__)

#define vt_debug_print(feature, ctx, ...)                               \
  vt_debug_print_impl(                                                  \
    false, vt::config::DefaultConfig, normal, feature, ctx, __VA_ARGS__ \
  )

#define vt_debug_print_verbose(feature, ctx, ...)                       \
  vt_debug_print_impl(                                                  \
    false, vt::config::DefaultConfig, verbose, feature, ctx, __VA_ARGS__ \
  )

#define vt_make_config(feature, cftype)                                  \
  vt::config::Configuration<                                             \
    static_cast<vt::config::CatEnum>(                                    \
      vt::config::cftype::category | vt::config::CatEnum::feature        \
    ),                                                                   \
    vt::config::cftype::context,                                         \
    vt::config::cftype::mode                                             \
  >

#define vt_config_print_force_impl(cftype, feature, ctx, ...)           \
  vt_debug_print_impl(                                                  \
    true, vt_make_config(feature, cftype), normal, feature, ctx,        \
    __VA_ARGS__                                                         \
  )

#define vt_print_force_impl(feature, ctx, ...)                          \
  vt_config_print_force_impl(VTPrintConfig, feature, ctx, __VA_ARGS__)

#define vt_debug_print_force_impl(feature, ctx, ...)                    \
  vt_config_print_force_impl(DefaultConfig, feature, ctx, __VA_ARGS__)

#if vt_debug_force_enabled
  //#warning "Debug force is enabled"
  #define vt_debug_print_force(feature, ctx, ...)                       \
    vt_debug_print_force_impl(                                          \
      feature, ctx, __VA_ARGS__                                         \
    )
#else
  //#warning "Debug force is not enabled"
  #define vt_debug_print_force vt_debug_print
#endif

#define vt_print(feature, ...)                                          \
  do {                                                                  \
    if (!::vt::theArgConfig()->vt_quiet) {                        \
      vt_print_force_impl(feature, node, __VA_ARGS__);                  \
    }                                                                   \
  } while(0);

#define vt_option_check_enabled(mode, bit) ((mode & bit) not_eq 0)

namespace vt { namespace runtime {
struct Runtime;
}} /* end namespace vt::runtime */

namespace vt {
extern runtime::Runtime* curRT;
} /* end namespace vt */

namespace vt { namespace debug {
NodeType preNode();
}} /* end namespace vt::debug */

namespace vt { namespace config {

template <CatEnum cat, CtxEnum ctx, ModeEnum mod>
struct DebugPrintOp;

template <CatEnum cat, ModeEnum mod, typename Arg, typename... Args>
static inline void debugPrintImpl(NodeType node, Arg&& arg, Args&&... args) {
  bool const verb = vt_option_check_enabled(mod, ModeEnum::verbose);
  if ((verb and ::vt::theArgConfig()->vt_debug_verbose) or not verb) {
    auto user = fmt::format(std::forward<Arg>(arg),std::forward<Args>(args)...);
    fmt::print(
      "{} {} {} {}",
      vt_print_colorize,
      vt_proc_print_colorize(node),
      vt_print_colorize_impl(::vt::debug::green(), PrettyPrintCat<cat>::print(), ":"),
      user
    );
    if (vt_option_check_enabled(mod, ModeEnum::flush) or ::vt::theArgConfig()->alwaysFlush()) {
      fflush(stdout);
    }
  }
}

template <CatEnum cat, ModeEnum mod>
struct DebugPrintOp<cat, CtxEnum::node, mod> {
  template <typename Arg, typename... Args>
  void operator()(bool const rt_option, Arg&& arg, Args&&... args) {
    if (rt_option or vt::theArgConfig()->vt_debug_all) {
      auto no_node = static_cast<NodeType>(-1);
      auto node = vt::curRT != nullptr ? vt::debug::preNode() : no_node;
      debugPrintImpl<cat,mod>(node,std::forward<Arg>(arg),std::forward<Args>(args)...);
    }
  }
};

template <CatEnum cat, ModeEnum mod>
struct DebugPrintOp<cat, CtxEnum::unknown, mod> {
  template <typename Arg, typename... Args>
  void operator()(bool const rt_option, Arg&& arg, Args&&... args) {
    if (rt_option or vt::theArgConfig()->vt_debug_all) {
      debugPrintImpl<cat,mod>(-1,std::forward<Arg>(arg),std::forward<Args>(args)...);
    }
  }
};

////////////////////////////////////////////////////////////////////////////////
// DispatchOp specializations
////////////////////////////////////////////////////////////////////////////////

template <
  template <CatEnum,CtxEnum,ModeEnum> class Op, typename C,
  CatEnum cat, CtxEnum ctx, ModeEnum mode
>
struct DispatchOp {
  template <typename... Args>
  static void apply(bool const op, Args&&... args) {
    return Op<cat,ctx,mode>()(op,std::forward<Args>(args)...);
  }
};

////////////////////////////////////////////////////////////////////////////////
// Context overloads
////////////////////////////////////////////////////////////////////////////////

template <
  template <CatEnum,CtxEnum,ModeEnum> class Op,
  typename C,
  CatEnum cat, CtxEnum ctx, ModeEnum mod,
  typename Enable=void
>
struct CheckEnabled {
  template <typename... Args>
  static void apply(bool const, Args&&... args) {
    return vt::debug::useVars(std::forward<Args>(args)...);
  }
};

template <
  template <CatEnum,CtxEnum,ModeEnum> class Op,
  typename C,
  CatEnum cat, CtxEnum ctx, ModeEnum mod
>
struct CheckEnabled<
  Op, C,
  cat, ctx, mod,
  typename std::enable_if_t<
    (C::context  & ctx) not_eq 0 and
    (C::category & cat) not_eq 0 and
    (C::mode     & mod) not_eq 0
  >
> {
  template <typename... Args>
  static void apply(bool const op, Args&&... args) {
    return DispatchOp<Op,C,cat,ctx,mod>::apply(op,std::forward<Args>(args)...);
  }
};

////////////////////////////////////////////////////////////////////////////////
// Apply the operation
////////////////////////////////////////////////////////////////////////////////

template <
  template <CatEnum,CtxEnum,ModeEnum> class Op, typename C,
  CatEnum cat, CtxEnum ctx, ModeEnum mod
>
struct ApplyOp {
  template <typename... Args>
  static void apply(bool const op, Args&&... args) {
    return CheckEnabled<Op,C,cat,ctx,mod>::apply(op,std::forward<Args>(args)...);
  }
};

}} /* end namespace vt::config */

#endif /*INCLUDED_VT_CONFIGS_DEBUG_DEBUG_PRINT_H*/
