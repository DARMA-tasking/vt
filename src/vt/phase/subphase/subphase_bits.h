/*
//@HEADER
// *****************************************************************************
//
//                               subphase_bits.h
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

#if !defined INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_BITS_H
#define INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_BITS_H

#include "vt/configs/types/types_type.h"

namespace vt { namespace phase { namespace subphase {

/**
 * \enum eSubphaseLayout
 *
 * \brief Layout of the subphase bits to create unique identifiers from strings
 * for both collective and rooted subphases
 */
enum eSubphaseLayout {
  SubphaseSeq       = 0,        /**< Sequence identifier */
  SubphaseNode      = 31,       /**< Only exists for rooted epochs */
  SubphaseIsRooted  = 63        /**< Whether this is rooted or collective */
};

/// Length of pure ID in subphase for rooted subphases
static constexpr SubphaseType const seq_len_rooted = 31;

/// Length of pure ID in subphase for collective subphases
static constexpr SubphaseType const seq_len_collective = 63;

/**
 * \struct SubphaseBits
 *
 * \brief Generates consistent IDs for subphases
 */
struct SubphaseBits {

  /**
   * \internal
   * \brief Generate a new ID from inputs
   *
   * \param[in] is_collective whether the subphase is collective
   * \param[in] node the node (ignored if collective)
   * \param[in] raw_id the raw ID bits to mix in
   *
   * \return the full unique ID
   */
  static SubphaseType makeID(
    bool is_collective, vt::NodeType node, SubphaseType raw_id
  );

};

}}} /* end namespace vt::phase::subphase */

#endif /*INCLUDED_VT_PHASE_SUBPHASE_SUBPHASE_BITS_H*/
