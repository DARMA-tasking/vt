/*
//@HEADER
// ************************************************************************
//
//                          handler.cc
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

#include "vt/handler/handler.h"
#include "vt/utils/bits/bits_common.h"

namespace vt {

/*static*/ HandlerType HandlerManager::makeHandler(
  bool is_auto, bool is_functor, HandlerIdentifierType id, bool is_objgroup
) {
  HandlerType new_han = blank_handler;
  HandlerManager::setHandlerAuto(new_han, is_auto);
  HandlerManager::setHandlerObjGroup(new_han, is_objgroup);
  HandlerManager::setHandlerFunctor(new_han, is_functor);
  HandlerManager::setHandlerIdentifier(new_han, id);

  debug_print(
    handler, node,
    "HandlerManager::makeHandler: is_functor={}, is_auto={}, is_objgroup={},"
    " id={}, han={}\n",
    is_functor, is_auto, is_objgroup, id, new_han
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

/*static*/ void HandlerManager::setHandlerIdentifier(
  HandlerType& han, HandlerIdentifierType id
) {
  BitPackerType::setField<HandlerBitsType::Identifier, handler_id_num_bits>(
    han, id
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

/*static*/ bool HandlerManager::isHandlerAuto(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::Auto>(han);
}

/*static*/ bool HandlerManager::isHandlerFunctor(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::Functor>(han);
}

/*static*/ bool HandlerManager::isHandlerObjGroup(HandlerType han) {
  return BitPackerType::boolGetField<HandlerBitsType::ObjGroup>(han);
}

} // end namespace vt

