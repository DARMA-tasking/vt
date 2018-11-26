/*
//@HEADER
// ************************************************************************
//
//                          scatter.impl.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H
#define INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H

#include "vt/config.h"
#include "vt/collective/scatter/scatter.h"
#include "vt/collective/scatter/scatter_msg.h"
#include "vt/context/context.h"

#include <cassert>
#include <cstring>

namespace vt { namespace collective { namespace scatter {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void Scatter::scatter(
  std::size_t const& total_size, std::size_t const& max_proc_size,
  FuncSizeType size_fn, FuncDataType data_fn
) {
  auto const& num_nodes = theContext()->getNumNodes();
  auto const& elm_size = max_proc_size;
  auto const& combined_size = num_nodes * elm_size;
  auto scatter_msg = makeSharedMessageSz<ScatterMsg>(
    combined_size, combined_size, elm_size
  );
  vtAssert(total_size == combined_size, "Sizes must be consistent");
  auto ptr = reinterpret_cast<char*>(scatter_msg) + sizeof(ScatterMsg);
  auto remaining_size = thePool()->remainingSize(
    reinterpret_cast<void*>(scatter_msg)
  );
  vtAssertInfo(
    remaining_size >= combined_size,
    "Remaining size must be sufficient",
    total_size, combined_size, remaining_size, elm_size
  );
  debug_print(
    scatter, node,
    "Scatter::scatter: total_size={}, elm_size={}, ScatterMsg={}, msg-ptr={}, "
    "ptr={}, remaining_size={}\n",
    total_size, elm_size, sizeof(ScatterMsg), print_ptr(scatter_msg),
    print_ptr(ptr), remaining_size
  );
  auto const& root_node = 0;
  auto nptr = applyScatterRecur(root_node, ptr, elm_size, size_fn, data_fn);
  debug_print(
    scatter, node,
    "Scatter::scatter: incremented size={}\n",
    nptr-ptr
  );
  vtAssert(nptr == ptr + combined_size, "nptr must match size");
  auto const& handler = auto_registry::makeAutoHandler<MessageT,f>(nullptr);
  auto const& this_node = theContext()->getNode();
  scatter_msg->user_han = handler;
  if (this_node != root_node) {
    theMsg()->sendMsgSz<ScatterMsg,scatterHandler>(
      root_node, scatter_msg, sizeof(ScatterMsg) + combined_size
    );
  } else {
    scatterIn(scatter_msg);
  }
}

}}} /* end namespace vt::collective::scatter */

#endif /*INCLUDED_COLLECTIVE_SCATTER_SCATTER_IMPL_H*/
