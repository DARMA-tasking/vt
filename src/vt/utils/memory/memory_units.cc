/*
//@HEADER
// *****************************************************************************
//
//                               memory_units.cc
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

#include "vt/config.h"
#include "vt/utils/memory/memory_units.h"

#include <unordered_map>

namespace vt { namespace util { namespace memory {

std::unordered_map<MemoryUnitEnum, std::string> memory_unit_names = {
  {MemoryUnitEnum::Bytes,     std::string{"B"}},
  {MemoryUnitEnum::Kilobytes, std::string{"KiB"}},
  {MemoryUnitEnum::Megabytes, std::string{"MiB"}},
  {MemoryUnitEnum::Gigabytes, std::string{"GiB"}}
};

std::string getMemoryUnitName(MemoryUnitEnum unit) {
  return memory_unit_names[unit];
}

MemoryUnitEnum getUnitFromString(std::string const& unit) {
  for (auto&& elm : memory_unit_names) {
    if (unit == elm.second) {
      return elm.first;
    }
  }
  return MemoryUnitEnum::Bytes;
}

std::tuple<std::string, double> getBestMemoryUnit(std::size_t bytes) {
  auto multiplier = static_cast<int8_t>(MemoryUnitEnum::Yottabytes);
  for ( ; multiplier > 0; multiplier--) {
    auto value_tmp = static_cast<double>(bytes);
    for (int8_t i = 0; i < static_cast<int8_t>(multiplier); i++) {
      value_tmp /= 1024.0;
    }
    if (value_tmp >= 1.) {
      break;
    }
  }

  // We found a multiplier that results in a value over 1.0, use it
  vtAssert(
    multiplier <= static_cast<int8_t>(MemoryUnitEnum::Yottabytes) and
    multiplier >= 0,
    "Must be a valid memory unit"
  );

  auto unit_name = getMemoryUnitName(static_cast<MemoryUnitEnum>(multiplier));

  auto new_value = static_cast<double>(bytes);
  for (int8_t i = 0; i < static_cast<int8_t>(multiplier); i++) {
    new_value /= 1024.0;
  }

  return std::make_tuple(unit_name, new_value);
}

}}} /* end namespace vt::util::memory */
