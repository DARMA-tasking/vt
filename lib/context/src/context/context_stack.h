/*
//@HEADER
// *****************************************************************************
//
//                               context_stack.h
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

#if !defined INCLUDED_CONTEXT_SRC_STACK_H
#define INCLUDED_CONTEXT_SRC_STACK_H

#include "context/context_wrapper.h"

#include <memory>
#include <cstdlib>

namespace fcontext {

static constexpr size_t const default_malloc_stack_size = 16384;

using FContextStackType = fcontext_stack_t;
using ULTContextType = ULTContext;

FContextStackType allocateMallocStackInner(size_t const size_in);
FContextStackType allocatePageSizedStackInner(size_t const size_in);

ULTContextType allocateMallocStack(size_t const size_in);
ULTContextType allocatePageSizedStack(size_t const size_in);

ULTContextType createStack(size_t size = 0, bool page_sized = false);

void destroyStackInner(fcontext_stack_t stack, bool is_page_alloced = false);
void destroyStack(ULTContextType stack);

} /* end namespace fcontext */

#endif /*INCLUDED_CONTEXT_SRC_STACK_H*/
