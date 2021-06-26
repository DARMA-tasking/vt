/*
//@HEADER
// *****************************************************************************
//
//                           lb_args_enum_converter.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_ARGS_ENUM_CONVERTER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_ARGS_ENUM_CONVERTER_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/read_lb.h"

#include <string>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct LBArgsEnumConverter
 *
 * \brief A VT component that converts enumerated values to their
 * stringifications for the purposes of reading LB arguments from the LB
 * spec file.
 */
template <typename T>
struct LBArgsEnumConverter {
  using EnumToStrMap = typename std::unordered_map<T, std::string>;
  using StrToEnumMap = typename std::unordered_map<std::string, T>;

  /**
   * \brief Construct a converter for a specific enumerated type
   *
   * \param[in] par_name name of the LB arg in the spec file
   * \param[in] enum_type name of the enumerated type being converted
   * \param[in] enum_to_str unordered map of enumerated values to their strings
   */
  LBArgsEnumConverter(
    const std::string &par_name,
    const std::string &enum_type,
    const EnumToStrMap &enum_to_str
  ) : enum_to_str_(enum_to_str),
      par_name_(par_name),
      enum_type_(enum_type)
  {
    for (const auto &it : enum_to_str_) {
      str_to_enum_[it.second] = it.first;
    }
  }

  virtual ~LBArgsEnumConverter() = default;

  /**
   * \brief Convert from an enumerated value to a string
   *
   * Using the mapping provided at construction, return the string to which the
   * passed enumerated value maps.
   */
  std::string getString(T e) const {
    auto it = enum_to_str_.find(e);
    if (it == enum_to_str_.end()) {
      // this error indicates that you need to update the constructor for
      // this converter to include all options
      auto err = fmt::format(
        "LBArgsEnumConverter: enum '{}' value '{}' corresponding to LB "
        "argument '{}' does not have a string associated with it",
        enum_type_, e, par_name_
      );
      vtAbort(err);
    }
    return it->second;
  }

  /**
   * \brief Convert from string to the enumerated value
   *
   * Using the reverse of the mapping provided at construction, return the
   * enumerated value to which the passed string maps.
   */
  T getEnum(const std::string &s) const {
    auto it = str_to_enum_.find(s);
    if (it == str_to_enum_.end()) {
      // either the user typed something in wrong or you need to update the
      // constructor for this converter to include all options
      auto err = fmt::format(
        "LBArgsEnumConverter: LB argument '{}' string '{}' is not a recognized "
        "option for enum '{}'",
        par_name_, s, enum_type_
      );
      vtAbort(err);
    }
    return it->second;
  }

  /**
   * \brief Read string from the spec and convert to enum
   *
   * Read the string from the spec entry and convert it to an enumerated value
   * using the reverse of the mapping provided at construction.
   */
  T getFromSpec(balance::SpecEntry* spec, T def_value) const {
    std::string spec_value = spec->getOrDefault<std::string>(
      par_name_, getString(def_value)
    );
    return getEnum(spec_value);
  }

private:
  EnumToStrMap enum_to_str_;  //< 1 to 1 mapping of enumerated values to strings
  StrToEnumMap str_to_enum_;  //< 1 to 1 mapping of strings to enumerated values
  std::string par_name_;      //< parameter to look for in the spec file
  std::string enum_type_;     //< name of the enumerated type for error handling
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_ARGS_ENUM_CONVERTER_H*/
