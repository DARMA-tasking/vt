/*
//@HEADER
// ************************************************************************
//
//                          ctrl.h
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

#if !defined INCLUDED_VT_FETCH_CTRL_H
#define INCLUDED_VT_FETCH_CTRL_H

#include "vt/config.h"
#include "vt/fetch/manager.fwd.h"
#include "vt/fetch/manager.h"

namespace vt { namespace fetch {

static struct CtrlNewTagType  { } CtrlNewTag  { };
static struct CtrlReadTagType { } CtrlReadTag { };

struct Ctrl final {

  Ctrl() = delete;
  Ctrl(Ctrl const& i) = delete;
  Ctrl(std::nullptr_t) : refs_(nullptr), read_(nullptr)             { }
  Ctrl(CtrlNewTagType) : refs_(new int), read_(new int), id_(ID())  { reset(); }
  Ctrl(Ctrl&& i)       : refs_(i.refs_), read_(i.read_), id_(i.id_) { inc(); }
  Ctrl(CtrlReadTagType, Ctrl const& i)
    : refs_(i.refs_), read_(i.read_), id_(i.id_)
  { incRead(); }
  Ctrl(Ctrl const& i, bool read)
    : refs_(i.refs_), read_(i.read_), id_(i.id_), is_read_(read)
  { if (read) incRead(); else inc(); }

  Ctrl& operator=(std::nullptr_t) { clear(); return *this; }
  Ctrl& operator=(Ctrl const& in) {
    if (this != &in) {
      clear();
      refs_     = in.refs_;
      read_     = in.read_;
      id_       = in.id_;
      dep_      = in.dep_;
      is_read_  = in.is_read_;
      dep_read_ = in.dep_read_;
    }
    return *this;
  }

  bool operator==(Ctrl const& i) const { return id_ == i.id_; }
  bool operator!=(Ctrl const& i) const { return id_ != i.id_; }
  bool operator==(std::nullptr_t) const    { return refs_ == nullptr; }
  bool operator!=(std::nullptr_t) const    { return refs_ != nullptr; }

  ~Ctrl() { clear(); }

  void reset() { *refs_ = 1; *read_ = 0; }
  void clear() { if (is_read_) decRead(); else dec(); tryFree(); }
  void tryFree() {
    if (getRef() == 0) {
      vtAssertExpr(read_ != nullptr and *read_ == 0);
      free();
    }
  }
  void tryReadFree() {
    if (getRead() == 0) {
      readFree();
    }
  }
  void free() {
    clearDep();
    vtAssertExpr(refs_ != nullptr);
    vtAssertExpr(read_ != nullptr);
    delete refs_;
    delete read_;
    refs_ = nullptr;
    read_ = nullptr;
    theFetch()->freeFetch(id_);
  }
  void readFree() {
    vtAssertExpr(refs_ != nullptr);
    vtAssertExpr(read_ != nullptr);
    vtAssertExpr(*read_ == 0);
    theFetch()->freeFetchRead(id_);
  }

  void inc() { if (refs_) (*refs_)++; }
  void dec() { if (refs_) (*refs_)--; }
  int getRef() const { return refs_ ? *refs_ : -1; }

  void incRead() { if (read_) (*read_)++; inc(); }
  void decRead() { if (read_) (*read_)--; dec(); tryReadFree(); }
  int getRead() const { return read_ ? *read_ : -1; }

  bool valid() const { return id_ != no_fetch; }
  FetchType getID() const { return id_; }

  void dep(Ctrl* in, bool read) {
    if (dep_) { clearDep(); }
    dep_ = in;
    dep_read_ = read;
    vtAssertExpr(dep_ != nullptr);
    if (dep_read_) dep_->incRead(); else dep_->inc();
  }
  void clearDep() {
    if (dep_) {
      if (dep_read_) dep_->decRead(); else dep_->dec();
      dep_ = nullptr;
    }
  }

  static FetchType ID() { return theFetch()->newID(); }

private:
  int* refs_     = nullptr;
  int* read_     = nullptr;
  FetchType id_  = no_fetch;
  Ctrl* dep_     = nullptr;
  bool is_read_  = false;
  bool dep_read_ = false;
};

}} /* end namespace vt::fetch */

#endif /*INCLUDED_VT_FETCH_CTRL_H*/
