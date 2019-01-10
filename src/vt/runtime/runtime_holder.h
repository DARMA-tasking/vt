/*
//@HEADER
// ************************************************************************
//
//                          runtime_holder.h
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

#if !defined INCLUDED_RUNTIME_RUNTIME_HOLDER_H
#define INCLUDED_RUNTIME_RUNTIME_HOLDER_H

#include "vt/config.h"
#include "vt/runtime/runtime_common.h"
#include "vt/runtime/runtime.h"

#include <cassert>

namespace vt { namespace runtime {

/*
 *  Holder class for `vt::runtime::Runtime' that essentially acts as a
 *  std::unique_ptr<Runtime>, but specialized for the Runtime struct. It enables
 *  user management of Runtime instances with a value-semantic container for
 *  safety.
 */
struct RuntimeHolder {
  using PointerType = vt::runtime::Runtime*;
  using ReferenceType = vt::runtime::Runtime&;

  RuntimeHolder(PointerType in_rt, bool in_owner = false)
    : rt_(in_rt), owner_(in_owner)
  { }
  RuntimeHolder(std::nullptr_t) : rt_(nullptr) { }
  RuntimeHolder(RuntimeHolder&&) = default;
  RuntimeHolder(RuntimeHolder const&) = delete;

  virtual ~RuntimeHolder() { release(); }

  RuntimeHolder& operator=(std::nullptr_t) { release(); return *this; }
  bool operator==(RuntimeHolder const& other) const { return rt_ == other.rt_; }
  bool operator!=(RuntimeHolder const& other) const { return rt_ != other.rt_; }
  bool operator==(std::nullptr_t) const { return rt_ == nullptr; }
  bool operator!=(std::nullptr_t) const { return rt_ != nullptr; }
  operator bool() const { return rt_ != nullptr; }

  RuntimeHolder& operator=(RuntimeHolder&& holder) {
    release();
    if (holder.rt_) {
      owner_ = holder.owner_;
      rt_ = holder.rt_;
      holder.owner_ = false;
      holder.release();
    } else {
      rt_ = nullptr;
    }
    return *this;
  }

  inline PointerType unsafe() { return rt_; }
  inline PointerType operator->() const { return rt_; }
  void makeNonOwning() { owner_ = false; }

private:
  inline PointerType get() const { return rt_; }
  inline ReferenceType operator*() { return getRef(); }
  inline ReferenceType getRef() const {
    vtAssert(rt_, "RuntimeHolder: holder is null: can not be dereferenced!");
    return *rt_;
  }

  void release() {
    if (owner_ && rt_ && rt_->isFinializeble() && rt_->hasSchedRun()) {
      rt_->finalize();
    }
    rt_ = nullptr;
  }

private:
  PointerType rt_ = nullptr;
  bool owner_ = false;
};

inline RuntimeHolder makeRuntimePtr(RuntimeHolder::PointerType const ptr) {
  return RuntimeHolder{ptr, true};
}

}} /* end namespace vt::runtime */

#endif /*INCLUDED_RUNTIME_RUNTIME_HOLDER_H*/
