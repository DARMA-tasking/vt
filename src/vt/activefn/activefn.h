/*
//@HEADER
// *****************************************************************************
//
//                                  activefn.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_ACTIVEFN_ACTIVEFN_H
#define INCLUDED_VT_ACTIVEFN_ACTIVEFN_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/configs/types/types_rdma.h"

namespace vt {

/*******************************************************************************
 *
 *                          Active Function Naming Scheme
 *
 *******************************************************************************
 *
 *    - All registerable active function handlers start with "Active" and end
 *      with "Type", since they are a type alias.
 *
 *    - If the active function is a pointer, it should end with `FnPtr' rather
 *      than just `Fn'
 *
 *      -- Non-function pointer
 *
 *            using ActiveFnType = void(BaseMessage*);
 *
 *      -- Function pointer
 *
 *            using ActiveFnPtrType = void(*)(BaseMessage*);
 *
 *     - If the active function is typed, it should have `Typed' before the
 *       Fn/FnPtr:
 *
 *      -- Typed function pointer
 *
 *            template <typename MessageT>
 *            using ActiveTypedFnPtrType = void(*)(MessageT*);
 *
 *    - If the active function is a closure (can have state associated with
 *      it beyond just a pointer), it should be post-fixed with `Closure'
 *
 *            using ActiveClosureFnType = std::function<void(BaseMessage*)>;
 *
 *******************************************************************************
 */

using ActiveClosureFnType = std::function<void(vt::BaseMessage*)>;
using ActiveFnType = void(vt::BaseMessage *);
using ActiveFnPtrType = void(*)(vt::BaseMessage *);

template <typename MessageT>
using ActiveTypedFnType = void(MessageT *);

using ActiveClosureRDMAGetFnType = std::function<
  RDMA_GetType(vt::BaseMessage*, ByteType, ByteType, TagType, bool)
>;
using ActiveRDMAGetFnPtrType = RDMA_GetType(*)(
  vt::BaseMessage *, ByteType, ByteType, TagType, bool
);
template <typename MessageT>
using ActiveTypedRDMAGetFnType = RDMA_GetType(
  MessageT*, ByteType, ByteType, TagType, bool
);

using ActiveClosureRDMAPutFnType = std::function<
  void(vt::BaseMessage*, RDMA_PtrType, ByteType, ByteType, TagType, bool)
>;
using ActiveRDMAPutFnPtrType = void(*)(
  vt::BaseMessage *, RDMA_PtrType, ByteType, ByteType, TagType, bool
);
template <typename MessageT>
using ActiveTypedRDMAPutFnType = void(
  MessageT*, RDMA_PtrType, ByteType, ByteType, TagType, bool
);



}  // end namespace vt

#endif  /*INCLUDED_VT_ACTIVEFN_ACTIVEFN_H*/
