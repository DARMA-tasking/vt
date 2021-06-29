/*
//@HEADER
// *****************************************************************************
//
//                          diagnostic_enum_format.cc
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

#include "vt/config.h"
#include "vt/runtime/component/diagnostic_enum_format.h"

namespace vt { namespace runtime { namespace component {

std::string diagnosticUpdateTypeString(DiagnosticUpdate update) {
  switch (update) {
  case DiagnosticUpdate::Sum:     return "SUM";       break;
  case DiagnosticUpdate::Avg:     return "MEAN";      break;
  case DiagnosticUpdate::Replace: return "REPLACE";   break;
  case DiagnosticUpdate::Min:     return "MIN";       break;
  case DiagnosticUpdate::Max:     return "MAX";       break;
  default:                        return "<unknown>"; break;
  }
  return "";
}

bool diagnosticShowTotal(DiagnosticUpdate update) {
  switch (update) {
  case DiagnosticUpdate::Sum:
  case DiagnosticUpdate::Replace:
    return true;
    break;
  // for Avg/Min/Max types, showing the total across nodes does not make sense
  case DiagnosticUpdate::Avg:
  case DiagnosticUpdate::Min:
  case DiagnosticUpdate::Max:
    return false;
    break;
  default:
    return true;
    break;
  }
  return true;
}

std::string diagnosticUnitTypeString(DiagnosticUnit unit) {
  switch (unit) {
  case DiagnosticUnit::Bytes:          return "bytes";       break;
  case DiagnosticUnit::Units:          return "units";       break;
  case DiagnosticUnit::UnitsPerSecond: return "units/sec";   break;
  case DiagnosticUnit::Seconds:        return "sec";         break;
  default:                             return "<unknown>"; break;
  }
  return "";
}

std::string diagnosticMultiplierString(UnitMultiplier multiplier) {
  switch (multiplier) {
  case UnitMultiplier::Base:         return "";          break;
  case UnitMultiplier::Thousands:    return "K";         break;
  case UnitMultiplier::Millions:     return "M";         break;
  case UnitMultiplier::Billions:     return "B";         break;
  case UnitMultiplier::Trillions:    return "t";         break;
  case UnitMultiplier::Quadrillions: return "q";         break;
  case UnitMultiplier::Quintillions: return "Q";         break;
  default:                           return "<unknown>"; break;
  }
  return "";
}

std::string diagnosticTimeMultiplierString(TimeMultiplier time) {
  switch (time) {
  case TimeMultiplier::Seconds:       return "sec";       break;
  case TimeMultiplier::Milliseconds:  return "ms";        break;
  case TimeMultiplier::Microseconds:  return "μs";        break;
  case TimeMultiplier::Nanoseconds:   return "ns";        break;
  default:                            return "<unknown>"; break;
  }
  return "";
}

}}} /* end namespace vt::runtime::component */
