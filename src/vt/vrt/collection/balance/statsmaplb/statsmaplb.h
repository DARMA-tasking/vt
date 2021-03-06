/*
//@HEADER
// ************************************************************************
//
//                          StatsMapLB.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATSMAPLB_STATSMAPLB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATSMAPLB_STATSMAPLB_H

#include "vt/objgroup/proxy/proxy_objgroup.h"

namespace vt { namespace vrt { namespace collection { namespace lb {

struct BaseLB;

struct StatsMapLB : BaseLB {
  StatsMapLB() = default;
  StatsMapLB(StatsMapLB const &) = delete;
  StatsMapLB(StatsMapLB &&) noexcept = default;
  StatsMapLB &operator=(StatsMapLB const &) = delete;
  StatsMapLB &operator=(StatsMapLB &&) noexcept = default;
  virtual ~StatsMapLB() = default;

  void init(objgroup::proxy::Proxy<StatsMapLB> in_proxy);
  void runLB() override;
  void inputParams(balance::SpecEntry* spec) override { }

private:
  objgroup::proxy::Proxy<StatsMapLB> proxy_ = {};
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATSMAPLB_STATSMAPLB_H*/
