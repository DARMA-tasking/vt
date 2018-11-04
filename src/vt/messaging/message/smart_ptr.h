
#if !defined INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H
#define INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H

#include "config.h"
#include "messaging/message/message.h"
#include "messaging/message/refs.h"

namespace vt { namespace messaging {

static struct MsgInitOwnerType { } MsgInitOwnerTag { };
static struct MsgInitNonOwnerType { } MsgInitNonOwnerTag { };

template <typename T>
struct MsgSharedPtr {
  using MsgType        = T;
  using MsgPtrType     = MsgType*;
  using BaseMsgType    = BaseMessage;
  using BaseMsgPtrType = BaseMsgType*;

  MsgSharedPtr(MsgInitOwnerType, T* in)
    : ptr_(in)
  {
    if (in) {
      shared_ = isSharedMessage<T>(in);
      if (shared_) {
        vtAssertInfo(
          envelopeGetRef(in->env) > 0, "owner: ref must be greater than 0",
          shared_, envelopeGetRef(in->env)
        );
      }
    }
  }
  MsgSharedPtr(MsgInitNonOwnerType, T* in)
    : ptr_(in)
  {
    if (in) {
      shared_ = isSharedMessage<T>(in);
      if (shared_) {
        vtAssertInfo(
          envelopeGetRef(in->env) > 0, "non-owner: ref must be greater than 0",
          shared_, envelopeGetRef(in->env)
        );
        messageRef(in);
      }
    }
  }
  MsgSharedPtr(std::nullptr_t) : ptr_(nullptr) { }
  MsgSharedPtr(MsgSharedPtr<T> const& in) : ptr_(in.ptr_) { ref(); }
  MsgSharedPtr(MsgSharedPtr<T>&& in)      : ptr_(in.ptr_) { ref(); }

  template <typename U>
  explicit MsgSharedPtr(MsgSharedPtr<U> const& in) : ptr_(in.get()) { ref(); }

  virtual ~MsgSharedPtr() {
    clear();
  }

  template <typename U>
  explicit operator MsgSharedPtr<U>() const { return to<U>(); }
  explicit operator MsgPtrType() const { return get(); }

  template <typename U>
  MsgSharedPtr<U> to() const { return MsgSharedPtr<U>{*this}; }

  MsgSharedPtr<T>& operator=(std::nullptr_t) { clear(); return *this; }
  MsgSharedPtr<T>& operator=(MsgSharedPtr<T> const& n) { set(n); return *this; }
  bool operator==(MsgSharedPtr<T> const& n) const { return ptr_ == n.ptr_; }
  bool operator!=(MsgSharedPtr<T> const& n) const { return ptr_ != n.ptr_; }
  bool operator==(std::nullptr_t) const           { return ptr_ == nullptr; }
  bool operator!=(std::nullptr_t) const           { return ptr_ != nullptr; }

  inline bool valid() const { return ptr_ != nullptr; }
  inline void set(MsgSharedPtr<T> const& n) {
    clear();
    if (n.valid()) {
      shared_ = isSharedMessage<T>(n.get());
      if (shared_) {
        vtAssertInfo(
          envelopeGetRef(n.get()->env) > 0, "Bad Ref (before ref set)",
          shared_, envelopeGetRef(n.get()->env)
        );
        debug_print(
          pool, node,
          "MsgSmartPtr: (auto) set(), ptr={}, ref={}, address={}\n",
          print_ptr(n.get()), envelopeGetRef(n.get()->env), print_ptr(this)
        );
        messageRef(n.get());
      }
      ptr_ = n.get();
    }
  }
  inline void ref() {
    if (valid()) {
      shared_ = isSharedMessage<T>(get());
      if (shared_) {
        vtAssertInfo(
          envelopeGetRef(get()->env) > 0, "Bad Ref (before ref)",
          shared_, envelopeGetRef(get()->env)
        );
        debug_print(
          pool, node,
          "MsgSmartPtr: (auto) ref(), ptr={}, ref={}, address={}\n",
          print_ptr(get()), envelopeGetRef(get()->env), print_ptr(this)
        );
        messageRef(get());
      }
    }
  }

  inline T* operator->() const { return get(); }
  inline T& operator*() const { return *ptr_; }
  inline T* get() const {
    return ptr_ ? reinterpret_cast<MsgPtrType>(ptr_) : nullptr;
  }


protected:
  inline void set(T* t) {
    clear();
    debug_print(
      pool, node,
      "MsgSmartPtr: (auto) set() BARE, ptr={}, ref={}, address={}\n",
      print_ptr(t), envelopeGetRef(t->env), print_ptr(this)
    );
    setPtr(t);
  }

  inline void setPtr(T* t) {
    ptr_ = reinterpret_cast<BaseMsgPtrType>(t);
  }

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
  BaseMsgPtrType ptr_ = nullptr;
  bool shared_        = true;
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
  return MsgSharedPtr<T>{messaging::MsgInitNonOwnerTag,msg};
}

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsg(T* msg) {
  if (!msg->has_owner_) {
    return promoteMsgOwner(msg);
  } else {
    return MsgSharedPtr<T>{messaging::MsgInitNonOwnerTag,msg};
  }
}

// using MsgSharedAnyPtr = messaging::MsgSharedPtr<BaseMessage>;

} /* end namespace vt */


#endif /*INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H*/
