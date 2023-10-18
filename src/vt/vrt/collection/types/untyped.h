/*
//@HEADER
// *****************************************************************************
//
//                                  untyped.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_UNTYPED_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_UNTYPED_H

#include "vt/config.h"
#include "vt/vrt/base/base.h"
#include "vt/vrt/collection/types/has_migrate.h"

namespace vt { namespace vrt { namespace collection {

/*
 *      Base untyped collection for safe casting
 */

struct UntypedCollection : VrtBase, HasMigrate {
  UntypedCollection() = default;

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    VrtBase::serialize(s);
    s | released_epochs_;
  }

public:
  /**
   * \brief Add a released epoch to the set
   *
   * \param[in] epoch the epoch to add
   */
  void addReleasedEpoch(EpochType epoch) { released_epochs_.insert(epoch); }

  /**
   * \brief Remove a released epoch from the set
   *
   * \param[in] epoch the epoch to remove
   */
  void removeReleasedEpoch(EpochType epoch) {
    if (auto i = released_epochs_.find(epoch); i != released_epochs_.end()) {
      released_epochs_.erase(i);
    }
  }

  /**
   * \brief Check if an epoch has been released
   *
   * \param[in] epoch the epoch to check
   *
   * \return whether it is released
   */
  bool isReleasedEpoch(EpochType epoch) const {
    return released_epochs_.find(epoch) != released_epochs_.end();
  }

private:
  /// Released epochs for this collection element
  std::unordered_set<EpochType> released_epochs_;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_UNTYPED_H*/
