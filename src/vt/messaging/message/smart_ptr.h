/*
//@HEADER
// *****************************************************************************
//
//                                 smart_ptr.h
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

#if !defined INCLUDED_VT_MESSAGING_MESSAGE_SMART_PTR_H
#define INCLUDED_VT_MESSAGING_MESSAGE_SMART_PTR_H

#include "vt/config.h"
#include "vt/messaging/message/message.h"
#include "vt/messaging/message/refs.h"
#include "vt/serialization/sizer.h"

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

  MsgSharedPtr(T* in, ByteType size=no_byte) {
    init(in, size == no_byte ? sizeof(T) : size, statics::Holder::getImpl<T>());
  }

  // Overload to retain ORIGINAL type-erased implementation.
  MsgSharedPtr(T* in, ByteType size, MsgPtrImplBase* impl) {
    init(in, size, impl);
  }

  MsgSharedPtr(MsgSharedPtr<T> const& in, ByteType size=no_byte) {
    if (in != nullptr) {
      init(in.get(), (size == no_byte ? in.size() : size), in.impl_);
    }
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
    init(in.get(), in.size(), in.impl_);
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
      reinterpret_cast<U*>(ptr_),
      size_, // retain ORIGINAL size
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

  /**
   * \brief Implicit access to MsgT*
   *
   * Asserts ownership: invalid to use after a std::move or implicit thief.
   *
   * \return the owned message, never null
   */
  inline T* operator->() const { return get(); }

  /**
   * \brief Implicit access to MsgT&
   *
   * Asserts ownership: invalid to use after a std::move or implicit thief.
   *
   * \return ref to the owned message
   */
  inline T& operator*() const { return *ptr_; }

  /**
   * \brief Explicit access to MsgT*
   *
   * Asserts ownership: invalid to use after a std::move or implicit thief.
   *
   * \return the owned message, never null
   */
  inline T* get() const {
    vtAssert(
      ptr_, "Access attempted of invalid MsgPtr."
    );
    return reinterpret_cast<T*>(ptr_);
  }

  /**
   * \brief Explicit access to message size
   *
   * \return the owned message, never null
   */
  inline ByteType size() const {
    vtAssert(
      ptr_, "Access attempted of invalid MsgPtr."
    );
    return size_;
  }

  /**
   * \internal
   * \brief Checks to see if a message is owned.
   *
   * \return \c true if a message is owned or \c false after std::move.
   */
  bool ownsMessage() const {
    return ptr_ not_eq nullptr;
  }

  friend std::ostream& operator<<(std::ostream&os, MsgSharedPtr<T> const& m) {
    auto nrefs = envelopeGetRef(m.get()->env);
    return os << "MsgSharedPtr("
              <<              m.ptr_    << ","
              << "size=" << m.size_ << ","
              << "ref="    << nrefs
              << ")";
  }

  template <
    typename SerializerT,
    typename = std::enable_if_t<
      std::is_same<SerializerT, checkpoint::Footprinter>::value
    >
  >
  void serialize(SerializerT& s) {
    if (ownsMessage()) {
      auto ptr = get();
      auto const msg_size = vt::serialization::MsgSizer<MsgType>::get(ptr);
      s.addBytes(msg_size);

      // skip footprinting of members, we rely on message size estimate instead
      s.skip(impl_);
      s.skip(size_);
      s.skip(ptr_);
    }
  }

