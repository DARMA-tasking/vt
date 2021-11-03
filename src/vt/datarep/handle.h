/*
//@HEADER
// *****************************************************************************
//
//                                   handle.h
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

#if !defined INCLUDED_VT_DATAREP_HANDLE_H
#define INCLUDED_VT_DATAREP_HANDLE_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_sentinels.h"
#include "vt/datarep/base.h"

#include <memory>

namespace vt { namespace datarep {

template <typename T, typename IndexT = int8_t>
struct DR : detail::DR_Base<IndexT> {

  DR() = default;
  DR(DR const&) = default;
  DR(DR&&) = default;
  DR& operator=(DR const&) = default;
  ~DR();

  template <typename U>
  void publish(DataVersionType version, U&& data);

  void unpublish(DataVersionType version);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    detail::DR_Base<IndexT>::serialize(s);
  }

private:
  struct DR_TAG_CONSTRUCT {};

  DR(DR_TAG_CONSTRUCT, DataRepIDType in_handle)
    : detail::DR_Base<IndexT>(in_handle)
  {}

  DR(
    DR_TAG_CONSTRUCT, DataRepIDType in_handle, IndexT in_index,
    TagType in_tag = no_tag, DataRepEnum in_hint = DataRepEnum::Normal
  ) : detail::DR_Base<IndexT>(in_handle, in_index, in_tag, in_hint)
  {}

  friend struct DataReplicator;
};

}} /* end namespace vt::datarep */

#endif /*INCLUDED_VT_DATAREP_HANDLE_H*/
