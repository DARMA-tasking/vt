/*
//@HEADER
// ************************************************************************
//
//                            sizing.h
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

#if !defined INCLUDED_VT_SERIALIZATION_AUTO_SIZING_SIZING_H
#define INCLUDED_VT_SERIALIZATION_AUTO_SIZING_SIZING_H

#include "vt/config.h"
#include "vt/serialization/serialize_interface.h"

#if HAS_SERIALIZATION_LIBRARY
  #define HAS_DETECTION_COMPONENT 1
  #include "serialization_library_headers.h"
  #include "traits/serializable_traits.h"
#endif

namespace vt { namespace serialization {

template <typename MsgT, typename=void>
struct Size {
  static std::size_t getSize(MsgT* msg);
};

#if HAS_SERIALIZATION_LIBRARY
template <typename MsgT>
struct Size<
  MsgT,
  typename std::enable_if_t<
    ::serdes::SerializableTraits<MsgT>::has_serialize_function
  >
> {
  static std::size_t getSize(MsgT* msg) {
    return ::serialization::interface::getSize<MsgT>(*msg);
  }
};

template <typename MsgT>
struct Size<
  MsgT,
  typename std::enable_if_t<
    !::serdes::SerializableTraits<MsgT>::has_serialize_function
  >
> {
  static std::size_t getSize(MsgT* msg) {
    return sizeof(MsgT);
  }
};

#else

template <typename MsgT>
struct Size<
  MsgT,
  typename std::enable_if_t<std::true_type>
> {
  static std::size_t getSize(MsgT* msg) {
    return sizeof(MsgT);
  }
};

#endif


}} /* end namespace vt::serialization */

#endif /*INCLUDED_VT_SERIALIZATION_AUTO_SIZING_SIZING_H*/
