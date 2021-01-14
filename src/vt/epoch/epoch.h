/*
//@HEADER
// *****************************************************************************
//
//                                   epoch.h
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

#if !defined INCLUDED_EPOCH_EPOCH_H
#define INCLUDED_EPOCH_EPOCH_H

#include "vt/config.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/configs/debug/debug_fmt.h"

namespace vt { namespace epoch {

/** \file */

/**
 * This is a description of how \c EpochType is actually laid out in memory.
 *
 * \verbatim
 * ========================= Layout of the Epoch =========================
 *
 *   w-1 .............. w-h-1 ...............w-h-c-1 ....................0
 *   | <EpochHeader> ... | <EpochCategory> ... | <Sequential Epoch ID>   |
 *                                             |                          \
 *      *where*    h = epoch_header_num_bits,  |                           \
 *                 c = epoch_category_num_bits,|                            \
 *                 w = sizeof(EpochType) * 8   |                             \
 *                 n = sizeof(NodeType)        ^  16  ^ 5 ^   [remainder]     ^
 *                                            /                               |
 *                                           /                                |
 *                                   _______                                  |
 *                                  /                                          \
 *                                  | .... n .... | ... scope ... | ...........|
 *                                    <NodeType>   <EpochScopeType> <SeqEpochID>
 *
 *  +++++++++++++++++++++++++++++++++++++++++++  Rooted Extended Layout ++
 *
 *   <EpochHeader>   = <IsRooted> <HasCategory>
 *   ....3 bits...   = ..bit 1..   ...bit 2...
 *
 * =======================================================================
 * \endverbatim
 */

/**
 * \brief The header bit positions for an epoch (\c vt::EpochType)
 */
enum struct eEpochHeader : int8_t {
  RootedEpoch   = 1
};

/// Number of bits for root flag
static constexpr BitCountType const epoch_root_num_bits = 1;

/**
 *  Important: if you add new types of epoch headers to the preceding enum, you
 *  must ensure that the number of epoch header bits is sufficient to hold all
 *  the header types.
 *
 */

/**
 * \brief The number of bits for all types of categories.
 *
 *  Important: if you add categories to the enum of epoch categories, you must
 *  ensure the \c epoch_category_num_bits is sufficiently large.
 *
 */
static constexpr BitCountType const epoch_category_num_bits = 2;

/**
 * \brief These are different categories of epochs that are allowed.
 *
 * These category bits statically identify an epoch as a certain type which can
 * be used to dispatch control logic.
 */
enum struct eEpochCategory : int8_t {
  NoCategoryEpoch       = 0x0,
  InsertEpoch           = 0x1,
  DijkstraScholtenEpoch = 0x2
};

/// Operator<< for printing the epoch category \c eEpochCategory enum
inline std::ostream& operator<<(std::ostream& os, eEpochCategory const& cat) {
  return debug::printEnum<eEpochCategory>(os,cat);
}

/// Holds a epoch scope ID (collectively generated)
using EpochScopeType = uint64_t;

/// The default, global epoch scope
static constexpr EpochScopeType const global_epoch_scope = 0;

// Number of bits allocated for an epoch scope
static constexpr BitCountType const scope_bits = 20;

/// The limit on number of live scopes at a given time;
/// Scope 0 is the default scope so that is excluded as a valid scope
static constexpr EpochScopeType const scope_limit = (1<<scope_bits) - 1;

/// The scope sentinel
static constexpr EpochScopeType const no_scope = ~0ull;

/// The number of sequential ID bits remaining for a collective \c EpochType
static constexpr BitCountType const epoch_seq_coll_num_bits =
  sizeof(EpochType) * 8 -
  (epoch_root_num_bits + epoch_category_num_bits + scope_bits);

/// The total number of bits remaining for a rooted \c EpochType
static constexpr BitCountType const epoch_seq_root_num_bits =
  sizeof(EpochType) * 8 -
  (epoch_root_num_bits + epoch_category_num_bits + scope_bits + node_num_bits);

/**
 *  \brief Epoch layout enum for collective epochs to help with manipulating the
 *  bits
 *
 *  This describes the layout of the epoch used by \c EpochManip to get/set the
 *  bits on an \c EpochType field
 */
enum eEpochColl {
  cEpochSequential = 0,
  cEpochScope      = eEpochColl::cEpochSequential + epoch_seq_coll_num_bits,
  cEpochCategory   = eEpochColl::cEpochScope      + scope_bits,
  cEpochIsRooted   = eEpochColl::cEpochCategory   + epoch_category_num_bits
};

/**
 *  \brief Epoch layout enum for rooted epochs to help with manipulating the
 *  bits
 *
 *  This describes the layout of the epoch used by \c EpochManip to get/set the
 *  bits on an \c EpochType field
 */
enum eEpochRoot {
  rEpochSequential = 0,
  rEpochNode       = eEpochRoot::rEpochSequential + epoch_seq_root_num_bits,
  rEpochScope      = eEpochRoot::rEpochNode       + node_num_bits,
  rEpochCategory   = eEpochRoot::rEpochScope      + scope_bits,
  rEpochIsRooted   = eEpochRoot::rEpochCategory   + epoch_category_num_bits
};

/// The first epoch sequence number
static constexpr EpochType const first_epoch = 1;

/// The default epoch node used for non-rooted epochs
static constexpr NodeType const default_epoch_node = uninitialized_destination;

/// The default epoch category
static constexpr eEpochCategory const default_epoch_category =
  eEpochCategory::NoCategoryEpoch;

}} //end namespace vt::epoch

namespace vt {

/// Type for epoch scope bits embedded in an \c EpochType
using EpochScopeType = epoch::EpochScopeType;

} /* end namespace vt */

#endif /*INCLUDED_EPOCH_EPOCH_H*/
