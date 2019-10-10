/*
//@HEADER
// *****************************************************************************
//
//                              hierlb_constants.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H

#include "vt/config.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

static constexpr double   const hierlb_threshold_p      = 0.8f;
static constexpr double   const hierlb_max_threshold_p  = 1.004f;
static constexpr NodeType const hierlb_nary             = 2;
static constexpr NodeType const hierlb_root             = 0;
static constexpr int32_t  const hierlb_bin_size         = 10;
static constexpr double   const hierlb_no_load_sentinel = -1.0f;
static constexpr double   const hierlb_tolerance        = 0.05f;
static constexpr bool     const hierlb_auto_threshold_p = true;

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_CONSTANTS_H*/
