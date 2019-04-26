/*
//@HEADER
// ************************************************************************
//
//                          debug_masterconfig.h
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

#if !defined INCLUDED_VT_CONFIGS_DEBUG_DEBUG_MASTERCONFIG_H
#define INCLUDED_VT_CONFIGS_DEBUG_DEBUG_MASTERCONFIG_H

#include <cmake_config.h>

/*
 * Define the compile-time configuration options. Eventually this will be
 * partially defined with cmake options
 */

#define debug_enabled cmake_config_debug_enabled
#define debug_force_enabled 0

#include "vt/configs/debug/debug_config.h"
#include "vt/configs/debug/debug_print.h"

namespace vt { namespace config {

#if !debug_enabled
using DefaultConfig = Configuration<
  static_cast<CatEnum>(CatEnum::none),
  static_cast<CtxEnum>(0ull),
  static_cast<ModeEnum>(0ull)
>;
#else
#define vt_backend_debug_categories cmake_config_debug_modes
using DefaultConfig = Configuration<
  static_cast<CatEnum>(
    vt_backend_debug_categories | CatEnum::none
  ),
  static_cast<CtxEnum>(
    CtxEnum::node |
    CtxEnum::unknown
  ),
  static_cast<ModeEnum>(
    ModeEnum::normal |
    ModeEnum::flush
  )
>;
#endif

}} /* end namespace vt::config */

#define vt_default_threading openmp

#define backend_no_threading                                             \
  !backend_check_enabled(openmp) && !backend_check_enabled(stdthread)

#endif /*INCLUDED_VT_CONFIGS_DEBUG_DEBUG_MASTERCONFIG_H*/
