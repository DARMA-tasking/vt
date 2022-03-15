/*
//@HEADER
// *****************************************************************************
//
//                                  handler.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_HANDLER_HANDLER_H
#define INCLUDED_VT_HANDLER_HANDLER_H

#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdint>

#include "vt/config.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/activefn/activefn.h"
#include "vt/registry/auto/auto_registry_type_enum.h"

namespace vt {

using HandlerIdentifierType = uint64_t;

// HandlerControlType: u32-bit field, limited to 20 bits in pattern
using HandlerControlType = uint32_t;

static constexpr HandlerIdentifierType const first_handle_identifier = 1;
static constexpr HandlerIdentifierType const uninitialized_handle_identifier = u64empty;
static constexpr HandlerType const blank_handler = 0;

static constexpr BitCountType const auto_num_bits = 1;
static constexpr BitCountType const functor_num_bits = 1;
static constexpr BitCountType const trace_num_bits = 1;
static constexpr BitCountType const control_num_bits = 20;
static constexpr BitCountType const base_msg_derived_num_bits = 1;
static constexpr BitCountType const registry_type_bits = 4;
static constexpr BitCountType const handler_id_num_bits =
 BitCounterType<HandlerType>::value - (
     auto_num_bits
   + functor_num_bits
   + control_num_bits
   + trace_num_bits
   + base_msg_derived_num_bits
   + registry_type_bits
 );

// eHandlerBits::ObjGroup identifies the handler as targeting an objgroup; the
// control bits are an extensible field used for module-specific sub-handlers
enum eHandlerBits {
  Auto   = 0,
  Functor         = eHandlerBits::Auto           + auto_num_bits,
  Trace           = eHandlerBits::Functor        + functor_num_bits,
  Control         = eHandlerBits::Trace          + trace_num_bits,
  BaseMsgDerived  = eHandlerBits::Control        + control_num_bits,
  RegistryType    = eHandlerBits::BaseMsgDerived + base_msg_derived_num_bits,
  Identifier      = eHandlerBits::RegistryType   + registry_type_bits,
};

struct HandlerManager {
  using HandlerBitsType = eHandlerBits;
  using RegistryTypeEnum = auto_registry::RegistryTypeEnum;

  HandlerManager() = default;

  static HandlerType makeHandler(
    bool is_auto, bool is_functor, HandlerIdentifierType id,
    RegistryTypeEnum const registry_type, HandlerControlType control = 0,
    bool is_trace = true, bool is_base_msg_derived = true
  );
  static void setHandlerIdentifier(HandlerType& han, HandlerIdentifierType id);
  static void setHandlerControl(HandlerType& han, HandlerControlType control);

  static HandlerIdentifierType getHandlerIdentifier(HandlerType han);
  static HandlerControlType getHandlerControl(HandlerType han);
  static RegistryTypeEnum getHandlerRegistryType(HandlerType han);
  static void setHandlerAuto(HandlerType& han, bool is_auto);
  static void setHandlerFunctor(HandlerType& han, bool is_functor);
  static void setHandlerBaseMsgDerived(HandlerType& han, bool is_base_msg_derived);
  static void setHandlerRegistryType(HandlerType& han, RegistryTypeEnum registryType);
  static bool isHandlerAuto(HandlerType han);
  static bool isHandlerFunctor(HandlerType han);
  static bool isHandlerObjGroup(HandlerType han);
  static bool isHandlerMember(HandlerType han);
  static bool isHandlerBaseMsgDerived(HandlerType han);
#if vt_check_enabled(trace_enabled)
  static void setHandlerTrace(HandlerType& han, bool is_trace);
  static bool isHandlerTrace(HandlerType han);
#endif
};

} //end namespace vt

#endif /*INCLUDED_VT_HANDLER_HANDLER_H*/
