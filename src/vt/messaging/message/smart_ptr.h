/*
//@HEADER
// *****************************************************************************
//
//                                 smart_ptr.h
//                           DARMA Toolkit v. 1.0.0
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

#if !defined INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H
#define INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H

#include "vt/config.h"
#include "vt/messaging/message/message.h"
#include "vt/messaging/message/refs.h"

#include <iosfwd>
#include <cassert>

namespace vt { namespace messaging {
  // Fwd decl for statics.
  template <typename T>
  struct MsgDerefTyped;
}} // end namespace vt::messaging

namespace vt { namespace messaging { namespace statics {
  /// Static message derefs for global lifetime and elimination
  /// of allocation as they are stateless objects with expected
  /// high reuse through the program.
  template <typename T>
  static MsgDerefTyped<T> StaticMsgDerefs;
}}} // end namespace vt::messaging::statics


namespace vt { namespace messaging {

struct MsgDerefBase {
  // Invoke messageDeref on the appropriate type.
  virtual void messageDeref(void* msg_ptr) = 0;
  virtual ~MsgDerefBase() {}
};

template <typename MsgT>
struct MsgDerefTyped : MsgDerefBase {
  virtual void messageDeref(void* msg_ptr) {
    vt::messageDeref(static_cast<MsgT*>(msg_ptr));
  }
};


template <typename T>
struct MsgSharedPtr final {
  using MsgType        = T;
  using BaseMsgType    = BaseMessage;

  MsgSharedPtr(std::nullptr_t) {}

  MsgSharedPtr(T* in, bool takeRef) {
    init(in, takeRef, nullptr);
  }

  MsgSharedPtr(T* in, bool takeRef, MsgDerefBase* deref) {
    init(in, takeRef, deref);
  }

  MsgSharedPtr(MsgSharedPtr<T> const& in) {
    init(in.get(), true, in.deref_);
  }

  MsgSharedPtr(MsgSharedPtr<T>&& in) {
    moveFrom(std::move(in));
  }

  MsgSharedPtr<T>& operator=(std::nullptr_t) {
    clear();
    return *this;
  }

  MsgSharedPtr<T>& operator=(MsgSharedPtr<T> const& in) {
    clear();
    init(in.get(), true, in.deref_);
    return *this;
  }

  MsgSharedPtr<T>& operator=(MsgSharedPtr<T>&& in) {
    clear();
    moveFrom(std::move(in));
    return *this;
  }

  ~MsgSharedPtr() {
    clear();
  }

  template <typename U>
  explicit operator MsgSharedPtr<U>() const { return to<U>(); }

  explicit operator T*() const { return get(); }

  /// Access as another (usually base) message type.
  template <typename U>
  MsgSharedPtr<U> to() const {
    // Establish type-erased deref if needed.
    // Non-shared message are not deref'ed and trivially destructible types
    // do not really care on which type the delete-expr is invoked.
    if (std::is_trivially_destructible<T>() or not shared_) {
      assert("should not have deref" && not deref_);
      return MsgSharedPtr<U>(
        reinterpret_cast<U*>(ptr_), true);
    }

    return MsgSharedPtr<U>(
      reinterpret_cast<U*>(ptr_), true,
      deref_ ? deref_ : &statics::StaticMsgDerefs<T>);
  }

  // Obsolete. Use to() as MsgVirtualPtr <-> MsgSharedPtr.
  template <typename U>
  MsgSharedPtr<U> toVirtual() const {
    return to<U>();
  }

  bool operator==(MsgSharedPtr<T> const& n) const { return ptr_ == n.ptr_; }
  bool operator!=(MsgSharedPtr<T> const& n) const { return ptr_ != n.ptr_; }
  bool operator==(std::nullptr_t) const           { return ptr_ == nullptr; }
  bool operator!=(std::nullptr_t) const           { return ptr_ != nullptr; }

  inline T* operator->() const { return get(); }
  inline T& operator*() const { return *ptr_; }
  inline T* get() const {
    return ptr_ ? reinterpret_cast<T*>(ptr_) : nullptr;
  }

  friend std::ostream& operator<<(std::ostream&os, MsgSharedPtr<T> const& m) {
    auto nrefs = m.ptr_ && m.shared_ ? envelopeGetRef(m.get()->env) : -1;
    return os << "MsgSharedPtr("
              <<              m.ptr_    << ","
              << "shared=" << m.shared_ << ","
              << "ref="    << nrefs
              << ")";
  }

private:

  // Performs state-ownership, optionally taking an additional message ref.
  // Should probably be called every constructor; must ONLY be
  // called from fresh/clear state.
  void init(T* msgPtr, bool takeRef, MsgDerefBase* deref) {
    assert("given message" && msgPtr);
    assert("not initialized" && not ptr_);

    ptr_ = msgPtr;
    deref_ = deref;
    bool shared = (shared_ = isSharedMessage<T>(msgPtr));

    assert("deref -> shared" && (not deref_ or (deref_ and shared)));

    if (shared) {
      vtAssertInfo(
        envelopeGetRef(msgPtr->env) > 0, "Bad Ref (before ref)",
        shared, envelopeGetRef(msgPtr->env)
      );
      debug_print(
        pool, node,
        "MsgSmartPtr: (auto) init(), ptr={}, envRef={}, takeRef={} address={}\n",
        print_ptr(msgPtr), envelopeGetRef(msgPtr->env), takeRef, print_ptr(this)
      );

      if (takeRef) {
        messageRef(msgPtr);
      }
    }
  }

  void clear() {
    bool shared = shared_;

    assert("deref -> shared" && (not deref_ or (deref_ and shared)));

    if (shared) {
      assert("shared -> message ptr" && ptr_);
      T* msgPtr = get();

      vtAssertInfo(
        envelopeGetRef(msgPtr->env) > 0, "Bad Ref (before deref)",
        shared, envelopeGetRef(msgPtr->env)
      );
      debug_print(
        pool, node,
        "MsgSmartPtr: (auto) clear(), ptr={}, envRef={}, address={}\n",
        print_ptr(msgPtr), envelopeGetRef(msgPtr->env), print_ptr(this)
      );

      MsgDerefBase* deref = deref_;
      if (deref) {
        deref->messageDeref(msgPtr);
      } else {
        messageDeref<T>(msgPtr);
      }

      shared_ = false;
      deref_ = nullptr;
    }

    ptr_ = nullptr;
  }

  /// Move. Must be invoked on fresh/clear state.
  void moveFrom(MsgSharedPtr<T>&& in) {
    ptr_ = in.ptr_;
    shared_ = in.shared_;
    deref_ = in.deref_;
    // clean take - nullify other cleanup
    in.ptr_ = nullptr;
    in.shared_ = false;
    in.deref_ = nullptr;
  }

private:
  // Underlying raw message - access as correct type via get()
  BaseMsgType* ptr_ = nullptr;
  // Is this a shared message?
  // If so, then it will have a deref done on delete.
  bool shared_      = false;
  // Type-erased to invoke messageDeref on correct type for delete-expr.
  // If set, use. Otherwise invoke messageDeref<T> directly.
  // Object has a STATIC LIFETIME and should not be deleted.
  MsgDerefBase* deref_ = nullptr;
};

}} /* end namespace vt::messaging */

