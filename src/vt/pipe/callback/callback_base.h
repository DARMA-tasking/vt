/*
//@HEADER
// *****************************************************************************
//
//                               callback_base.h
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

#if !defined INCLUDED_VT_PIPE_CALLBACK_CALLBACK_BASE_H
#define INCLUDED_VT_PIPE_CALLBACK_CALLBACK_BASE_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/context/context.h"

namespace vt { namespace pipe { namespace callback {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct CallbackPersistTagType   {} CallbackPersistTag   {};
static struct CallbackSingleUseTagType {} CallbackSingleUseTag {};
static struct CallbackMultiUseTagType  {} CallbackMultiUseTag  {};
static struct CallbackExplicitTagType  {} CallbackExplicitTag  {};
#pragma GCC diagnostic pop

template <typename SignalT>
struct CallbackBase {
  using SignalType = SignalT;
  using SignalDataType = typename SignalT::DataType;

  CallbackBase(CallbackBase const&) = default;
  CallbackBase(CallbackBase&&) = default;
  CallbackBase& operator=(CallbackBase const&) = default;

  CallbackBase() :
    CallbackBase(CallbackSingleUseTag)
  { }
  explicit CallbackBase(CallbackPersistTagType)
    : reference_counted_(false)
  { }
  explicit CallbackBase(CallbackSingleUseTagType)
    : reference_counted_(true),
      refs_(1)
  { }
  CallbackBase(CallbackMultiUseTagType, RefType const& in_num_refs)
    : reference_counted_(true),
      refs_(in_num_refs)
  { }
  CallbackBase(
    CallbackExplicitTagType, bool const& persist, RefType const& in_refs = -1
  ) : reference_counted_(!persist), refs_(in_refs)
  { }

  virtual ~CallbackBase() = default;

public:
  bool expired() const { return reference_counted_ && refs_ < 1; }

protected:
  virtual void trigger_(SignalDataType* data) = 0;

  virtual void trigger_(SignalDataType* data, PipeType const& pipe_id) {
    return trigger_(data);
  }

public:
  void trigger(SignalDataType* data, PipeType const& pipe_id) {
    if (reference_counted_) {
      refs_--;
      triggered_++;
    }
    vt_debug_print(
      verbose, pipe,
      "CallbackBase: (before) invoke trigger_: pipe={:x}\n",
      pipe_id
    );
    trigger_(data,pipe_id);
    vt_debug_print(
      verbose, pipe,
      "CallbackBase: (after) invoke trigger_: pipe={:x}\n",
      pipe_id
    );
  }

  bool finished() const {
    if (reference_counted_) {
      vtAssert(refs_ != -1, "refs_ must be a valid positive integer");
      return refs_ == 0;
    } else {
      return false;
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | reference_counted_ | triggered_ | refs_;
  }

private:
  bool reference_counted_ = true;
  RefType triggered_      = 0;
  RefType refs_           = 1;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_VT_PIPE_CALLBACK_CALLBACK_BASE_H*/
