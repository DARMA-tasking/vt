/*
//@HEADER
// ************************************************************************
//
//                          handler.h
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

#if !defined INCLUDED_HANDLER_HANDLER_H
#define INCLUDED_HANDLER_HANDLER_H

#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/activefn/activefn.h"

namespace vt {

using HandlerIdentifierType = uint64_t;

static constexpr HandlerIdentifierType const first_handle_identifier = 1;
static constexpr HandlerIdentifierType const uninitialized_handle_identifier = -1;
static constexpr HandlerType const blank_handler = 0;

static constexpr BitCountType const auto_num_bits = 1;
static constexpr BitCountType const functor_num_bits = 1;
static constexpr BitCountType const handler_id_num_bits =
 BitCounterType<HandlerType>::value - (auto_num_bits + functor_num_bits);

enum eHandlerBits {
  Auto       = 0,
  Functor    = eHandlerBits::Auto       + auto_num_bits,
  Identifier = eHandlerBits::Functor    + functor_num_bits
};

struct HandlerManager {
  using HandlerBitsType = eHandlerBits;

  HandlerManager() = default;

  static HandlerType makeHandler(
    bool is_auto, bool is_functor, HandlerIdentifierType id
  );
  static void setHandlerIdentifier(
    HandlerType& han, HandlerIdentifierType ident
  );
  static HandlerIdentifierType getHandlerIdentifier(HandlerType han);
  static void setHandlerAuto(HandlerType& han, bool is_auto);
  static void setHandlerFunctor(HandlerType& han, bool is_functor);
  static bool isHandlerAuto(HandlerType han);
  static bool isHandlerFunctor(HandlerType han);
};

} //end namespace vt

#endif /*INCLUDED_HANDLER_HANDLER_H*/
