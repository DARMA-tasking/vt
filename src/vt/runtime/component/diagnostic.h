/*
//@HEADER
// *****************************************************************************
//
//                                 diagnostic.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_H

#include "vt/runtime/component/component_name.h"
#include "vt/runtime/component/component_reduce.h"
#include "vt/runtime/component/diagnostic_types.h"
#include "vt/runtime/component/diagnostic_units.h"
#include "vt/runtime/component/diagnostic_value.h"
#include "vt/runtime/component/diagnostic_meter.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace vt { namespace runtime { namespace component {

/**
 * \struct Diagnostic diagnostic.h vt/runtime/component/diagnostic.h
 *
 * \brief The abstract \c Diagnostic trait for outputting debugging state
 * information generically across VT components
 */
struct Diagnostic : ComponentName, ComponentReducer {
  using DiagnosticBasePtrType = std::unique_ptr<detail::DiagnosticBase>;
  using UpdateType = DiagnosticUpdate;
  using UnitType = DiagnosticUnit;

  virtual void dumpState() = 0;

  /**
   * \internal \brief Apply a function to each base diagnostic in a consistent
   * order across all nodes
   *
   * \param[in] apply function to apply that takes \c detail::DiagnosticBase
   */
  void foreachDiagnostic(std::function<void(detail::DiagnosticBase*)> apply);

  /**
   * \internal \brief Pre-diagnostic hook; this fires before finalize but after
   * all work is done and the system is about to shut down.
   *
   * This is typically used to compute some final timings that the system might
   * use to output time-based diagnostic metrics
   *
   * \note Components can rely on this being called exactly once in the
   * lifecycle
   */
  virtual void preDiagnostic() { }

protected:
  /**
   * \internal \brief Register a new diagnostic counter
   *
   * \param[in] key unique key for diagnostic, should match across nodes
   * \param[in] desc description of the diagnostic value
   * \param[in] unit the unit type for this diagnostic
   *
   * \return the counter
   */
  template <typename T>
  Counter<T> registerCounter(
    std::string const& key, std::string const& desc,
    DiagnosticUnit unit = DiagnosticUnit::Units
  );

  /**
   * \internal \brief Register a new diagnostic gauge
   *
   * \param[in] key unique key for diagnostic, should match across nodes
   * \param[in] desc description of the diagnostic value
   * \param[in] unit the unit type for this diagnostic
   *
   * \return the counter
   */
  template <typename T>
  Gauge<T> registerGauge(
    std::string const& key, std::string const& desc,
    DiagnosticUnit unit = DiagnosticUnit::Units
  );

  /**
   * \internal \brief Register a new diagnostic timer
   *
   * \param[in] key unique key for diagnostic, should match across nodes
   * \param[in] desc description of the diagnostic value
   * \param[in] unit the unit type for this diagnostic
   *
   * \return the counter
   */
  template <typename T>
  Timer<T> registerTimer(
    std::string const& key, std::string const& desc,
    DiagnosticUnit unit = DiagnosticUnit::Seconds
  );

  /**
   * \internal \brief Register a new diagnostic
   *
   * \param[in] key unique key for diagnostic, should match across nodes
   * \param[in] desc description of the diagnostic value
   * \param[in] update the update operator that is applied for
   * \c updateDiagnostic
   * \param[in] unit the unit type for this diagnostic
   * \param[in] type the type of diagnostic being registered
   * \param[in] initial_value the initial value for the diagnostic
   *
   * \return the underlying diagnostic value
   */
  template <typename T>
  detail::DiagnosticValue<T>* registerDiagnostic(
    std::string const& key, std::string const& desc, DiagnosticUpdate update,
    DiagnosticUnit unit = DiagnosticUnit::Units,
    DiagnosticTypeEnum type = DiagnosticTypeEnum::PerformanceDiagnostic,
    T initial_value = {}
  );

  /**
   * \internal \brief Update the current diagnostic value for a particular key
   *
   * How the update will be applied depends on the \c DiagnosticUpdate that was
   * registered when the diagnostic was created:
   *
   * \c DiagnosticUpdate::Sum : Accumulate up the values
   * \c DiagnosticUpdate::Avg : Average the values
   * \c DiagnosticUpdate::Replace : Replace the existing value with each update
   *
   * \param[in] key unique key for diagnostic, should match across nodes
   * \param[in] value the value to apply the updater to
   *
   * \return reference to diagnostic value
   */
  template <typename T>
  void updateDiagnostic(std::string const& key, T value);

private:
  /// Type-erased base pointers to different diagnostic values
  std::unordered_map<std::string, DiagnosticBasePtrType> values_;
};

}}} /* end namespace vt::runtime::component */

#include "vt/runtime/component/diagnostic.impl.h"

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_H*/
