/*
//@HEADER
// *****************************************************************************
//
//                                debug_print.cc
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

#include "vt/configs/debug/debug_masterconfig.h"
#include "vt/configs/debug/debug_print.h"
#include "vt/configs/error/hard_error.h"

#define vt_scoped_modifier_opt_definition(opt, val)                            \
  ScopedModifier_##opt##_##val::ScopedModifier_##opt##_##val()                 \
      : orig_val{::vt::theConfig()->vt_debug_##opt} {                          \
    ::vt::theConfig()->vt_debug_##opt = val;                                   \
  }                                                                            \
                                                                               \
  ScopedModifier_##opt##_##val::~ScopedModifier_##opt##_##val() {              \
    ::vt::theConfig()->vt_debug_##opt = orig_val;                              \
  }                                                                            \
                                                                               \
  ScopedModifier_##opt##_##val createScopedModifier_##opt##_##val() {          \
    if (not ::vt::curRT) {                                                     \
      vtAbort("Trying to read config when VT is not initialized");             \
    }                                                                          \
                                                                               \
    return ScopedModifier_##opt##_##val{};                                     \
  }

#define vt_define_debug_scoped_modifiers(opt)                                  \
  vt_scoped_modifier_opt_definition(opt, true)                                 \
  vt_scoped_modifier_opt_definition(opt, false)

namespace vt {

vt_define_debug_scoped_modifiers(all)
vt_define_debug_scoped_modifiers(none)
vt_define_debug_scoped_modifiers(gen)
vt_define_debug_scoped_modifiers(runtime)
vt_define_debug_scoped_modifiers(active)
vt_define_debug_scoped_modifiers(term)
vt_define_debug_scoped_modifiers(termds)
vt_define_debug_scoped_modifiers(barrier)
vt_define_debug_scoped_modifiers(event)
vt_define_debug_scoped_modifiers(pipe)
vt_define_debug_scoped_modifiers(pool)
vt_define_debug_scoped_modifiers(reduce)
vt_define_debug_scoped_modifiers(rdma)
vt_define_debug_scoped_modifiers(rdma_channel)
vt_define_debug_scoped_modifiers(rdma_state)
vt_define_debug_scoped_modifiers(param)
vt_define_debug_scoped_modifiers(handler)
vt_define_debug_scoped_modifiers(hierlb)
vt_define_debug_scoped_modifiers(temperedlb)
vt_define_debug_scoped_modifiers(scatter)
vt_define_debug_scoped_modifiers(serial_msg)
vt_define_debug_scoped_modifiers(trace)
vt_define_debug_scoped_modifiers(location)
vt_define_debug_scoped_modifiers(lb)
vt_define_debug_scoped_modifiers(vrt)
vt_define_debug_scoped_modifiers(vrt_coll)
vt_define_debug_scoped_modifiers(worker)
vt_define_debug_scoped_modifiers(group)
vt_define_debug_scoped_modifiers(broadcast)
vt_define_debug_scoped_modifiers(objgroup)
vt_define_debug_scoped_modifiers(phase)
vt_define_debug_scoped_modifiers(context)
vt_define_debug_scoped_modifiers(epoch)

}