namespace vt {

// For historic reasons;
// Functionality is now part of MsgSharedPtr
template <typename T>
using MsgVirtualPtr = messaging::MsgSharedPtr<T>;

// For historic reasons;
// Functionality is now part of MsgSharedPtr
using MsgVirtualPtrAny = messaging::MsgSharedPtr<ShortMessage>;

template <typename T>
using MsgSharedPtr = messaging::MsgSharedPtr<T>;

/// Steal ownership of the message (no ref-increase).
template <typename T>
inline MsgSharedPtr<T> promoteMsgOwner(T* const msg) {
  msg->has_owner_ = true;
  return MsgSharedPtr<T>{msg,false};
}

/// Take additional ownership of the message (increase message ref).
/// The message must already have an owner.
// TODO: eliminate if possible as it has confusing semantics
// with overload and is duplicated by copy/move ctors.
template <typename T>
inline MsgSharedPtr<T> promoteMsg(MsgSharedPtr<T> msg) {
  vtAssert(msg->has_owner_, "promoteMsg shared ptr must have owner");
  return MsgSharedPtr<T>{msg.get(),true};
}

/// If the message does not have an owner, steal ownership (no ref-increase).
/// Otherwise, take additional ownership (increate message ref).
template <typename T>
inline MsgSharedPtr<T> promoteMsg(T* msg) {
  if (!msg->has_owner_) {
    return promoteMsgOwner(msg);
  } else {
    return MsgSharedPtr<T>{msg,true};
  }
}

} /* end namespace vt */


#endif /*INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H*/
