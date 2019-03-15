/*
//@HEADER
// ************************************************************************
//
//                           manager.h
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

#if !defined INCLUDED_VT_FETCH_MANAGER_H
#define INCLUDED_VT_FETCH_MANAGER_H

#include "vt/config.h"
#include "vt/fetch/fetch_id.h"
#include "vt/context/context.h"

#include <vector>
#include <unordered_map>

namespace vt { namespace fetch {

struct FetchManager {
  using ActionListType = std::vector<ActionType>;

  void whenReady(FetchType id, ActionType action) {
    when_ready_[id].push_back(action);
  }
  void whenFinishRead(FetchType id, ActionType action) {
    when_ready_read_[id].push_back(action);
  }
  void whenFree(FetchType id, ActionType action) {
    when_free_[id].push_back(action);
  }

  void notifyReady(FetchType id) {
    trigger(id, when_ready_);
  }

  void freeFetch(FetchType id) {
    // Assert that all ready triggers have been executed
    auto ready_iter = when_ready_.find(id);
    vtAssert(ready_iter == when_ready_.end(), "Ready triggers must be fired");
    auto read_iter = when_ready_read_.find(id);
    vtAssert(read_iter == when_ready_read_.end(), "Read triggers must be fired");
    // Trigger all the free actions
    trigger(id, when_free_);
  }

  void freeFetchRead(FetchType id) {
    // Trigger all the finished read actions
    trigger(id, when_ready_read_);
  }

  FetchType newID() {
    auto const this_node = theContext()->getNode();
    return FetchIDBuilder::make(cur_seq_id_++, this_node);
  }

private:
  void trigger(FetchType id, std::unordered_map<FetchType, ActionListType>& in) {
    auto iter = in.find(id);
    if (iter != in.end()) {
      for (auto&& elm : iter->second) {
        elm();
      }
      in.erase(iter);
    }
  }

private:
  std::unordered_map<FetchType, ActionListType> when_ready_      = {};
  std::unordered_map<FetchType, ActionListType> when_ready_read_ = {};
  std::unordered_map<FetchType, ActionListType> when_free_       = {};
  FetchSeqIDType cur_seq_id_                                     = 1;
};

}} /* end namespace vt::fetch */

#endif /*INCLUDED_VT_FETCH_MANAGER_H*/
