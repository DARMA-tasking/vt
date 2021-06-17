/*
//@HEADER
// *****************************************************************************
//
//                                rdma_region.h
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

#if !defined INCLUDED_RDMA_RDMA_REGION_H
#define INCLUDED_RDMA_RDMA_REGION_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"

#include <sstream>

namespace vt { namespace rdma {

using RegionElmSizeType = int32_t;
static constexpr RegionElmSizeType const no_elm_size = -1;

struct Region {
  RDMA_ElmType lo;
  RDMA_ElmType hi;
  RDMA_ElmType sd = 1;

  RegionElmSizeType elm_size = no_elm_size;

  Region(Region const&) = default;

  Region(
    RDMA_ElmType const& in_lo, RDMA_ElmType const& in_hi,
    RDMA_ElmType const& in_sd = 1,
    RegionElmSizeType const& in_elm_size = no_elm_size
  ) : lo(in_lo), hi(in_hi), sd(in_sd), elm_size(in_elm_size)
  {
    vtAssertExpr(sd == 1);
  }

  bool hasElmSize() const {
    return elm_size != no_elm_size;
  }

  void setElmSize(RegionElmSizeType const& size) {
    fmt::print("setting region size to {}\n", size);

    elm_size = size;
  }

  RDMA_ElmType getSize() const {
    return (hi-lo)/sd;
  }

  std::string regionToBuf() const {
    std::stringstream buf;
    buf << "[" << lo << ":" << hi << ":" << sd << "]" << std::endl;
    // char const* const print_format = "[{}:{}:{}]";
    // auto const& len = std::strlen(print_format);
    // std::string str(len);
    // sfmt::print(str.c_str(), print_format, lo, hi, sd);
    return buf.str();
  }
};

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_REGION_H*/