private:

  /// Performs state-ownership, always taking an additional message ref.
  /// Should probably be called every constructor; must ONLY be
  /// called from fresh (zero-init member) or clear() state.
  void init(T* msgPtr, ByteType size, MsgPtrImplBase* impl) {
    vtAssert(
      msgPtr,
      "MsgPtr cannot wrap 'null' messages."
    );
    assert("not initialized" && not ptr_);
    assert("given impl" && impl);

    ptr_ = msgPtr;
    size_ = size;
    impl_ = impl;

    // Could be moved to type-erased impl..
    messageRef(msgPtr);
  }

  /// Clear all internal state. Effectively destructor in operation,
  /// with guarantee that init() can be used again after.
  void clear() {
    if (not ptr_) {
      return;
    }

    T* msgPtr = get();

    impl_->messageDeref(msgPtr);

    ptr_ = nullptr;
  }

  /// Move. Must be invoked on fresh/clear state.
  void moveFrom(MsgSharedPtr<T>&& in) {
    ptr_ = in.ptr_;
    size_ = in.size_;
    impl_ = in.impl_;
    // clean take - nullify/prevent other cleanup
    in.ptr_ = nullptr;
  }

private:
  // Underlying raw message - access as correct type via get()
  BaseMsgType* ptr_ = nullptr;
  // Message size preserved before type erasure
  ByteType size_ = no_byte;
  // Type-erased implementation support.
  // Object has a STATIC LIFETIME / is not owned / should not be deleted.
  MsgPtrImplBase* impl_ = nullptr;
};

/**
 * \brief Helper to unify 'stealing' message ownership.
 *
 * This type is not intented to be used directly. It uses implicit conversion
 * construtors to perform a 'std::move' on a \c MsgPtr<MsgT>&.
 *
 * If a normal \c MsgT* is supplied, it is promoted without stealing.
 */
template <typename MsgT>
struct MsgPtrThief {
  /**
   * \brief Promote a \c MsgT*
   * \deprecated Use \c MsgPtr<MsgT>& to pass ownership of messages.
   *
   * The message is promoted without any implicit stealing.
   * It is generally assumed the caller did not 'own' this message anyway.
   */
  // If this method is removed then this type will no longer able to work in
  // a transitory nature, which is very handy when converting some API.
  // [[deprecated]]
  /*implicit*/ MsgPtrThief(MsgT const* msgPtr) : msg_(MsgSharedPtr<MsgT>{const_cast<MsgT*>(msgPtr)}) {
  }

  /**
   * \brief Moves (steals) the incoming MsgPtr.
   *
   * As though \c std::move(msg) was used, the \c msg is invalidated.
   */
  /*implicit*/ MsgPtrThief(MsgSharedPtr<MsgT>& msg) : msg_(std::move(msg)) {
  }

  /**
   * \brief Moves (steals) the incoming MsgPtr.
   *
   * This form allows code to explicitly use \c std::move as desired.
   */
  /*implicit*/ MsgPtrThief(MsgSharedPtr<MsgT>&& msg) : msg_(std::move(msg)) {
  }

  MsgSharedPtr<MsgT> msg_;
};

}} /* end namespace vt::messaging */


// Expose public/common types in vt:: namespace.
namespace vt {

/**
 * \internal
 * \obsolete Use \c MsgPtr<T>, for which this is an alias.
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
 * \obsolete Use \c MsgPtr<T>, for which this is an alias.
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
  return MsgPtr<T>{msg};
}

/// Obsolete form - do not use.
/// There is no direct replacement; has_owner_ is removed
/// and the semantic operation differed from promoteMsg(T*).
template <typename T>
[[deprecated("Do not use: no direct replacement")]]
inline MsgPtr<T> promoteMsg(MsgPtr<T> msg) {
  return MsgPtr<T>{msg.get(), msg.size()};
}

/**
 * \brief Wrap a message as a MsgPtr<Msg>, increasing ref-ownership.
 *
 * This is the same as using MsgPtr<T>{T*} directly.
 * The primary usage is in historic call-sites as new code should prefer
 * using \c makeMessage (and accepting a MsgPtr) instead of creating
 * a raw message first.
 */
template <typename T>
inline MsgPtr<T> promoteMsg(T* msg) {
  return MsgPtr<T>{msg};
}

template <typename T>
using MsgPtr = messaging::MsgSharedPtr<T>;

} /* end namespace vt */


#endif /*INCLUDED_VT_MESSAGING_MESSAGE_SMART_PTR_H*/
