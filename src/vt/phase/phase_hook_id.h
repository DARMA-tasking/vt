/*
//@HEADER
// *****************************************************************************
//
//                               phase_hook_id.h
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

#if !defined INCLUDED_VT_PHASE_PHASE_HOOK_ID_H
#define INCLUDED_VT_PHASE_PHASE_HOOK_ID_H

#include "vt/phase/phase_hook_enum.h"

namespace vt { namespace phase {

// forward-decl for friendship
struct PhaseManager;

/**
 * \struct PhaseHookID
 *
 * \brief A registered phase hook used to identify it and unregister it.
 */
struct PhaseHookID {

private:
  /**
   * \internal
   * \brief Used by the system to create a new phase hook ID
   *
   * \param[in] in_type the type of hook
   * \param[in] in_id the registered ID
   * \param[in] in_is_collective whether it is collective
   * \param[in] in_is_rooted whether it is rooted
   */
  PhaseHookID(
    PhaseHook in_type, std::size_t in_id, bool in_is_collective,
    bool in_is_rooted
  ) : type_(in_type),
      id_(in_id),
      is_collective_(in_is_collective),
      is_rooted_(in_is_rooted)
  { }

  friend struct PhaseManager;

public:
  /**
   * \brief Get the type of hook
   *
   * \return the type of hook
   */
  PhaseHook getType() const { return type_; }

  /**
   * \brief Get the ID of the registered hook
   *
   * \return the registered hook ID
   */
  std::size_t getID() const { return id_; }

  /**
   * \brief Get whether the hook is collective or not
   *
   * \return whether it is collective
   */
  std::size_t getIsCollective() const { return is_collective_; }

  /**
   * \brief Get whether the hook is rooted or not
   *
   * \return whether it is rooted
   */
  std::size_t getIsRooted() const { return is_rooted_; }

private:
  PhaseHook type_;
  std::size_t id_ = 0;
  bool is_collective_ = false;
  bool is_rooted_ = false;
};

}} /* end namespace vt::phase */

#endif /*INCLUDED_VT_PHASE_PHASE_HOOK_ID_H*/
