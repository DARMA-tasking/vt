
#if !defined INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H
#define INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H

#include "config.h"
#include "messaging/message/message.h"

namespace vt { namespace messaging {

static struct MsgInitOwnerType { } MsgInitOwnerTag { };

template <typename T>
struct MsgSharedPtr {
  using MsgType        = T;
  using MsgPtrType     = MsgType*;
  using BaseMsgType    = BaseMessage;
  using BaseMsgPtrType = BaseMsgType*;

  MsgSharedPtr(MsgInitOwnerType, T* in)
    : ptr_(in)
  {
    vtAssert(envelopeGetRef(in->env) > 0, "Must have ref greater than 0");
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
      vtAssertExpr(envelopeGetRef(n.get()->env) > 0);
      messageRef(n.get());
      ptr_ = n.get();
    }
  }
  inline void ref() {
    if (valid()) {
      vtAssertExpr(envelopeGetRef(get()->env) > 0);
      messageRef(get());
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
    setPtr(t);
  }

  inline void setPtr(T* t) {
    ptr_ = reinterpret_cast<BaseMsgPtrType>(t);
  }

  inline void clear() {
    if (valid()) {
      vtAssertExpr(envelopeGetRef(get()->env) > 0);
      messageDeref(get());
    }
    ptr_ = nullptr;
  }

private:
  BaseMsgPtrType ptr_ = nullptr;
};

}} /* end namespace vt::messaging */

namespace vt {

template <typename T>
using MsgSharedPtr = messaging::MsgSharedPtr<T>;

template <typename T>
inline messaging::MsgSharedPtr<T> promoteMsg(T* const msg) {
  return MsgSharedPtr<T>{messaging::MsgInitOwnerTag,msg};
}

// using MsgSharedAnyPtr = messaging::MsgSharedPtr<BaseMessage>;

} /* end namespace vt */


#endif /*INCLUDED_MESSAGING_MESSAGE_SMART_PTR_H*/
