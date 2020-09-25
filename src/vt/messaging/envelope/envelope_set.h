/*
//@HEADER
// *****************************************************************************
//
//                                envelope_set.h
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

#if !defined INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H
#define INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H

#include "vt/config.h"
#include "vt/messaging/envelope/envelope_type.h"
#include "vt/messaging/envelope/envelope_base.h"

namespace vt {

// Set the type of Envelope

/** \file */

/**
 * \brief Clear all type bits make "normal"
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setNormalType(Env& env);

/**
 * \brief Set pipe bit \c EnvPipe
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setPipeType(Env& env);

/**
 * \brief Set put bit \c EnvPut
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setPutType(Env& env);

/**
 * \brief Set term bit \c EnvTerm
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setTermType(Env& env);

/**
 * \brief Set broadcast bit (changes how \c dest is interpreted) \c EnvBroadcast
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setBroadcastType(Env& env);

/**
 * \brief Set epoch bit \c EnvEpoch
 *
 * Indicates that the envelope is either of type \c EpochActiveEnvelope or \c
 * EpochTagActiveEnvelope depending on whether \c EnvTag is set or not.
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setEpochType(Env& env);

/**
 * \brief Set tag bit \c EnvTag
 *
 * Indicates that the envelope is either of type \c TagActiveEnvelope or \c
 * EpochTagActiveEnvelope depending on whether \c EnvEpoch is set or not.
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void setTagType(Env& env);

/**
 * \brief Set handler field in envelope
 *
 * \param[in,out] env the envelope
 * \param[in] handler the handler
 */
template <typename Env>
inline void envelopeSetHandler(Env& env, HandlerType const& handler);

/**
 * \brief Set destination \c dest field in envelope
 *
 * \param[in,out] env the envelope
 * \param[in] dest the destination if set or root if (non-group) broadcast
 */
template <typename Env>
inline void envelopeSetDest(Env& env, NodeType const& dest);

/**
 * \brief Set reference count on envelope.
 *
 * This overrides the typical mechanism for (de-)referencing May be dangerous to
 * set directly. Typically you should use this in special cases when the current
 * reference count does not apply. For instance, when a message arrives off the
 * network and the count needs to be reset.
 *
 * \param[in,out] env the envelope
 * \param[in] ref the reference count
 */
template <typename Env>
inline void envelopeSetRef(Env& env, RefType const& ref = 0);

/**
 * \brief Set pipe bit \c EnvPipe
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void envelopeSetGroup(Env& env, GroupType const& group = default_group);

#if vt_check_enabled(priorities)
/**
 * \brief Set priority
 *
 * \param[in,out] env the envelope
 * \param[in] priority the priority
 */
template <typename Env>
inline void envelopeSetPriority(Env& env, PriorityType priority);

/**
 * \brief Set priority level
 *
 * \param[in,out] env the envelope
 * \param[in] priority_level the priority level
 */
template <typename Env>
inline void envelopeSetPriorityLevel(Env& env, PriorityLevelType priority_level);
#endif

#if vt_check_enabled(trace_enabled)
/**
 * \brief Set trace event
 *
 * \param[in,out] env the envelope
 * \param[in] evt the trace event
 */
template <typename Env>
inline void envelopeSetTraceEvent(Env& env, trace::TraceEventIDType const& evt);

/**
 * \brief Set whether this envelope is traced at runtime
 *
 * \param[in,out] env the envelope
 * \param[in] is_trace_enabled whether it should be traced
 */
template <typename Env>
inline void envelopeSetTraceRuntimeEnabled(Env& env, bool is_trace_enabled);
#endif

/**
 * \brief Set whether this message's base serializer has been called.
 *
 * \param[in,out] env the envelope
 * \param[in] has_been_serialized value indicating message is serialized
 */
template <typename Env>
inline void envelopeSetHasBeenSerialized(Env& env, bool has_been_serialized);

/**
 * \brief Set whether this message's envelope is locked.
 *
 * A locked message will prevent key parts of the envelope from being updated
 * with a guard assert. This is to prevent accidental edge-cases such as
 * sending a message twice.
 *
 * A message is locked when it is sent and recieved. Unlocking messages
 * should be reserved for special cases as done in some core code.
 *
 * \param[in,out] env the envelope
 * \param[in] is_locked value indicating if message is locked
 */
template <typename Env>
inline void envelopeSetIsLocked(Env& env, bool is_locked);

/**
 * \brief Unlock an envelope, denoting the use is a proxy handler.
 *
 * A received message is normally locked to discourage the
 * re-usage of messages supplied to handlers. However, some
 * proxy/forwarding handlers re-use the original message.
 *
 * This is equivalent to calling \c envelopeSetIsLocked with false
 * and should generally be set inside the message handler itself.
 *
 * \param[in,out] env the envelope
 */
template <typename Env>
inline void envelopeUnlockForForwarding(Env& env);

} /* end namespace vt */

#include "vt/messaging/envelope/envelope_set.impl.h"

#endif /*INCLUDED_MESSAGING_ENVELOPE_ENVELOPE_SET_H*/
