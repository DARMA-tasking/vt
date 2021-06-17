/*
//@HEADER
// *****************************************************************************
//
//                                  priority.h
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

#include <array>
#include <utility>

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

static constexpr PriorityType const priority_level_bits =
  vt_feature_cmake_priority_bits_level;
static constexpr PriorityType const priority_num_levels =
  priority_num_bits / priority_level_bits;
static constexpr PriorityType const priority_midpoint =
  (((1 << (priority_level_bits + 1)) - 1) >> 2);
static constexpr PriorityType const priority_remainder =
  priority_num_bits - priority_level_bits * priority_num_levels;
static constexpr PriorityType const priority_all_set =
  ((1 << (priority_level_bits + 1)) - 1) >> 1;

// 111 1000 111 0000000001000       1000 111

enum PriorityLevel {
  SystemLevel  = 0,
  UserLevelMin = PriorityLevel::SystemLevel + 1,
  UserLevelMax = priority_num_levels - 1
};

/*
 * Recursively, at compile-time, build the proper mask for the default priority
 * for depth-first expansion
 *
 */

template <
  PriorityType N, PriorityType M, PriorityType L, PriorityType R,
  bool F, typename = void
>
struct Mask {
  static constexpr PriorityType const offset = N - priority_level_bits;
  static constexpr PriorityType const M_p    = M | (priority_midpoint << (offset-1));
  static constexpr PriorityType const value  = Mask<offset, M_p, L-1, R, F>::value;
};

template <PriorityType N, PriorityType M, PriorityType R>
struct Mask<N, M, 0, R, true, typename std::enable_if<N != R>::type> {
  static constexpr PriorityType const offset = N - priority_level_bits;
  static constexpr PriorityType const M_p    = M | (((1 << (priority_level_bits + 1)) - 1) << (offset-1));
  static constexpr PriorityType const value  = Mask<offset, M_p, 0, R, true>::value;
};

template <PriorityType N, PriorityType M, PriorityType R>
struct Mask<N, M, 0, R, false, typename std::enable_if<N != R>::type> {
  static constexpr PriorityType const offset = N - priority_level_bits;
  static constexpr PriorityType const value  = Mask<offset, M, 0, R, false>::value;
};


template <PriorityType N, PriorityType M, PriorityType L, PriorityType R, bool fill>
struct Mask<N, M, L, R, fill, typename std::enable_if<N == R>::type> {
  static constexpr PriorityType const value = M << R;
};

template <
  PriorityType N = priority_num_bits,
  PriorityType M = 0x0,
  PriorityType L = priority_num_levels,
  PriorityType R = priority_remainder
>
struct DefaultMask : Mask<N, M, L, R, false> { };

template <
  PriorityType N,
  PriorityType M,
  PriorityType L,
  PriorityType R = priority_remainder
>
struct LevelMask : Mask<N, M, L, R, false> { };

template <
  PriorityType N,
  PriorityType M,
  PriorityType L,
  PriorityType R = priority_remainder
>
struct LevelFillMask : Mask<N, M, L, R, true> { };

using PriorityArrayType = std::array<PriorityType, sched::priority_num_levels>;

/*
 * Build the medium-level priority array for each level
 */

template <PriorityType... i>
constexpr PriorityArrayType mediumArray(std::integer_sequence<PriorityType, i...>) {
  return PriorityArrayType{{LevelMask<priority_num_bits, 0x0, i+1>::value...}};
}

template <PriorityType L = priority_num_levels>
constexpr PriorityArrayType mediumArray() {
  return mediumArray(std::make_integer_sequence<PriorityType, L>{});
}

/*
 * Build the breadth-first priority array for each level
 */

template <PriorityType... i>
constexpr PriorityArrayType breadthArray(std::integer_sequence<PriorityType, i...>) {
  return PriorityArrayType{{LevelFillMask<priority_num_bits, 0x0, i+1>::value...}};
}

template <PriorityType L = priority_num_levels>
constexpr PriorityArrayType breadthArray() {
  return breadthArray(std::make_integer_sequence<PriorityType, L>{});
}

/*
 * Some names for priorities that could possibly be used with a 1-,2-,3-bit
 * layout per level
 */

#if vt_feature_cmake_priority_bits_level == 1
  enum Priority {
    // Breadth-first execution is all bits set
    BreadthFirst = 1,//1

    // Depth-first execution priority hierarchy
    DepthFirst   = 0,//0
  };
#elif vt_feature_cmake_priority_bits_level == 2
  enum Priority {
    // Breadth-first execution is all bits set
    BreadthFirst = 3,//11

    // Depth-first execution priority hierarchy
    High         = 2,//10
    Medium       = 1,//01
    Low          = 0 //00
  };
#elif vt_feature_cmake_priority_bits_level == 3
  enum Priority {
    // Breadth-first execution is all bits set
    BreadthFirst = 7,//111

    // Depth-first execution priority hierarchy
    Highest      = 6,//110
    High         = 5,//101
    MediumHigh   = 4,//100
    Medium       = 3,//011
    MediumLow    = 2,//010
    Low          = 1,//001
    Lowest       = 0 //000
  };
#else
  // No enum, priority bits must be set without an alias from the system
#endif

}} /* end namespace vt::sched */

namespace vt {

static constexpr sched::PriorityArrayType breadth_priority = sched::breadthArray<>();
static constexpr sched::PriorityArrayType medium_priority  = sched::mediumArray<>();
static constexpr PriorityType const sys_max_priority       = breadth_priority[0];
static constexpr PriorityType const sys_min_priority       = 0;
static constexpr PriorityType const max_priority           = breadth_priority[1];
static constexpr PriorityType const min_priority           = medium_priority[0];
static constexpr PriorityType const default_priority       = sched::DefaultMask<>::value;

} /* end namespace vt */

#endif /*INCLUDED_VT_SCHEDULER_PRIORITY_H*/
