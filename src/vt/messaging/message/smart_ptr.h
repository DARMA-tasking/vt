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
#include "vt/messaging/message/smart_ptr_virtual.h"

#include <iosfwd>

namespace vt { namespace messaging {

template <typename T>
struct MsgSharedPtr final {
  using MsgType        = T;
  using BaseMsgType    = BaseMessage;

  MsgSharedPtr(std::nullptr_t) : ptr_(nullptr), shared_(false) { }

  MsgSharedPtr(T* in, bool takeOwnership) {
    init(in, takeOwnership);
  }

  MsgSharedPtr(MsgSharedPtr<T> const& in) {
    init(in.get(), true);
  }

  MsgSharedPtr(MsgSharedPtr<T>&& in) {
    ptr_ = in.ptr_;
    shared_ = in.shared_;
    in.ptr_ = nullptr;
    in.shared_ = false;
  }

  ~MsgSharedPtr() {
    clear();
  }

  template <typename U>
  explicit operator MsgSharedPtr<U>() const { return to<U>(); }

  explicit operator T*() const { return get(); }

  /// Access as another (base) message type.
  /// Requires the type to be trivial destructible.
  template <typename U>
  MsgSharedPtr<U> to() const {
    static_assert(
      std::is_trivially_destructible<T>(),
      "Message shall not be downcast unless trivially destructible"
    );

    return MsgSharedPtr<U>(reinterpret_cast<U*>(ptr_), true);
  }

  /// Access as another (base) message type,
  /// even when such types are not trivially destructible.
  template <typename U>
  MsgVirtualPtr<U> toVirtual() const {
    // Type-erased shared_ptr capture to manage lifetime.
    // Can probably (see other notes) change this MsgSharedPtr to not
    // acquire a messageRef itself.
    return MsgVirtualPtr<U>(
      MsgSharedPtr<U>(reinterpret_cast<U*>(ptr_), true),
      std::make_shared<MsgSharedPtr<T>>(*this));
  }

  MsgSharedPtr<T>& operator=(std::nullptr_t) {
    clear();
    return *this;
  }

  MsgSharedPtr<T>& operator=(MsgSharedPtr<T> const& in) {
    clear();
    init(in.get(), true);
    return *this;
  }

  MsgSharedPtr<T>& operator=(MsgSharedPtr<T>&& in) {
    clear();
    ptr_ = in.ptr_;
    shared_ = in.shared_;
    in.ptr_ = nullptr;
    in.shared_ = false;
    return *this;
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

protected:

  // Assigns ptr_/shared_, optionally taking ownership.
  // Should probably be called every constructor.
  void init(T* msgPtr, bool takeOwnership) {
    if (msgPtr) {
      const bool shared = isSharedMessage<T>(msgPtr);
      if (shared) {
        auto msgEnv = msgPtr->env;
        vtAssertInfo(
          envelopeGetRef(msgEnv) > 0, "Bad Ref (before ref)",
          shared, envelopeGetRef(msgEnv)
        );
        debug_print(
          pool, node,
          "MsgSmartPtr: (auto) init(), ptr={}, envRef={}, takeOwnership={} address={}\n",
          print_ptr(msgPtr), envelopeGetRef(msgEnv), takeOwnership, print_ptr(this)
        );

        // TODO: verify not issue with no messageRef when shared_ == true and
        // takeOwnership == false; is state guarded externally?
        if (takeOwnership) {
          messageRef(msgPtr);
        }
      }

      ptr_ = msgPtr;
      shared_ = shared;
    } else {
      ptr_ = nullptr;
      shared_ = false;
    }
  }

  // NOTE: can call messageDeref even if messageRef NOT acquired
  // because shared_ can be set (to true) BEFORE declining to take a ref.
  void clear() {
    T* msgPtr = get();
    if (msgPtr) {
      const bool shared = shared_;
      if (shared) {
        auto msgEnv = msgPtr->env;
        vtAssertInfo(
          envelopeGetRef(msgEnv) > 0, "Bad Ref (before deref)",
          shared, envelopeGetRef(msgEnv)
        );
        debug_print(
          pool, node,
          "MsgSmartPtr: (auto) clear(), ptr={}, ref={}, address={}\n",
          print_ptr(msgPtr), envelopeGetRef(msgEnv), print_ptr(this)
        );

        messageDeref(msgPtr);
      }
    }

    ptr_ = nullptr;
    shared_ = false;
  }

private:
  BaseMsgType* ptr_ = nullptr;
  bool shared_      = false;
};

}} /* end namespace vt::messaging */

namespace vt {

template <typename T>
using MsgSharedPtr = messaging::MsgSharedPtr<T>;

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsgOwner(T* const msg) {
  msg->has_owner_ = true;
  return MsgSharedPtr<T>{msg,false};
}

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsg(MsgSharedPtr<T> msg) {
  vtAssert(msg->has_owner_, "promoteMsg shared ptr must have owner");
  return MsgSharedPtr<T>{msg.get(),true};
}

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsg(T* msg) {
  if (!msg->has_owner_) {
    return promoteMsgOwner(msg);
  } else {
    return MsgSharedPtr<T>{msg,true};
  }
}

} /* end namespace vt */


#endif /*INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H*/
