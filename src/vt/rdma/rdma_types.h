/*
//@HEADER
// ************************************************************************
//
//                          rdma_types.h
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

#if !defined INCLUDED_RDMA_RDMA_TYPES_H
#define INCLUDED_RDMA_RDMA_TYPES_H

#include "vt/config.h"

#include <sstream>
#include <iostream>

namespace vt { namespace rdma {

using UnderlyingNodeType = NodeType;

struct Endpoint {
  Endpoint(
    bool const& in_is_target, UnderlyingNodeType const& in_node
  ) : is_target(in_is_target), value(in_node)
  { }

  operator UnderlyingNodeType() const { return get(); }

  UnderlyingNodeType get() const { return value; }

  bool target() const { return is_target; }

private:
  bool is_target = false;
  UnderlyingNodeType value = uninitialized_destination;
};

struct Target : Endpoint {
  explicit Target(UnderlyingNodeType const& in_node)
    : Endpoint(true, in_node)
  { }
};

struct NonTarget : Endpoint {
  explicit NonTarget(UnderlyingNodeType const& in_node)
    : Endpoint(false, in_node)
  { }
};

}} //end namespace vt::rdma

namespace vt {

using RDMA_TargetType = rdma::Target;
using RDMA_NonTargetType = rdma::NonTarget;

struct from_s {
  RDMA_TargetType operator=(rdma::UnderlyingNodeType val) {
    return RDMA_TargetType(val);
  }
};

struct to_s {
  RDMA_NonTargetType operator=(rdma::UnderlyingNodeType val) {
    return RDMA_NonTargetType(val);
  }
};

extern from_s rdma_from;
extern to_s rdma_to;

} //end namespace vt

#endif /*INCLUDED_RDMA_RDMA_TYPES_H*/
