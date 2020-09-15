/*
//@HEADER
// *****************************************************************************
//
//                               envelope_setup.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

namespace vt {

/** \file */

/**
 * \brief Setup an envelope for the first time
 *
 * \param[in,out] env the envelope
 * \param[in] dest the destination node
 * \param[in] handler the handler
 */
template <typename Env>
inline void envelopeSetup(
  Env& env, NodeType const& dest, HandlerType const& handler
);

/**
 * \brief Initialize an envelope with default/sentinel values
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void envelopeInit(Env& env);

/**
 * \brief Initialize an envelope without defaults
 *
 * \param[in,out] env the envelope
 */
inline void envelopeInitEmpty(Envelope& env);

/**
 * \brief Initialize an envelope via a copy.
 *
 * Some properties of the target envelope are preserved.
 * The target envelope is left unlocked.
 *
 * \param[in,out] env the target envelope to init
 * \param[in] env the original envelope to use as a copy
 */
template <typename Env>
inline void envelopeInitCopy(Env& env, Env const& src_env);

/**
 * \brief Initialize/validate an envelope that has been received.
 *
 * The ref-count is set to zero.
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void envelopeInitRecv(Env& env);

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_setup.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SETUP_H*/
