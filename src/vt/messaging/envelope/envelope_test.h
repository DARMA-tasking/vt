/*
//@HEADER
// *****************************************************************************
//
//                               envelope_test.h
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

#if !defined INCLUDED_VT_MESSAGING_ENVELOPE_ENVELOPE_TEST_H
#define INCLUDED_VT_MESSAGING_ENVELOPE_ENVELOPE_TEST_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

namespace vt { namespace messaging {

// Test the type of Envelope

/** \file */

/**
 * \brief Test if envelope type is term \c EnvTerm
 *
 * \param[in] env the envelope
 *
 * \return whether the bit is set
 */
template <typename Env>
inline bool envelopeIsTerm(Env const& env);

/**
 * \brief Test if envelope type is pipe \c EnvPipe
 *
 * This designation if enabled changes how the group bits are
 * interpreted---either as the \c GroupType or \c PipeType. Note that this works
 * because its impossible for a message to be sent both as a group and pipe
 * simultaneously.
 *
 * \param[in] env the envelope
 *
 * \return whether the bit is set
 */
template <typename Env>
inline bool envelopeIsPipe(Env const& env);

/**
 * \brief Test if envelope type is put \c EnvPut
 *
 * \param[in] env the envelope
 *
 * \return whether the bit is set
 */
template <typename Env>
inline bool envelopeIsPut(Env const& env);

/**
 * \brief Test if envelope type is broadcast \c EnvBroadcast
 *
 * \param[in] env the envelope
 *
 * \return whether the bit is set
 */
template <typename Env>
inline bool envelopeIsBcast(Env const& env);

/**
 * \brief Test if envelope type is epoch \c EnvEpoch
 *
 * \param[in] env the envelope
 *
 * \return whether the bit is set
 */
template <typename Env>
inline bool envelopeIsEpochType(Env const& env);

/**
 * \brief Test if envelope type is tag \c EnvTag
 *
 * \param[in] env the envelope
 *
 * \return whether the bit is set
 */
template <typename Env>
inline bool envelopeIsTagType(Env const& env);

/**
 * \brief Test if the message's base serializer has been called.
 *
 * \param[in] env the envelope
 */
template <typename Env>
inline bool envelopeHasBeenSerialized(Env& env);

/**
 * \brief Test if the message's envelope has been locked.
 *
 * \param[in] env the envelope
 */
template <typename Env>
inline bool envelopeIsLocked(Env& env);

}} //end namespace vt::messaging

#include "vt/messaging/envelope/envelope_test.impl.h"

#endif /*INCLUDED_VT_MESSAGING_ENVELOPE_ENVELOPE_TEST_H*/
