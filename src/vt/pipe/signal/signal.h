/*
//@HEADER
// *****************************************************************************
//
//                                   signal.h
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

#if !defined INCLUDED_VT_PIPE_SIGNAL_SIGNAL_H
#define INCLUDED_VT_PIPE_SIGNAL_SIGNAL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"

#include <cstdlib>

namespace vt { namespace pipe { namespace signal {

struct SignalBase {};

template <typename DataT>
struct Signal : SignalBase {
  using DataType    = DataT;
  using DataPtrType = DataType*;

  Signal() = default;
  Signal(Signal const&) = default;
  Signal(Signal&&) = default;
  Signal& operator=(Signal const&) = default;

  explicit Signal(DataPtrType in_ptr)
    : Signal(in_ptr,no_tag)
  { }
  Signal(DataPtrType in_ptr, TagType in_tag)
    : data_ptr_(in_ptr), signal_tag_(in_tag)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | signal_tag_;

    s.skip(data_ptr_);
  }

public:
  DataPtrType get() const { return data_ptr_; }
  TagType getTag() const { return signal_tag_; }

private:
  DataPtrType data_ptr_ = nullptr;
  TagType signal_tag_ = no_tag;
};

using SigVoidType = int8_t;
using SignalVoid = Signal<SigVoidType>;

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_VT_PIPE_SIGNAL_SIGNAL_H*/
