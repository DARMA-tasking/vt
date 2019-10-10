/*
//@HEADER
// *****************************************************************************
//
//                                  priority.h
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

#if !defined INCLUDED_VT_SCHEDULER_PRIORITY_H
#define INCLUDED_VT_SCHEDULER_PRIORITY_H

#include "vt/config.h"

namespace vt { namespace sched {

/*
 * The priority field range: [0..2 << priority_num_bits)
 *
 * It's split into levels to partition across recursive invocations of
 * components.
 *
 *   System level: 0
 *   User levels: [1 ... priority_num_levels - 1)
 *
 * Thus, if level bits is 3 and priority field width is 16, num levels is 5
 * (must be rounded down).
 *
 *           Example PriorityType bit field layout:
 *
 *     [ 0,1,2, 3,4,5, 6,7,8, 9,10,11, 12,13,14, (unused bits)... ]
 *       <===>  <=============================>
 *       System    User Bits: 4 user levels
 *       Bits      w/ 3 bits each
 *
 */

static constexpr PriorityType const priority_level_bits = 3;
static constexpr PriorityType const priority_num_levels =
  priority_num_bits / priority_level_bits ;

enum PriorityLevel {
  SystemLevel  = 0,
  UserLevelMin = PriorityLevel::SystemLevel + 1,
  UserLevelMax = priority_num_levels - 1;
};

/*
 * Some names for priorities that could possibly be used with a 3-bit layout per
 * level
 */

enum Priority {
  Highest    = 0,
  High       = 1,
  MediumHigh = 2,
  Medium     = 3,
  MediumLow  = 4,
  Low        = 5,
  Lower      = 6,
  Lowest     = 7
};

}} /* end namespace vt::sched */

#endif /*INCLUDED_VT_SCHEDULER_PRIORITY_H*/
