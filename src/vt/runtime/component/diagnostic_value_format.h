/*
//@HEADER
// *****************************************************************************
//
//                          diagnostic_value_format.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_FORMAT_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_FORMAT_H

#include "vt/runtime/component/diagnostic_value.h"
#include "vt/runtime/component/diagnostic_units.h"
#include "vt/runtime/component/diagnostic_enum_format.h"
#include "vt/utils/memory/memory_units.h"

#include <fmt/format.h>

namespace vt { namespace runtime { namespace component { namespace detail {

static constexpr char const* decimal_format = "{:.1f}";

template <typename T, typename _Enable = void>
struct DiagnosticValueFormatter;

template <typename T>
struct DiagnosticValueFormatter<
  T, typename std::enable_if_t<
       std::is_same<T, double>::value or std::is_same<T, float>::value
     >
> {
  static constexpr char const* format = decimal_format;
};

template <typename T>
struct DiagnosticValueFormatter<
  T, typename std::enable_if_t<
       not (std::is_same<T, double>::value or std::is_same<T, float>::value)
     >
> {
  static constexpr char const* format = "{}";
};

template <typename T>
struct DiagnosticStringizer {

  static DiagnosticString get(
    DiagnosticValueWrapper<T> wrapper, DiagnosticUnit unit
  ) {
    std::string const spec = DiagnosticValueFormatter<T>::format;
    std::string const double_spec = std::string{decimal_format};

    DiagnosticString dstr;
    dstr.min_value_ = getValueWithUnits(wrapper.min(), unit, spec);
    dstr.max_value_ = getValueWithUnits(wrapper.max(), unit, spec);
    dstr.sum_value_ = getValueWithUnits(wrapper.sum(), unit, spec);
    dstr.avg_value_ = getValueWithUnits(wrapper.avg(), unit, double_spec);
    dstr.std_value_ = getValueWithUnits(wrapper.stdv(), unit, double_spec);
    dstr.var_value_ = getValueWithUnits(wrapper.var(), unit, double_spec);
    return dstr;
  }

private:
  template <typename U>
  static std::string getValueWithUnits(
    U val, DiagnosticUnit unit, std::string default_spec
  ) {
    using util::memory::MemoryUnitEnum;
    using util::memory::getMemoryUnitName;

    switch (unit) {
    case DiagnosticUnit::Bytes: {
      // Start with the largest unit, testing if its appropriate for the value,
      // and if not, downgrade it until we find one that works
      auto multiplier = static_cast<int8_t>(MemoryUnitEnum::Gigabytes);
      for ( ; multiplier > 0; multiplier--) {
        auto value_tmp = static_cast<double>(val);
        for (int8_t i = 0; i < static_cast<int8_t>(multiplier); i++) {
          value_tmp /= 1024.0;
        }
        if (value_tmp > 1.) {
          goto found_appropiate_memory_multiplier;
        }
      }

      // We found a multiplier that results in a value over 1.0, use it
      found_appropiate_memory_multiplier:
      vtAssert(
        multiplier <= static_cast<int8_t>(MemoryUnitEnum::Gigabytes) and
        multiplier >= 0,
        "Must be a valid memory unit"
      );

      auto unit_name = getMemoryUnitName(static_cast<MemoryUnitEnum>(multiplier));

      // Use the default spec if we have base units (multiplier of 1)---this
      // means that integer types won't have a decimal place (as desired)
      // Otherwise, add decimal places since we used a multiplier

      if (multiplier == 0) {
        return fmt::format(default_spec + " {}", val, unitFormat(unit_name));
      } else {
        // Compute the new value with multiplier as a double
        auto new_value = static_cast<double>(val);
        for (int8_t i = 0; i < static_cast<int8_t>(multiplier); i++) {
          new_value /= 1024.0;
        }

        auto decimal = std::string{decimal_format};
        return fmt::format(decimal + " {}", new_value, unitFormat(unit_name));
      }
      break;
    }
    case DiagnosticUnit::Units: {

      // Start with the largest unit, testing if its appropriate for the value,
      // and if not, downgrade it until we find one that works
      auto multiplier = static_cast<int8_t>(UnitMultiplier::Trillions);
      for ( ; multiplier > 0; multiplier--) {
        auto value_tmp = static_cast<double>(val);
        for (int8_t i = 0; i < static_cast<int8_t>(multiplier); i++) {
          value_tmp /= 1000.0;
        }
        if (value_tmp > 1.) {
          goto found_appropiate_unit_multiplier;
        }
      }

      // We found a multiplier that results in a value over 1.0, use it
      found_appropiate_unit_multiplier:
      vtAssert(
        multiplier <= static_cast<int8_t>(UnitMultiplier::Trillions) and
        multiplier >= 0,
        "Must be a valid unit multiplier"
      );

      auto unit_name =
        diagnosticMultiplierString(static_cast<UnitMultiplier>(multiplier));

      // Use the default spec if we have base units (multiplier of 1)---this
      // means that integer types won't have a decimal place (as desired)
      // Otherwise, add decimal places since we used a multiplier

      if (multiplier == 0) {
        return fmt::format(default_spec + " {}", val, unitFormat(unit_name));
      } else {
        // Compute the new value with multiplier as a double
        auto new_value = static_cast<double>(val);
        for (int8_t i = 0; i < static_cast<int8_t>(multiplier); i++) {
          new_value /= 1000.0;
        }

        auto decimal = std::string{decimal_format};
        return fmt::format(decimal + " {}", new_value, unitFormat(unit_name));
      }

      break;
    }
    case DiagnosticUnit::UnitsPerSecond:
    default:
      return fmt::format(default_spec, val);
      break;
    }
    return "";
  }

  static std::string unitFormat(std::string unit_name) {
    return fmt::format("{:<3}", unit_name);
  }

};

}}}} /* end namespace vt::runtime::component::detail */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_FORMAT_H*/
