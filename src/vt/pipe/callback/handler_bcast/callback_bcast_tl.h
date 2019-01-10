/*
//@HEADER
// ************************************************************************
//
//                          callback_bcast_tl.h
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

#if !defined INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H
#define INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/active.h"

namespace vt { namespace pipe { namespace callback {

struct CallbackBcastTypeless : CallbackBaseTL<CallbackBcastTypeless> {
  CallbackBcastTypeless() = default;
  CallbackBcastTypeless(CallbackBcastTypeless const&) = default;
  CallbackBcastTypeless(CallbackBcastTypeless&&) = default;
  CallbackBcastTypeless& operator=(CallbackBcastTypeless const&) = default;

  CallbackBcastTypeless(
    HandlerType const& in_handler, bool const& in_include
  );

  HandlerType getHandler() const { return handler_; }
  bool getIncSender() const { return include_sender_; }

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackBcastTypeless const& other) const {
    return
      other.include_sender_ == include_sender_ &&
      other.handler_ == handler_;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);
  void triggerVoid(PipeType const& pipe);

private:
  HandlerType handler_ = uninitialized_handler;
  bool include_sender_  = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_HANDLER_BCAST_CALLBACK_BCAST_TL_H*/
