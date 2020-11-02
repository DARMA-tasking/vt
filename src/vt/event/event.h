/*
//@HEADER
// *****************************************************************************
//
//                                   event.h
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

#if !defined INCLUDED_EVENT_EVENT_H
#define INCLUDED_EVENT_EVENT_H

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/context/context.h"
#include "vt/event/event_record.h"
#include "vt/event/event_id.h"
#include "vt/event/event_holder.h"
#include "vt/event/event_msgs.h"

#include <checkpoint/checkpoint.h>

#include <memory>
#include <vector>
#include <list>
#include <functional>
#include <unordered_map>

#include <mpi.h>

namespace vt { namespace event {

/** \file */

/**
 * \internal \brief State of an event that may be local or remote
 *
 * Tracks the state of a distributed event
 */
enum class EventState : int8_t {
  EventReady = 1,               /**< Indicates event is ready/satisfied */
  EventWaiting = 2,             /**< Indicates event is waiting on op */
  EventRemote = 3               /**< Indicates event is non-local */
};

/**
 * \struct AsyncEvent event.h vt/event/event.h
 *
 * \brief Used to track events
 *
 * Component to track events in the system to trigger actions or other events
 */
struct AsyncEvent : runtime::component::PollableComponent<AsyncEvent> {
  using EventRecordTypeType = eEventRecord;
  using EventManagerType = EventIDManager;
  using EventStateType = EventState;
  using EventRecordType = EventRecord;
  using EventRecordPtrType = std::unique_ptr<EventRecordType>;
  using EventHolderType = EventHolder;
  using EventHolderPtrType = EventHolder*;
  using TypedEventContainerType = std::list<EventHolderType>;
  using EventContIter = typename TypedEventContainerType::iterator;
  using EventContainerType = std::unordered_map<EventType, EventContIter>;

  AsyncEvent() = default;

  virtual ~AsyncEvent();

  void initialize() override;
  void finalize() override;

  /**
   * \brief Create a new event
   *
   * \param[in] type the type of event to create
   * \param[in] node the node that's embedded in the event bit field
   *
   * \return a new event identifier
   */
  EventType createEvent(EventRecordTypeType const& type, NodeType const& node);

  /**
   * \brief Get the event record
   *
   * \param[in] event the event identifier
   *
   * \return the event record
   */
  EventRecordType& getEvent(EventType const& event);

  /**
   * \brief Get the owning node for an event
   *
   * \param[in] event the event identifier
   *
   * \return the node that owns the event
   */
  NodeType getOwningNode(EventType const& event);

  /**
   * \brief Create a new MPI event that holds a MPI_Request
   *
   * \param[in] node the node on which the MPI event exists
   *
   * \return the event identifier
   */
  EventType createMPIEvent(NodeType const& node);

  /**
   * \brief Create a regular type event
   *
   * \param[in] node the node that owns it
   *
   * \return the event identifier
   */
  EventType createNormalEvent(NodeType const& node);

  /**
   * \brief Create a parent event that can have multiple children
   *
   * \param[in] node the node that owns it
   *
   * \return the event identifier
   */
  EventType createParentEvent(NodeType const& node);

  /**
   * \brief Get the holder for an event
   *
   * \param[in] event the event identifier
   *
   * \return the holder
   */
  EventHolderType& getEventHolder(EventType const& event);

  /**
   * \brief Check if a holder exist for an event
   *
   * \param[in] event the event identifier
   *
   * \return if it exists
   */
  bool holderExists(EventType const& event);
  bool needsPolling(EventRecordTypeType const& type);
  void removeEventID(EventType const& event);
  EventStateType testEventComplete(EventType const& event);
  EventType attachAction(EventType const& event, ActionType callable);
  void testEventsTrigger(int const& num_events = num_check_actions);
  int progress() override;
  bool isLocalTerm();

  static void eventFinished(EventFinishedMsg* msg);
  static void checkEventFinished(EventCheckFinishedMsg* msg);

  std::string name() override { return "AsyncEvent"; }

  template <
    typename SerializerT,
    typename = std::enable_if_t<
      std::is_same<SerializerT, checkpoint::Footprinter>::value
    >
  >
  void serialize(SerializerT& s) {
    s | cur_event_
      | event_container_
      | polling_event_container_;

    s.countBytes(lookup_container_);

  # if vt_check_enabled(trace_enabled)
    s | trace_event_polling;
  # endif
  }

private:
  // next event id
  EventType cur_event_ = 0;

  // std::list of events
  TypedEventContainerType event_container_;

  // list of events that need polling for progress
  TypedEventContainerType polling_event_container_;

  // container to lookup events by EventType
  EventContainerType lookup_container_;

# if vt_check_enabled(trace_enabled)
  vt::trace::UserEventIDType trace_event_polling = 0;
# endif

private:
  diagnostic::Counter eventPollCount;
  diagnostic::Gauge eventSizeGauge;
  diagnostic::Timer mpiEventWaitTime;
};

}} //end namespace vt::event

namespace vt {

using EventRecordType = event::EventRecord;

extern event::AsyncEvent* theEvent();

} //end namespace vt

#endif /*INCLUDED_EVENT_EVENT_H*/
