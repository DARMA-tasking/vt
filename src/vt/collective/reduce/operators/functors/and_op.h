/*
//@HEADER
// *****************************************************************************
//
//                                   and_op.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_AND_OP_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_AND_OP_H

#include "vt/config.h"

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename T>
struct AndOp {
  void operator()(T& v1, T const& v2) {
    v1 = v1 && v2;
  }
};

template <typename T>
struct AndOp< std::vector<T> > {
  void operator()(std::vector<T>& v1, std::vector<T> const& v2) {
    vtAssert(v1.size() == v2.size(), "Sizes of vectors in reduce must be equal");
    for (size_t ii = 0; ii < v1.size(); ++ii)
      v1[ii] = v1[ii] && v2[ii];
  }
};

template <typename T, std::size_t N>
struct AndOp< std::array<T, N> > {
  void operator()(std::array<T, N>& v1, std::array<T, N> const& v2) {
    for (size_t ii = 0; ii < N; ++ii)
    v1[ii] = v1[ii] && v2[ii];
  }
};

}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using AndOp = reduce::operators::AndOp<T>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_FUNCTORS_AND_OP_H*/
