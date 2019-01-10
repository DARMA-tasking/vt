/*
//@HEADER
// ************************************************************************
//
//                          pool_header.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
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

#include "vt/config.h"
#include "vt/pool/header/pool_header.h"
#include "vt/context/context.h"

namespace vt { namespace pool {

/*static*/ char* HeaderManager::setHeader(
  size_t const& num_bytes, size_t const& oversize, char* buffer
) {
  AllocView view;
  view.buffer = buffer;
  view.layout->prealloc.alloc_size = num_bytes;
  view.layout->prealloc.oversize = oversize;
  view.layout->prealloc.alloc_worker = theContext()->getWorker();
  auto buf_start = buffer + sizeof(Header);
  return buf_start;
}

/*static*/ size_t HeaderManager::getHeaderBytes(char* buffer) {
  AllocView view;
  view.buffer = buffer - sizeof(Header);
  return view.layout->prealloc.alloc_size;
}

/*static*/ size_t HeaderManager::getHeaderOversizeBytes(char* buffer) {
  AllocView view;
  view.buffer = buffer - sizeof(Header);
  return view.layout->prealloc.oversize;
}

/*static*/ WorkerIDType HeaderManager::getHeaderWorker(char* buffer) {
  AllocView view;
  view.buffer = buffer - sizeof(Header);
  return view.layout->prealloc.alloc_worker;
}

/*static*/ char* HeaderManager::getHeaderPtr(char* buffer) {
  return buffer - sizeof(Header);
}

}} /* end namespace vt::pool */
