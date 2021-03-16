/*
//@HEADER
// *****************************************************************************
//
//                                debug_config.h
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

#if !defined INCLUDED_VT_CONFIGS_DEBUG_DEBUG_CONFIG_H
#define INCLUDED_VT_CONFIGS_DEBUG_DEBUG_CONFIG_H

#include <cstdint>

namespace vt { namespace config {

enum CatEnum : uint64_t {
  none         = 1ull<<0,
  gen          = 1ull<<1,
  runtime      = 1ull<<2,
  active       = 1ull<<3,
  term         = 1ull<<4,
  termds       = 1ull<<5,
  barrier      = 1ull<<6,
  event        = 1ull<<7,
  pipe         = 1ull<<8,
  pool         = 1ull<<9,
  reduce       = 1ull<<10,
  rdma         = 1ull<<11,
  rdma_channel = 1ull<<12,
  rdma_state   = 1ull<<13,
  param        = 1ull<<14,
  handler      = 1ull<<15,
  hierlb       = 1ull<<16,
  scatter      = 1ull<<17,
  sequence     = 1ull<<18,
  sequence_vrt = 1ull<<19,
  serial_msg   = 1ull<<20,
  trace        = 1ull<<21,
  location     = 1ull<<22,
  lb           = 1ull<<23,
  vrt          = 1ull<<24,
  vrt_coll     = 1ull<<25,
  worker       = 1ull<<26,
  group        = 1ull<<27,
  broadcast    = 1ull<<28,
  objgroup     = 1ull<<29,
  gossiplb     = 1ull<<30,
  phase        = 1ull<<31,
  context      = 1ull<<32
};

enum CtxEnum : uint64_t {
  node         = 1ull<<0,
  unknown      = 1ull<<2
};

enum ModeEnum : uint64_t {
  normal       = 1ull<<0,
  verbose      = 1ull<<1,
  verbose_2    = 1ull<<2,
  flush        = 1ull<<3,
  startup      = 1ull<<4,
  line_file    = 1ull<<5,
  function     = 1ull<<6
};

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/*
 * Pretty print for CatEnum output
 */

template <CatEnum cat>
struct PrettyPrintCat;

#define vt_option_category_pretty_print(option, option_string)           \
  template <>                                                            \
  struct PrettyPrintCat<CatEnum::option> {                               \
    static constexpr char const* print() { return option_string; } \
    static constexpr char const* const str = option_string;              \
  };

vt_option_category_pretty_print(none,         "none")
vt_option_category_pretty_print(active,       "active")
vt_option_category_pretty_print(barrier,      "barrier")
vt_option_category_pretty_print(broadcast,    "bcast")
vt_option_category_pretty_print(context,      "context")
vt_option_category_pretty_print(event,        "event")
vt_option_category_pretty_print(gen,          "general")
vt_option_category_pretty_print(group,        "group")
vt_option_category_pretty_print(handler,      "handler")
vt_option_category_pretty_print(hierlb,       "HierarchicalLB")
vt_option_category_pretty_print(gossiplb,     "GossipLB")
vt_option_category_pretty_print(lb,           "lb")
vt_option_category_pretty_print(location,     "location")
vt_option_category_pretty_print(objgroup,     "objgroup")
vt_option_category_pretty_print(param,        "parameterization")
vt_option_category_pretty_print(phase,        "phase")
vt_option_category_pretty_print(pipe,         "pipe")
vt_option_category_pretty_print(pool,         "pool")
vt_option_category_pretty_print(reduce,       "reduce")
vt_option_category_pretty_print(rdma,         "RDMA")
vt_option_category_pretty_print(rdma_channel, "RDMA Channel")
vt_option_category_pretty_print(rdma_state,   "RDMA State")
vt_option_category_pretty_print(runtime,      "runtime")
vt_option_category_pretty_print(scatter,      "scatter")
vt_option_category_pretty_print(sequence,     "sequencer")
vt_option_category_pretty_print(sequence_vrt, "sequencer-vrt")
vt_option_category_pretty_print(serial_msg,   "serialized-msg")
vt_option_category_pretty_print(term,         "termination")
vt_option_category_pretty_print(termds,       "dijkstra-scholten-TD")
vt_option_category_pretty_print(trace,        "trace")
vt_option_category_pretty_print(vrt,          "vc")
vt_option_category_pretty_print(vrt_coll,     "vcc")
vt_option_category_pretty_print(worker,       "worker")

/*
 * Pretty print for CtxEnum output
 */

template <CtxEnum ctx>
struct PrettyPrintCtx;

#define vt_option_context_pretty_print(option, option_string)            \
  template <>                                                            \
  struct PrettyPrintCtx<CtxEnum::option> {                               \
    static constexpr char const* print() { return option_string; } \
    static constexpr char const* const str = option_string;              \
  };

vt_option_context_pretty_print(node,     "Print current node")
vt_option_context_pretty_print(unknown,  "Print no processor")

/*
 * Pretty print for ModeEnum output
 */

template <ModeEnum cat>
struct PrettyPrintMode;

#define vt_option_mode_pretty_print(option, option_string)               \
  template <>                                                            \
  struct PrettyPrintMode<ModeEnum::option> {                             \
    static constexpr char const* print() { return option_string; } \
    static constexpr char const* const str = option_string;              \
  };

vt_option_mode_pretty_print(normal,    "normal")
vt_option_mode_pretty_print(verbose,   "verbose")
vt_option_mode_pretty_print(verbose_2, "verbose_2")
vt_option_mode_pretty_print(flush,     "flush all debug prints")
vt_option_mode_pretty_print(startup,   "print function context_debug")
vt_option_mode_pretty_print(line_file, "print line/file debug")
vt_option_mode_pretty_print(function,  "startup sequence")

/*
 * vt configuration type
 */

template <CatEnum cat, CtxEnum ctx, ModeEnum mod>
struct Configuration {
  static constexpr auto category = cat;
  static constexpr auto context  = ctx;
  static constexpr auto mode     = mod;
};

}} /* end namespace vt::config */

#include "vt/configs/features/features_featureswitch.h"
#include "vt/configs/features/features_defines.h"
#include "vt/configs/features/features_enableif.h"
#include "vt/configs/debug/debug_printconst.h"

#endif /*INCLUDED_VT_CONFIGS_DEBUG_DEBUG_CONFIG_H*/
