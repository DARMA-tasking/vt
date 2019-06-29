/*
//@HEADER
// ************************************************************************
//
//                          category_map.h
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

#if !defined INCLUDED_VT_TERMINATION_INTERVAL_CATEGORY_MAP_H
#define INCLUDED_VT_TERMINATION_INTERVAL_CATEGORY_MAP_H

#include "vt/config.h"
#include "vt/epoch/epoch.h"

namespace vt { namespace term { namespace interval {

struct CategoryMap {

  CategoryMap() = default;

  EpochType categorize(EpochType const& epoch) const {
    auto cat_epoch = epoch;
    epoch::EpochManip::setSeq(cat_epoch,0);
    return cat_epoch;
  }

  vt::IntegralSet<EpochType>& get(EpochType const& epoch) {
    auto cat = categorize(epoch);
    auto iter = map_.find(cat);
    if (iter == map_.end()) {
      map_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(cat),
        std::forward_as_tuple(vt::IntegralSet<EpochType>(cat))
      );
      iter = map_.find(cat);
    }
    vtAssertExpr(iter != map_.end());
    return iter->second;
  }

  void cleanup(EpochType const& epoch) {
    auto cat = categorize(epoch);
    auto iter = map_.find(cat);
    if (iter != map_.end() and iter->second.empty()) {
      map_.erase(iter);
    }
  }

private:
  std::unordered_map<EpochType, vt::IntegralSet<EpochType>> map_ = {};
};

}}} /* end namespace vt::term::interval */

namespace vt {

using EpochCategoryMap = term::interval::CategoryMap;

} /* end namespace vt */

#endif /*INCLUDED_VT_TERMINATION_INTERVAL_CATEGORY_MAP_H*/
