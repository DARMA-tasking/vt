
#if !defined INCLUDED_RUNTIME_RUNTIME_HOLDER_H
#define INCLUDED_RUNTIME_RUNTIME_HOLDER_H

#include "config.h"
#include "runtime/runtime_common.h"
#include "runtime/runtime.h"

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

  inline PointerType unsafe() { return rt_; }
  inline PointerType operator->() const { return rt_; }
  void makeNonOwning() { owner_ = false; }

private:
  inline PointerType get() const { return rt_; }
  inline ReferenceType operator*() { return getRef(); }
  inline ReferenceType getRef() const {
    if (rt_) {
      return *rt_;
    } else {
      assert(0 && "RuntimeHolder: holder is null: can not be dereferenced!");
    }
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
