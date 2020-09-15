/*
//@HEADER
// *****************************************************************************
//
//                            epoch_parameterized.h
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

#if !defined INCLUDED_VT_EPOCH_EPOCH_PARAMETERIZED_H
#define INCLUDED_VT_EPOCH_EPOCH_PARAMETERIZED_H

#include "vt/epoch/epoch.h"

namespace vt { namespace epoch {

struct EpochCollectiveScope;

/**
 * \struct RootedEpoch
 *
 * \brief A parameterized, rooted epoch that is collectively created for
 * dependency management which can be concretized for any specific node.
 */
struct RootedEpoch {

private:
  /**
   * \internal \brief Create a new parameterized, rooted epoch---called by the
   * system in scoped context.
   *
   * \param[in] in_epoch the epoch that does not contain a node yet
   */
  explicit RootedEpoch(EpochType in_epoch)
    : nodeless_rooted_epoch_(in_epoch)
  { }

  friend struct EpochCollectiveScope;

public:
  /**
   * \brief Get the rooted epoch for a particular node
   *
   * \param[in] node the node
   *
   * \return the rooted epoch for that node
   */
  EpochType get(NodeType node) const;

private:
  EpochType nodeless_rooted_epoch_ = no_epoch; /**< The epoch without a node */
};

}} /* end namespace vt::epoch */

#endif /*INCLUDED_VT_EPOCH_EPOCH_PARAMETERIZED_H*/
