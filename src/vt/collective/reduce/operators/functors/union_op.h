/*
//@HEADER
// *****************************************************************************
//
//                                  union_op.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_UNION_OP_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_UNION_OP_H

#include "vt/config.h"

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename T>
struct UnionOp;

template <typename T>
struct UnionOp<std::set<T>> {
  void operator()(std::set<T>& v1, std::set<T> const& v2) {
    v1.insert(v2.begin(), v2.end());
  }
};

template <typename T, typename U>
struct UnionOp<std::unordered_map<T, std::set<U>>> {
  void operator()(
    std::unordered_map<T, std::set<U>>& v1,
    std::unordered_map<T, std::set<U>> const& v2
  ) {
    for (auto&& elm : v2) {
      v1[elm.first].insert(elm.second.begin(), elm.second.end());
    }
  }
};

template <typename T, typename U>
struct UnionOp<std::map<T, std::set<U>>> {
  void operator()(
    std::map<T, std::set<U>>& v1,
    std::map<T, std::set<U>> const& v2
  ) {
    for (auto&& elm : v2) {
      v1[elm.first].insert(elm.second.begin(), elm.second.end());
    }
  }
};

}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using UnionOp = reduce::operators::UnionOp<T>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_UNION_OP_H*/
