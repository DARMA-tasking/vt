/*
//@HEADER
// *****************************************************************************
//
//                           diagnostic_enum_format.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_ENUM_FORMAT_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_ENUM_FORMAT_H

#include "vt/runtime/component/diagnostic_units.h"
#include "vt/runtime/component/diagnostic_types.h"

#include <string>

namespace vt { namespace runtime { namespace component {

/**
 * \brief Convert a diagnostic update enum to a human-readable string
 *
 * \param[in] update the update type
 *
 * \return a string for printing
 */
std::string diagnosticUpdateTypeString(DiagnosticUpdate update);

/**
 * \internal \brief Whether to print the total (or sum across processors) of a
 * given type of update
 *
 * For example, a \c DiagnosticUpdate::Max value updater does not make sense to
 * print the total across nodes.
 *
 * \param[in] update the update type
 *
 * \return whether to print the total
 */
bool diagnosticShowTotal(DiagnosticUpdate update);

/**
 * \brief Convert a diagnostic unit type to a human-readable string
 *
 * \param[in] unit the unit
 *
 * \return a string for printing
 */
std::string diagnosticUnitTypeString(DiagnosticUnit unit);

/**
 * \brief Convert a diagnostic unit multiplier to a human-readable string
 *
 * \param[in] multiplier the multiplier type
 *
 * \return a string for printing
 */
std::string diagnosticMultiplierString(UnitMultiplier multiplier);

/**
 * \brief Convert a time unit multiplier to a human-readable string
 *
 * \param[in] time the time unit
 *
 * \return a string for printing
 */
std::string diagnosticTimeMultiplierString(TimeMultiplier time);

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_ENUM_FORMAT_H*/
