/*
//@HEADER
// *****************************************************************************
//
//                              diagnostic_units.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_UNITS_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_UNITS_H

namespace vt { namespace runtime { namespace component {

/** \brief Unit type for a diagnostic value */
enum struct DiagnosticUnit : int8_t {
  Bytes,                        /**< Memory units as bytes */
  Units,                        /**< Units of some entity */
  UnitsPerSecond,               /**< Units per second */
  Seconds                       /**< Time units as seconds */
};

/** \brief Multipliers for basic units */
enum struct UnitMultiplier : int8_t {
  Base = 0,               /**< Multiplier = 1 */
  Thousands = 1,          /**< Multiplier = 1,000 */
  Millions = 2,           /**< Multiplier = 1,000,000 */
  Billions = 3,           /**< Multiplier = 1,000,000,000 */
  Trillions = 4,          /**< Multiplier = 1,000,000,000,000 */
  Quadrillions = 5,       /**< Multiplier = 1,000,000,000,000,000 */
  Quintillions = 6,       /**< Multiplier = 1,000,000,000,000,000,000 */
};

/** \brief Time multipliers for DiagnosticUnit::Seconds */
enum struct TimeMultiplier : int8_t {
  Seconds = 0,             /**< Multiplier = 1 */
  Milliseconds = -1,       /**< Multiplier = 0.001 */
  Microseconds = -2,       /**< Multiplier = 0.000001 */
  Nanoseconds = -3,        /**< Multiplier = 0.000000001 */
};

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_UNITS_H*/
