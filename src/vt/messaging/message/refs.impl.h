/*
//@HEADER
// *****************************************************************************
//
//                                 refs.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_MESSAGING_MESSAGE_REFS_IMPL_H
#define INCLUDED_VT_MESSAGING_MESSAGE_REFS_IMPL_H

#include "vt/config.h"
#include "vt/messaging/envelope.h"
#include "vt/context/context.h"

namespace vt {

template <typename MsgT>
void messageRef(MsgT* msg) {
  envelopeRef(msg->env);

  vt_debug_print(
    verbose, pool,
    "messageRef msg={}, refs={}\n",
    print_ptr(msg), envelopeGetRef(msg->env)
  );
}

template <typename MsgT>
void messageDeref(MsgT* msg) {
  RefType refcount = envelopeDeref(msg->env);

  vt_debug_print(
    verbose, pool,
    "messageDeref msg={}, refs={}\n",
    print_ptr(msg), refcount
  );

  if (refcount == 0) {
    /* @todo: what is the correct strategy here? invoking dealloc does not
     * invoke the destructor
     *
     * Instead of explicit/direct dealloc to pool:
     *   thePool()->dealloc(msg);
     *
     * Call `delete' to trigger the destructor and execute overloaded
     * functionality! This should trigger the pool dealloc invocation if it is
     * being used.
     *
     */
    delete msg;
  }
}

} /* end namespace vt */

#endif /*INCLUDED_VT_MESSAGING_MESSAGE_REFS_IMPL_H*/
