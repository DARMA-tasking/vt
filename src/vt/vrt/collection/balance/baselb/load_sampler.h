/*
//@HEADER
// *****************************************************************************
//
//                                load_sampler.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_LOAD_SAMPLER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_LOAD_SAMPLER_H

#include "vt/vrt/collection/balance/baselb/baselb.h"

#include <list>
#include <map>

namespace vt { namespace vrt { namespace collection { namespace lb {

static constexpr int32_t const default_bin_size = 10;

struct LoadSamplerBaseLB : BaseLB {
  using ObjBinType       = int32_t;
  using ObjBinListType   = std::list<ObjIDType>;
  using ObjSampleType    = std::map<ObjBinType, ObjBinListType>;

  explicit LoadSamplerBaseLB(
    int32_t in_bin_size = default_bin_size
  ) : bin_size_(in_bin_size)
  { }

public:
  void buildHistogram();
  ObjBinType histogramSample(LoadType const& load) const;
  int32_t getBinSize() const { return bin_size_; }

protected:
  int32_t bin_size_                               = 10;
  ObjSampleType obj_sample                        = {};
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_BASELB_LOAD_SAMPLER_H*/
