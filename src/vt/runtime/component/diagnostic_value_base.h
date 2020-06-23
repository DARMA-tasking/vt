/*
//@HEADER
// *****************************************************************************
//
//                           diagnostic_value_base.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_BASE_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_BASE_H

#include "vt/runtime/component/diagnostic_types.h"
#include "vt/runtime/component/diagnostic_string.h"

#include <string>

namespace vt { namespace runtime { namespace component {

struct Diagnostic;

}}} /* end namespace vt::runtime::component */

namespace vt { namespace runtime { namespace component { namespace detail {

/**
 * \struct DiagnosticBase
 *
 * \brief Base class for a diagnostic value with type of actual value erased
 */
struct DiagnosticBase {

  /**
   * \internal \brief Construct a new diagnostic base value
   *
   * \param[in] in_key key for the diagnostic value lookup
   * \param[in] in_desc the long description of the diagnostic form
   * \param[in] in_update the type of update to use for the underlying value
   * \param[in] in_type the type of diagnostic
   */
  DiagnosticBase(
    std::string const& in_key, std::string const& in_desc,
    DiagnosticUpdate in_update, DiagnosticTypeEnum in_type
  ) : type_(in_type),
      update_(in_update),
      key_(in_key),
      desc_(in_desc)
  { }

  /**
   * \internal \brief Get the diagnostic value key
   *
   * \return the key
   */
  std::string const& getKey() const { return key_; }

  /**
   * \internal \brief Get the description of the diagnostic value
   *
   * \return the description
   */
  std::string const& getDescription() const { return desc_; }

  /**
   * \internal \brief Get the type of diagnostic value
   *
   * \return the type of diagnostic
   */
  DiagnosticTypeEnum getType() const { return type_; }

  /**
   * \internal \brief Get the type of diagnostic update
   *
   * \return the type of diagnostic update
   */
  DiagnosticUpdate getUpdateType() const { return update_; }

  /**
   * \internal \brief Reduce over this value to compute stats over all the nodes
   * for this particular diagnostic value
   *
   * \param[in] diagnostic the component with the \c Diagnostic trait to reduce
   * \param[in] out type-erased values output from reduction over all nodes
   */
  virtual void reduceOver(Diagnostic* diagnostic, DiagnosticString* out) = 0;

protected:
  DiagnosticTypeEnum const type_; /**< The diagnostic type */
  DiagnosticUpdate const update_; /**< The diagnostic value update type */
  std::string const key_;         /**< Key for looking up the value */
  std::string const desc_;        /**< Long description of the value */
};

}}}} /* end namespace vt::runtime::component::detail */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_VALUE_BASE_H*/
