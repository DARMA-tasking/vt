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

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct MsgInitOwnerType { } MsgInitOwnerTag { };
static struct MsgInitNonOwnerType { } MsgInitNonOwnerTag { };
#pragma GCC diagnostic pop

template <typename T>
struct MsgSharedPtr final {
  using MsgType        = T;
  using BaseMsgType    = BaseMessage;

  MsgSharedPtr(MsgInitOwnerType, T* in) {
    init(in, false);
  }

  MsgSharedPtr(MsgInitNonOwnerType, T* in) {
    init(in, true);
  }

  MsgSharedPtr(std::nullptr_t) : ptr_(nullptr), shared_(false) { }

  MsgSharedPtr(MsgSharedPtr<T> const& in) {
    init(in.get(), true);
  }

  MsgSharedPtr(MsgSharedPtr<T>&& in) {
    // Not really a move.. should take ownership and set/clear ptr directly.
    init(in.get(), true);
  }

  template <typename U>
  explicit MsgSharedPtr(MsgSharedPtr<U> const& in) {
    // Only used by VirtualMsgPtr; little dance.. U* -> base -> T*
    BaseMsgType* basePtr{in.get()};
    T* msgPtr{reinterpret_cast<T*>(basePtr)};
    init(msgPtr, true);
  }

  ~MsgSharedPtr() {
    clear();
  }

  template <typename U>
  explicit operator MsgSharedPtr<U>() const { return to<U>(); }

  explicit operator T*() const { return get(); }

  template <typename U>
  MsgSharedPtr<U> to() const {
    static_assert(
      std::is_trivially_destructible<T>(),
      "Message shall not be downcast unless trivially destructible"
    );

    return MsgSharedPtr<U>{*this};
  }

  template <typename U>
  MsgVirtualPtr<U> toVirtual() const {
    return MsgVirtualPtr<U>(*this);
  }

  MsgSharedPtr<T>& operator=(std::nullptr_t) { clear(); return *this; }
  MsgSharedPtr<T>& operator=(MsgSharedPtr<T> const& n) { set(n); return *this; }
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
    auto nrefs = m.valid() && m.shared_ ? envelopeGetRef(m.get()->env) : -1;
    return os << "MsgSharedPtr("
              <<              m.ptr_    << ","
              << "shared=" << m.shared_ << ","
              << "ref="    << nrefs
              << ")";
  }

protected:

  inline bool valid() const { return ptr_ != nullptr; }

  // Assigns ptr_/shared_, optionally taking ownership.
  // Should probably be called every constructor.
  inline void init(T* in_ptr, bool takeOwnership) {
    if (in_ptr) {
      bool shared = isSharedMessage<T>(in_ptr);
      if (shared) {
        auto msgEnv = in_ptr->env;
        vtAssertInfo(
          envelopeGetRef(msgEnv) > 0, "Bad Ref (before ref); envelop ref already acquired",
          shared, envelopeGetRef(msgEnv)
        );
        debug_print(
          pool, node,
          "MsgSmartPtr: (auto) init(), ptr={}, envRef={}, takeOwnership={} address={}\n",
          print_ptr(in_ptr), envelopeGetRef(msgEnv), takeOwnership, print_ptr(this)
        );

        if (takeOwnership) {
          messageRef(in_ptr);
        }
      }

      ptr_ = in_ptr;
      shared_ = shared;
    } else {
      ptr_ = nullptr;
      shared_ = false;
    }
  }

  inline void set(MsgSharedPtr<T> const& n) {
    clear();
    T* in_ptr = n.get();
    init(in_ptr, true);
  }

  // NOTE: unlike other set, does not increment ref.
  inline void set(T* t) {
    clear();
    debug_print(
      pool, node,
      "MsgSmartPtr: (auto) set() BARE, ptr={}, ref={}, address={}\n",
      print_ptr(t), envelopeGetRef(t->env), print_ptr(this)
    );

    ptr_ = reinterpret_cast<BaseMsgType*>(t);
  }

  // NOTE: can call messageDeref even if message ref NOT acquired.
  inline void clear() {
    if (valid()) {
      if (shared_) {
        vtAssertInfo(
          envelopeGetRef(get()->env) > 0, "Bad Ref (before deref)",
          shared_, envelopeGetRef(get()->env)
        );
        debug_print(
          pool, node,
          "MsgSmartPtr: (auto) clear(), ptr={}, ref={}, address={}\n",
          print_ptr(get()), envelopeGetRef(get()->env), print_ptr(this)
        );
        messageDeref(get());
      }
    }
    ptr_ = nullptr;
  }

private:
  BaseMsgType* ptr_ = nullptr;
  bool shared_      = true;
};

}} /* end namespace vt::messaging */

namespace vt {

template <typename T>
using MsgSharedPtr = messaging::MsgSharedPtr<T>;

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsgOwner(T* const msg) {
  msg->has_owner_ = true;
  return MsgSharedPtr<T>{messaging::MsgInitOwnerTag,msg};
}

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsg(MsgSharedPtr<T> msg) {
  vtAssert(msg->has_owner_, "promoteMsg shared ptr must have owner");
  return MsgSharedPtr<T>{messaging::MsgInitNonOwnerTag,msg.get()};
}

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsg(T* msg) {
  if (!msg->has_owner_) {
    return promoteMsgOwner(msg);
  } else {
    return MsgSharedPtr<T>{messaging::MsgInitNonOwnerTag,msg};
  }
}

} /* end namespace vt */


#endif /*INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H*/
