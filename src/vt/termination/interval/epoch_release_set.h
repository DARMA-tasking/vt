/*
//@HEADER
// ************************************************************************
//
//                          epoch_release_set.h
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

#if !defined INCLUDED_VT_TERMINATION_INTERVAL_EPOCH_RELEASE_SET_H
#define INCLUDED_VT_TERMINATION_INTERVAL_EPOCH_RELEASE_SET_H

#include "vt/config.h"
#include "vt/termination/interval/integral_set.h"
#include "vt/termination/interval/category_map.h"
#include "vt/termination/termination.h"
#include "vt/epoch/epoch.h"

#include <tuple>
#include <unordered_map>
#include <list>
#include <functional>

namespace vt { namespace term { namespace interval {

struct EpochReleaseSet {
  using ReleaseFnType = std::function<void(MsgVirtualPtrAny)>;

  EpochReleaseSet() = default;
  explicit EpochReleaseSet(ReleaseFnType in_fn) : release_fn_(in_fn) { }

  void setReleaseFn(ReleaseFnType in_fn) { release_fn_ = in_fn; }

  void release(EpochType const& epoch) {
    debug_print(
      gen, node,
      "release: epoch={:x}\n",
      epoch
    );

    // Insert into integral set
    map_.get(epoch).insert(epoch);

    // Trigger functions on the buffered messages with released epoch
    auto iter = wait_.find(epoch);
    if (iter != wait_.end()) {
      auto lst = std::move(iter->second);
      wait_.erase(iter);
      for (auto&& msg : lst) {
        debug_print(
          gen, node,
          "release: epoch={:x}, running msg={}, consume\n",
          epoch, print_ptr(msg.get())
        );

        release_fn_(msg);
        theTerm()->consume(epoch);
      }
    }

    // Trigger actions waiting for epoch release
    auto iter2 = action_.find(epoch);
    if (iter2 != action_.end()) {
      auto lst = std::move(iter2->second);
      action_.erase(iter2);
      for (auto&& fn : lst) { fn(); }
    }

    // Add an action for cleaning up the epoch after termination
    theTerm()->addAction(epoch, [this,epoch]{ notifyTerminated(epoch); });
  }

  bool isReleased(EpochType const& epoch) {
    bool const is_dep = epoch::EpochManip::isDep(epoch);

    debug_print(
      gen, node,
      "isReleased: epoch={:x}, is_dep={}\n",
      epoch, is_dep
    );

    if (is_dep) {
      bool const is_term = theTerm()->isTerm(epoch);

      debug_print(
        gen, node,
        "isReleased: epoch={:x}, is_dep={}, is_term={}\n",
        epoch, is_dep, is_term
      );

      if (is_term) {
        return true;
      } else {
        auto exists = map_.get(epoch).exists(epoch);

        debug_print(
          gen, node,
          "isReleased: epoch={:x}, is_dep={}, exists={}\n",
          epoch, is_dep, exists
        );

        return exists;
      }
    } else {
      return true;
    }
  }

  void notifyTerminated(EpochType const& epoch) {
    map_.get(epoch).erase(epoch);
    map_.cleanup(epoch);
  }

  void buffer(EpochType const& epoch, MsgVirtualPtrAny msg) {
    debug_print(
      gen, node,
      "buffer: epoch={:x}, buffering msg={}, produce, num={}\n",
      epoch, print_ptr(msg.get()), wait_[epoch].size()
    );

    theTerm()->produce(epoch);
    wait_[epoch].push_back(msg);
  }

  void whenReleased(EpochType const& epoch, ActionType action) {
    if (isReleased(epoch)) {
      action();
    } else {
      action_[epoch].push_back(action);
    }
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    if (not s.isUnpacking()) {
      vtAssertExpr(wait_.size() == 0);
      vtAssertExpr(action_.size() == 0);
    }
    s | map_;
    if (s.isUnpacking()) {
      release_fn_ = nullptr;
    }
  }

private:
  vt::EpochCategoryMap                                      map_        = {};
  std::unordered_map<EpochType,std::list<MsgVirtualPtrAny>> wait_       = {};
  std::unordered_map<EpochType,std::list<ActionType>>       action_     = {};
  ReleaseFnType                                             release_fn_ = {};
};

}}} /* end namespace vt::term::interval */

namespace vt {

using EpochReleaseSet = term::interval::EpochReleaseSet;

} /* end namespace vt */


#endif /*INCLUDED_VT_TERMINATION_INTERVAL_EPOCH_RELEASE_SET_H*/
