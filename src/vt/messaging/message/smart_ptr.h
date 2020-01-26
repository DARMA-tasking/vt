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
  struct MsgPtrImplTyped;
}} // end namespace vt::messaging

namespace vt { namespace messaging { namespace statics {
  /// Static objects with global lifetime and elimination of allocation.
  /// They are stateless and have a very high/constant reuse pattern.
  struct Holder {
    template <typename T>
    static MsgPtrImplTyped<T>* getImpl() {
      return &TypedMsgPtrImpls<T>;
    }

    template <typename T>
    static MsgPtrImplTyped<T> TypedMsgPtrImpls;
  };

  template <typename T>
  /*static*/ MsgPtrImplTyped<T> Holder::TypedMsgPtrImpls;
}}} // end namespace vt::messaging::statics


namespace vt { namespace messaging {

/// Message-type agnostic virtual base class.
struct MsgPtrImplBase {
  /// Invoke messageDeref on the appropriate type.
  /// (Ensure a valid delete-expression on virtual message types.)
  virtual void messageDeref(void* msg_ptr) = 0;
  virtual ~MsgPtrImplBase() {}
};

template <typename MsgT>
struct MsgPtrImplTyped : MsgPtrImplBase {
  virtual void messageDeref(void* msg_ptr) {
    // N.B. messageDeref<T> invokes delete-expr T.
    vt::messageDeref(static_cast<MsgT*>(msg_ptr));
  }
};


template <typename T>
struct MsgSharedPtr final {
  using MsgType        = T;
  using BaseMsgType    = BaseMessage;

  MsgSharedPtr(std::nullptr_t) {}

  MsgSharedPtr(T* in) {
    init(in, true, &statics::Holder::TypedMsgPtrImpls<T>);
  }

  MsgSharedPtr(T* in, bool takeRef) {
    init(in, takeRef, statics::Holder::getImpl<T>());
  }

  }

  // Overload to retain ORIGINAL type-erased implementation.
  MsgSharedPtr(T* in, bool takeRef, MsgPtrImplBase* impl) {
    init(in, takeRef, impl);
  }

  MsgSharedPtr(MsgSharedPtr<T> const& in) {
    init(in.get(), true, in.impl_);
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
    init(in.get(), true, in.impl_);
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
    return MsgSharedPtr<U>(
      reinterpret_cast<U*>(ptr_), true,
      /*N.B. retain ORIGINAL-type implementation*/ impl_);
  }

  /// [obsolete] Use to() as MsgVirtualPtr <-> MsgSharedPtr.
  /// Both methods are equivalent in funciton.
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
    auto nrefs = envelopeGetRef(m.get()->env);
    return os << "MsgSharedPtr("
              <<              m.ptr_    << ","
              << "ref="    << nrefs
              << ")";
  }

private:

  /// Performs state-ownership, optionally taking an additional message ref.
  /// Should probably be called every constructor; must ONLY be
  /// called from fresh (zero-init member) or clear() state.
  void init(T* msgPtr, bool takeRef, MsgPtrImplBase* impl) {
    assert("given message" && msgPtr);
    assert("not initialized" && not ptr_);
    assert("given impl" && impl);

    ptr_ = msgPtr;
    impl_ = impl;

    vtAssertInfo(
      envelopeGetRef(msgPtr->env) >= 0, "Bad Message Ref (before ref)",
      envelopeGetRef(msgPtr->env)
    );
    debug_print(
      pool, node,
      "MsgSmartPtr: (auto) init(), ptr={}, envRef={}, takeRef={} address={}\n",
      print_ptr(msgPtr), envelopeGetRef(msgPtr->env), takeRef, print_ptr(this)
    );

    if (takeRef) {
      // Could be moved to type-erased impl..
      messageRef(msgPtr);
    }
  }

  /// Clear all internal state. Effectively destructor in operation,
  /// with guarantee that init() can be used again after.
  void clear() {
    if (not ptr_) {
      return;
    }

    T* msgPtr = get();

    vtAssertInfo(
      envelopeGetRef(msgPtr->env) >= 1, "Bad Message Ref (before deref)",
      envelopeGetRef(msgPtr->env)
    );
    debug_print(
      pool, node,
      "MsgSmartPtr: (auto) clear(), ptr={}, envRef={}, address={}\n",
      print_ptr(msgPtr), envelopeGetRef(msgPtr->env), print_ptr(this)
    );

    impl_->messageDeref(msgPtr);

    ptr_ = nullptr;
  }

  /// Move. Must be invoked on fresh/clear state.
  void moveFrom(MsgSharedPtr<T>&& in) {
    ptr_ = in.ptr_;
    impl_ = in.impl_;
    // clean take - nullify/prevent other cleanup
    in.ptr_ = nullptr;
  }

private:
  // Underlying raw message - access as correct type via get()
  BaseMsgType* ptr_ = nullptr;
  // Type-erased implementation support.
  // Object has a STATIC LIFETIME / is not owned / should not be deleted.
  MsgPtrImplBase* impl_ = nullptr;
};

}} /* end namespace vt::messaging */


// Expose public/common types in vt:: namespace.
namespace vt {

/**
 * \internal
 * \obsolete Use \c MsgPtr<T>, for which is is an alias.
 */
template <typename T>
using MsgVirtualPtr = messaging::MsgSharedPtr<T>;

/**
 * \internal
 * \obsolete Use \c MsgPtr<ShortMessage>, or as appropriate.
 */
using MsgVirtualPtrAny = messaging::MsgSharedPtr<ShortMessage>;

/**
 * \internal
 * \obsolete Use \c MsgPtr<T>, for which is is an alias.
 */
template <typename T>
using MsgSharedPtr = messaging::MsgSharedPtr<T>;

/**
 * \brief Wrapper to manage Active Messages.
 *
 * A MsgPtr represents a 'shared pointer like' object wrapping a
 * message that correcly manages reference-counts to order to
 * eliminate memory leaks.
 */
template <typename T>
using MsgPtr = messaging::MsgSharedPtr<T>;

/// Obsolete form - do not use.
/// There is no direct replacement; has_owner_ is removed.
template <typename T>
[[deprecated("Do not use: no direct replacement")]]
inline MsgPtr<T> promoteMsgOwner(T* const msg) {
  return MsgPtr<T>{msg,false};
}

/// Obsolete form - do not use.
/// There is no direct replacement; has_owner_ is removed
/// and the semantic operation differed from promoteMsg(T*).
template <typename T>
[[deprecated("Do not use: no direct repalcement")]]
inline MsgPtr<T> promoteMsg(MsgPtr<T> msg) {
  return MsgPtr<T>{msg.get()};
}

/// Wrap a Msg* in a MsgPtr<Msg>, increasing ref-ownership.
/// This is the same as using MsgPtr<T>{T*} directly.
template <typename T>
inline MsgPtr<T> promoteMsg(T* msg) {
  return MsgPtr<T>{msg};
}

} /* end namespace vt */


#endif /*INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H*/
