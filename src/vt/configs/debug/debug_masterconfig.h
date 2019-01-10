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

#if !defined INCLUDED_DEBUG_MASTER_CONFIG
#define INCLUDED_DEBUG_MASTER_CONFIG

#include <cmake_config.h>

/*
 * Define the compile-time configuration options. Eventually this will be
 * partially defined with cmake options
 */

#define debug_enabled cmake_config_debug_enabled
#define debug_force_enabled 0

#include "vt/configs/debug/debug_print.h"

#if !debug_enabled
#define backend_debug_modes backend_options_on(none)
#else
#define backend_debug_modes backend_options_on(                          \
  cmake_config_debug_modes                                               \
)
#endif

#define default_threading openmp

#define backend_features backend_options_on(                             \
  default_threading, memory_pool, cmake_config_features                  \
)

#define backend_debug_contexts backend_options_on(                       \
  node, locale, unknown                                                  \
)

#define backend_defaults backend_options_on(                             \
  startup                                                                \
)

#define backend_no_threading                                             \
  !backend_check_enabled(openmp) && !backend_check_enabled(stdthread)

#endif  /*INCLUDED_DEBUG_MASTER_CONFIG*/
