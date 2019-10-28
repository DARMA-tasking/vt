/*
//@HEADER
// *****************************************************************************
//
//                             smart_ptr_virtual.h
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

#if !defined INCLUDED_VT_MESSAGING_MESSAGE_SMART_PTR_VIRTUAL_H
#define INCLUDED_VT_MESSAGING_MESSAGE_SMART_PTR_VIRTUAL_H

#include "vt/config.h"
#include "vt/messaging/message/message.h"

namespace vt { namespace messaging {

template <typename T>
struct MsgSharedPtr;

struct PendingSend;

/// Wrap a MsgSharedPtr such that a base type can be accessed.
/// This is suitable for use with types with virtual destructors.
/// The original message is owned by the resulting object.
template <typename BaseMsgT>
struct MsgVirtualPtr final {

  template <typename MsgT>
  explicit MsgVirtualPtr(MsgSharedPtr<MsgT> in_ptr)
    : ptr_(MsgSharedPtr<BaseMsgT>(in_ptr)),
      closure_([in_ptr]{ })
  { }

  template <typename MsgT>
  explicit MsgVirtualPtr(MsgVirtualPtr<MsgT> in_vrt)
    : ptr_(MsgSharedPtr<BaseMsgT>(in_vrt.ptr_)),
      closure_(in_vrt.closure_)
  { }

  MsgVirtualPtr(std::nullptr_t) { }

  inline void operator=(std::nullptr_t)
  {
    ptr_ = nullptr;
    closure_ = nullptr;
  }

  inline BaseMsgT* get() const { return ptr_.get(); }
  inline BaseMsgT* operator->() const { return ptr_.get(); }
  inline BaseMsgT& operator*() const { return *ptr_.get(); }
  inline bool operator==(std::nullptr_t) const { return ptr_ == nullptr; }
  inline bool operator!=(std::nullptr_t) const { return ptr_ != nullptr; }

private:
  MsgSharedPtr<BaseMsgT> ptr_ = nullptr;
  std::function<void()> closure_ = nullptr;

  friend struct PendingSend;
  MsgSharedPtr<BaseMsgT>& getShared() { return ptr_; }
};

}} /* end namespace vt::messaging */

namespace vt {

template <typename MsgT>
using MsgVirtualPtr = messaging::MsgVirtualPtr<MsgT>;

using MsgVirtualPtrAny = messaging::MsgVirtualPtr<ShortMessage>;

} /* end namespace vt */

#endif /*INCLUDED_VT_MESSAGING_MESSAGE_SMART_PTR_VIRTUAL_H*/
