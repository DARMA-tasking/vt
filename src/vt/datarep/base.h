/*
//@HEADER
// *****************************************************************************
//
//                                    base.h
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

#if !defined INCLUDED_VT_DATAREP_BASE_H
#define INCLUDED_VT_DATAREP_BASE_H

#include "vt/configs/types/types_type.h"
#include "vt/configs/types/types_sentinels.h"

namespace vt { namespace datarep {

enum struct DataRepEnum : int8_t {
  Normal = 0,
  PersistentAcrossVersions = 1
};

namespace detail {

struct ReaderBase {};

template <typename IndexT>
struct DR_Base : ReaderBase {

  DR_Base() = default;
  explicit DR_Base(DataRepIDType in_handle)
    : handle_(in_handle)
  { }

  DR_Base(
    DataRepIDType in_handle, IndexT in_index, TagType in_tag,
    DataRepEnum in_hint = DataRepEnum::Normal
  ) : handle_(in_handle),
      tag_(in_tag),
      is_proxy_(true),
      index_(in_index),
      hint_(in_hint)
  { }

  DataRepIDType getHandleID() const { return handle_; }
  TagType getTag() const { return tag_; }
  bool isProxy() const { return is_proxy_; }
  IndexT getIndex() const { return index_; }
  DataRepEnum getHint() const { return hint_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | handle_
      | tag_
      | is_proxy_
      | index_
      | hint_;
  }

protected:
  DataRepIDType handle_ = no_datarep;
  TagType tag_ = no_tag;
  bool is_proxy_ = false;
  IndexT index_ = {};
  DataRepEnum hint_ = DataRepEnum::Normal;
};

}}} /* end namespace vt::datarep::detail */

#endif /*INCLUDED_VT_DATAREP_BASE_H*/
