/*
//@HEADER
// *****************************************************************************
//
//                                pending_send.h
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

#if !defined INCLUDED_VT_MESSAGING_PENDING_SEND_H
#define INCLUDED_VT_MESSAGING_PENDING_SEND_H

#include "vt/config.h"
#include "vt/messaging/message.h"

#include <functional>
#include <cstddef>

namespace vt { namespace messaging {

/** \file */

/**
 * \struct PendingSend
 *
 * \brief A pending send (or other similar operation) that is delayed until this
 * holder goes out of scope.
 *
 * \c PendingSend holds the message and dispatch function for a send (or
 * broadcast or similar event) to a node, collection, objgroup, etc. This allows
 * VT to expand the DAG before it actually violates a data or control dependency
 * by expanding and prematurely performing the operation. We can then wire
 * future sends up with past ones by associating future events with termination
 * sequencing---see \c CollectionChainSet
 */
struct PendingSend final {
  /// Function for complex action on send---takes a message to operate on
  using SendActionType = std::function<void(MsgSharedPtr<BaseMsgType>&)>;
  using EpochActionType = std::function<void()>;

  /**
   * \brief Construct a pending send.
   *
   * This is the prefered form, especially for internal constructs.
   * If an action is specified it will be run. Otherwise the default
   * internal message-sending action is applied.
   *
   * \param[in] in_msg the message to send
   * \param[in] in_msg_size the size of the message (type is erased)
   * \param[in] in_action the "send" action to run, if overridden.
   */
  PendingSend(
    MsgSharedPtr<BaseMsgType>& in_msg,
    ByteType in_msg_size,
    SendActionType in_action = nullptr
  ) : msg_(in_msg)
    , msg_size_(in_msg_size)
    , send_action_(in_action)
  {
    produceMsg();
  }

  /**
   * \brief Construct a pending send that invokes a callback.
   *
   * \note This form does not implictly send a message. The callback
   * action is responsible for all further work. It is a useful
   * construct to delay the callback and ensure an epoch is produced.
   *
   * This constructor is for a complex \c PendingSend that holds a \c
   * std::function for performing the send (e.g., sending to a collection
   * element). When released, it will run the \c in_action of type \c
   * SendActionType.
   *
   * \param[in] in_msg the message to send
   * \param[in] in_action the "send" action to run
   */
  template <typename MsgT>
  PendingSend(
    MsgSharedPtr<MsgT>& in_msg,
    SendActionType in_action
  ) : msg_(in_msg.template toVirtual<BaseMsgType>())
    , msg_size_(sizeof(MsgT))
    , send_action_(in_action)
  {
    produceMsg();
  }

  /**
   * \brief Construct a pending send to push an epoch.
   *
   * \note This form does not implictly send a message.
   *
   * \param[in] in_msg the message to send
   * \param[in] in_action the "send" action to run
   */
  PendingSend(EpochType ep, EpochActionType const& in_action);

  explicit PendingSend(std::nullptr_t) { }
  PendingSend(PendingSend&& in) noexcept;

  PendingSend(const PendingSend&) = delete;
  PendingSend& operator=(PendingSend&& in) = delete;
  PendingSend& operator=(PendingSend& in) = delete;

  /**
   * \brief Release the pending send on destruction (when this goes out of scope)
   *
   * \code
   *   {
   *     auto ps = vt::theMsg()->sendMsg<MyMsg, handler>(msg);
   *   } // Message is sent here when the destructor runs
   *
   *   // Message is sent right away on this following line as the PendingSend
   *   // is not captured
   *   vt::theMsg()->sendMsg<MyMsg, handler>(msg);
   * \endcode
   */
  ~PendingSend() { release(); }

  /**
   * \brief Release the message, run action if needed
   */
  void release();

private:

  /**
   * \brief Get the epoch produced when holder was created
   *
   * This is required because the epoch on the envelope can change in some cases
   * in between when this is created and actually released.
   *
   * \return the produce epoch
   */
  EpochType getProduceEpochFromMsg() const;

  /**
   * \brief Produce on the messages epoch to inhibit early termination
   */
  void produceMsg();

  /**
   * \brief Consume on the messages epoch to inhibit early termination
   */
  void consumeMsg();

  /// Send the message saved directly or trigger the lambda for
  /// specialized sends from the pending holder
  void sendMsg();

private:
  MsgPtr<BaseMsgType> msg_ = nullptr;
  ByteType msg_size_ = no_byte;
  SendActionType send_action_ = {};
  EpochActionType epoch_action_ = {};
  EpochType epoch_produced_ = no_epoch;
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_PENDING_SEND_H*/
