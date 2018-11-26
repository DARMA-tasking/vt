/*
//@HEADER
// ************************************************************************
//
//                          signal_holder.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H
#define INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base.h"

#include <cassert>
#include <unordered_map>
#include <vector>
#include <memory>
#include <list>
#include <cstdlib>

namespace vt { namespace pipe { namespace signal {

template <typename SignalT>
struct SignalHolder {
  using DataType             = typename SignalT::DataType;
  using DataPtrType          = DataType*;
  using SigCountType         = int32_t;
  using SignalListType       = std::vector<SignalT>;
  using ListenerBaseType     = callback::CallbackBase<SignalT>;
  using ListenerType         = std::unique_ptr<ListenerBaseType>;
  using ListenerListType     = std::list<ListenerType>;
  using ListenerListIterType = typename ListenerListType::iterator;
  using ListenerPtrType      = ListenerBaseType*;
  using ListenerMapType      = std::unordered_map<PipeType,ListenerListType>;
  using SignalMapType        = std::unordered_map<PipeType,SignalListType>;
  using CountMapType         = std::unordered_map<PipeType,SigCountType>;
  using ListenerMapIterType  = typename ListenerMapType::iterator;

  void addSignal(PipeType const& pid, DataPtrType in_data);
  void removeListener(PipeType const& pid, ListenerPtrType listener);
  ListenerListIterType removeListener(
    ListenerMapIterType map_iter, ListenerListIterType iter
  );
  void clearAllListeners(PipeType const& pid);
  void clearAllSignals(PipeType const& pid);
  void clearPipe(PipeType const& pid);
  void deliverAll(PipeType const& pid, DataPtrType data);
  void addListener(PipeType const& pid, ListenerType&& cb);
  void setCount(PipeType const& pid, SigCountType const& count);
  SigCountType getCount(PipeType const& pid);
  void applySignal(
    ListenerPtrType listener, DataPtrType data, PipeType const& pid
  );
  bool finished(ListenerPtrType listener) const;
  bool exists(PipeType const& pipe) const;

private:
  SignalMapType pending_holder_;
  CountMapType listener_count_;
  ListenerMapType listeners_;
};

}}} /* end namespace vt::pipe::signal */

#endif /*INCLUDED_PIPE_SIGNAL_SIGNAL_HOLDER_H*/
