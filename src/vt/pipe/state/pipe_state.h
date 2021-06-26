/*
//@HEADER
// *****************************************************************************
//
//                                 pipe_state.h
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

#if !defined INCLUDED_VT_PIPE_STATE_PIPE_STATE_H
#define INCLUDED_VT_PIPE_STATE_PIPE_STATE_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"

#include <functional>

namespace vt { namespace pipe {

struct PipeState {
  using DispatchFuncType = std::function<void(void*)>;

  PipeState(
    PipeType const& in_pipe, RefType const& in_signals, RefType const& in_lis,
    bool const& in_typeless = false
  );

  PipeState(PipeType const& in_pipe, bool const& in_typeless = false);

  void signalRecv();
  void listenerReg();
  bool isAutomatic() const;
  bool isTypeless() const;
  bool isPersist() const;
  PipeType getPipe() const;
  bool finished() const;
  RefType refsPerListener() const;

  bool hasDispatch() const;
  void setDispatch(DispatchFuncType in_dispatch);
  void dispatch(void* ptr);

private:
  bool automatic_                 = false;
  bool typeless_                  = false;
  RefType num_signals_expected_   = -1;
  RefType num_signals_received_   = 0;
  RefType num_listeners_expected_ = -1;
  RefType num_listeners_received_ = 0;
  PipeType pipe_                  = no_pipe;
  DispatchFuncType dispatch_      = nullptr;
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_VT_PIPE_STATE_PIPE_STATE_H*/
