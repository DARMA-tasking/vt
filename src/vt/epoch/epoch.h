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
 *
 *      *where*    h = epoch_header_num_bits,
 *                 c = epoch_category_num_bits,
 *                 w = sizeof(EpochType) * 8
 *                 n = sizeof(NodeType)        ^             ^           ^
 *                                             | .... n .... | ..........|
 *                                               <NodeType>  <SeqEpochID>
 *
 *  +++++++++++++++++++++++++++++++++++++++++++  Rooted Extended Layout ++
 *
 *   <EpochHeader>   = <IsRooted> <HasCategory> <IsUser>
 *   ....3 bits...   = ..bit 1..   ...bit 2...  ..bit 3..
 *
 * =======================================================================
 * \endverbatim
 */

/**
 * \brief The header bit positions for an epoch (\c vt::EpochType)
 */
enum struct eEpochHeader : int8_t {
  RootedEpoch   = 1,
  CategoryEpoch = 2,
  UserEpoch     = 3
};

/// Number of bits for root flag
static constexpr BitCountType const epoch_root_num_bits = 1;
/// Number of bits for category flag
static constexpr BitCountType const epoch_hcat_num_bits = 1;
/// Number of bits for user flag
static constexpr BitCountType const epoch_user_num_bits = 1;

/**
 *  Important: if you add new types of epoch headers to the preceding enum, you
 *  must ensure that the number of epoch header bits is sufficient to hold all
 *  the header types.
 *
 */

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

/**
 * \brief The number of bits for all types of categories.
 *
 *  Important: if you add categories to the enum of epoch categories, you must
 *  ensure the \c epoch_category_num_bits is sufficiently large.
 *
 */
static constexpr BitCountType const epoch_category_num_bits = 2;

/// The total number of bits remaining the sequential part of the \c EpochType
static constexpr BitCountType const epoch_seq_num_bits = sizeof(EpochType) * 8 -
  (epoch_root_num_bits     +
   epoch_hcat_num_bits     + epoch_user_num_bits +
   epoch_category_num_bits + node_num_bits);

/**
 *  \brief Epoch layout enum to help with manipuating the bits
 *
 *  This describes the layout of the epoch used by \c EpochManip to get/set the
 *  bits on an \c EpochType field
 */
enum eEpochLayout {
  EpochSequential   = 0,
  EpochNode         = eEpochLayout::EpochSequential  + epoch_seq_num_bits,
  EpochCategory     = eEpochLayout::EpochNode        + node_num_bits,
  EpochUser         = eEpochLayout::EpochCategory    + epoch_category_num_bits,
  EpochHasCategory  = eEpochLayout::EpochUser        + epoch_user_num_bits,
  EpochIsRooted     = eEpochLayout::EpochHasCategory + epoch_hcat_num_bits,
  EpochSentinelEnd  = eEpochLayout::EpochIsRooted
};

/**
 *  The first basic epoch: BasicEpoch, NoCategoryEpoch:
 */

static constexpr EpochType const first_epoch = 1;

}} //end namespace vt::epoch

#endif /*INCLUDED_EPOCH_EPOCH_H*/
