/*
//@HEADER
// *****************************************************************************
//
//                                epoch_manip.h
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

#if !defined INCLUDED_EPOCH_EPOCH_MANIP_H
#define INCLUDED_EPOCH_EPOCH_MANIP_H

#include "vt/config.h"
#include "vt/epoch/epoch.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/utils/bits/bits_packer.h"

namespace vt { namespace epoch {

static constexpr NodeType const default_epoch_node = uninitialized_destination;
static constexpr eEpochCategory const default_epoch_category =
  eEpochCategory::NoCategoryEpoch;

struct EpochManip {
  /*
   *  Epoch getters to check type and state of EpochType
   */
  static bool           isRooted   (EpochType const& epoch);
  static bool           hasCategory(EpochType const& epoch);
  static bool           isUser     (EpochType const& epoch);
  static eEpochCategory category   (EpochType const& epoch);
  static NodeType       node       (EpochType const& epoch);
  static EpochType      seq        (EpochType const& epoch);

  /*
   *  Epoch setters to manipulate the type and state of EpochType
   */
  static void setIsRooted   (EpochType& epoch, bool           const is_rooted);
  static void setHasCategory(EpochType& epoch, bool           const has_cat  );
  static void setIsUser     (EpochType& epoch, bool           const is_user  );
  static void setCategory   (EpochType& epoch, eEpochCategory const cat      );
  static void setNode       (EpochType& epoch, NodeType       const node     );
  static void setSeq        (EpochType& epoch, EpochType      const seq      );

  /*
   * General (stateless) methods for creating a epoch with certain properties
   * based on a current sequence number
   */
  static EpochType makeRootedEpoch(
    EpochType      const& seq,
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );
  static EpochType makeEpoch(
    EpochType      const& seq,
    bool           const& is_rooted  = false,
    NodeType       const& root_node  = default_epoch_node,
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );

  /*
   * Stateful methods for creating a epoch based on epochs that have already
   * been created in the past
   */
  static EpochType makeNewRootedEpoch(
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );
  static EpochType makeNewEpoch(
    bool           const& is_rooted  = false,
    NodeType       const& root_node  = default_epoch_node,
    bool           const& is_user    = false,
    eEpochCategory const& category   = default_epoch_category
  );
  static EpochType next(EpochType const& epoch);

private:
  static EpochType nextSlow(EpochType const& epoch);
  static EpochType nextFast(EpochType const& epoch);

private:
  static EpochType cur_rooted_;
  static EpochType cur_non_rooted_;
};

}} /* end namespace vt::epoch */

#include "vt/epoch/epoch_manip_get.h"
#include "vt/epoch/epoch_manip_set.h"
#include "vt/epoch/epoch_manip_make.h"

#endif /*INCLUDED_EPOCH_EPOCH_MANIP_H*/
