/*
//@HEADER
// *****************************************************************************
//
//                                 code_class.h
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

#if !defined INCLUDED_VT_CONFIGS_ERROR_CODE_CLASS_H
#define INCLUDED_VT_CONFIGS_ERROR_CODE_CLASS_H

#include <cstdlib>

namespace vt { namespace error {

enum struct ModuleErrorCode : int8_t {
  VrtCollection    = 0,
  VrtContext       = 1,
  RDMA             = 2,
  Barrier          = 3,
  Reduce           = 4,
  Scatter          = 5,
  Epoch            = 6,
  Event            = 7,
  Group            = 8,
  Handler          = 9,
  LB               = 10,
  Active           = 11,
  Pipe             = 12,
  Pool             = 13,
  Runtime          = 14,
  Timing           = 15,
  Termination      = 16,
  Trace            = 17,
  Serialization    = 18,
  Util             = 19,
  Sequence         = 20,
  Registry         = 21,
  Location         = 22,
  Index            = 23,
  Mapping          = 24,
  Worker           = 25,
  Parameterization = 26
};

}} /* end namespace vt::error */

#endif /*INCLUDED_VT_CONFIGS_ERROR_CODE_CLASS_H*/
