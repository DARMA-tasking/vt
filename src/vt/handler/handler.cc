/*
//@HEADER
// *****************************************************************************
//
//                                  handler.cc
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

#include "vt/handler/handler.h"
#include "vt/utils/bits/bits_common.h"

namespace vt {

/*static*/ HandlerType HandlerManager::makeHandler(
  bool is_auto, bool is_functor, HandlerIdentifierType id, bool is_member,
  bool is_objgroup, HandlerControlType control, bool is_trace
) {
  HandlerType new_han = blank_handler;
  HandlerManager::setHandlerAuto(new_han, is_auto);
  HandlerManager::setHandlerObjGroup(new_han, is_objgroup);
  HandlerManager::setHandlerFunctor(new_han, is_functor);
  HandlerManager::setHandlerMember(new_han, is_member);
  HandlerManager::setHandlerIdentifier(new_han, id);

#if vt_check_enabled(trace_enabled)
  HandlerManager::setHandlerTrace(new_han, is_trace);
#endif

  if (control != 0) {
    HandlerManager::setHandlerControl(new_han, control);
  }

  vt_debug_print(
    handler, node,
    "HandlerManager::makeHandler: is_functor={}, is_auto={}, is_objgroup={},"
    "is_memer={} id={:x}, control={:x}, han={:x}, is_trace={}\n",
    is_functor, is_auto, is_objgroup, is_member, id, control, new_han,
    is_trace
  );

  return new_han;
}

/*static*/ HandlerIdentifierType HandlerManager::getHandlerIdentifier(
  HandlerType han
) {
  return BitPackerType::getField<
    HandlerBitsType::Identifier, handler_id_num_bits, HandlerIdentifierType
  >(han);
}

/*static*/ HandlerControlType HandlerManager::getHandlerControl(
  HandlerType han
) {
  return BitPackerType::getField<
    HandlerBitsType::Control, control_num_bits, HandlerControlType
  >(han);
}

/*static*/ void HandlerManager::setHandlerIdentifier(
  HandlerType& han, HandlerIdentifierType id
) {
  BitPackerType::setField<HandlerBitsType::Identifier, handler_id_num_bits>(
    han, id
  );
}

/*static*/ void HandlerManager::setHandlerControl(
  HandlerType& han, HandlerControlType control
) {
  BitPackerType::setField<HandlerBitsType::Control, control_num_bits>(
    han, control
  );
}

/*static*/ void HandlerManager::setHandlerAuto(HandlerType& han, bool is_auto) {
  BitPackerType::boolSetField<HandlerBitsType::Auto>(han, is_auto);
}

/*static*/ void HandlerManager::setHandlerObjGroup(
  HandlerType& han, bool is_objgroup
) {
  BitPackerType::boolSetField<HandlerBitsType::ObjGroup>(han, is_objgroup);
}

/*static*/ void HandlerManager::setHandlerFunctor(
  HandlerType& han, bool is_functor
) {
  BitPackerType::boolSetField<HandlerBitsType::Functor>(han, is_functor);
}

/*static*/ void HandlerManager::setHandlerMember(
  HandlerType& han, bool is_member
) {
  BitPackerType::boolSetField<HandlerBitsType::Member>(han, is_member);
}

/*static*/ bool HandlerManager::isHandlerAuto(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::Auto>(han);
}

/*static*/ bool HandlerManager::isHandlerFunctor(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::Functor>(han);
}

/*static*/ bool HandlerManager::isHandlerObjGroup(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::ObjGroup>(han);
}

/*static*/ bool HandlerManager::isHandlerMember(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::Member>(han);
}

#if vt_check_enabled(trace_enabled)
/*static*/ bool HandlerManager::isHandlerTrace(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::Trace>(han);
}

/*static*/ void HandlerManager::setHandlerTrace(
  HandlerType& han, bool is_trace
) {
  BitPackerType::boolSetField<HandlerBitsType::Trace>(han, is_trace);
}
#endif

} // end namespace vt
