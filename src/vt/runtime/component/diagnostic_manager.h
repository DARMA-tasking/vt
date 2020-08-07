/*
//@HEADER
// *****************************************************************************
//
//                             diagnostic_manager.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_MANAGER_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_MANAGER_H

#include "vt/runtime/component/component_pack.h"

#include <chrono>
#include <vector>
#include <memory>

namespace vt { namespace runtime { namespace component {

namespace detail {
struct DiagnosticBase;
} /* end namespace detail */

/**
 * \internal \struct DiagnosticManager
 *
 * \brief Manager various diagnostic snapshots over time. Snapshot 0 is the
 * default over the whole execution.
 */
struct DiagnosticManager {
  using SnapshotType = int;
  using DiagnosticBasePtr = std::unique_ptr<detail::DiagnosticBase>;
  using DiagnosticSnapshot = std::vector<DiagnosticBasePtr>;

  /**
   * \internal \brief Construct a diagnostic manager
   *
   * \param[in] pack a non-owning pointer to the underlying component pack that
   * this diagnostic manager runs over
   */
  DiagnosticManager(ComponentPack* in_pack)
    : pack_(in_pack)
  {
    // setup default snapshot (whole execution time)
    addSnapshot(std::chrono::milliseconds(0));
    // setup default, 1-second snapshot
    addSnapshot(std::chrono::milliseconds(1000));
  }

  /**
   * \brief Collectively add a new snapshot period (must be called in the same
   * order on each node).
   *
   * \warning If the snapshots should be coordinated across nodes, synchronize
   * across nodes before invoking this so the timed trigger fire at about the
   * same time across nodes.
   *
   * \param[in] period the period in milliseconds
   *
   * \return the snapshot ID
   */
  SnapshotType addSnapshot(std::chrono::milliseconds period);

  /**
   * \internal \brief Get the collected snapshot data across all components
   */
  std::vector<std::vector<DiagnosticSnapshot>>& getData();

private:
  /**
   * \internal \brief Trigger collection at end of snapshot period
   *
   * \param[in] snapshot the snapshot
   */
  void triggerSnapshot(SnapshotType snapshot);

private:
  /// Component pack that this manager manages the diagnostics of
  ComponentPack* pack_ = nullptr;
  /// Registered snapshot periods
  std::vector<std::chrono::milliseconds> snapshots_;
  /// Collected data on varying periods
  std::vector<std::vector<DiagnosticSnapshot>> data_;
};

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_MANAGER_H*/
