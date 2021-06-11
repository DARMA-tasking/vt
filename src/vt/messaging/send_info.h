/*
//@HEADER
// *****************************************************************************
//
//                                 send_info.h
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

#if !defined INCLUDED_VT_MESSAGING_SEND_INFO_H
#define INCLUDED_VT_MESSAGING_SEND_INFO_H

#include "vt/config.h"

namespace vt { namespace messaging {

/**
 * \struct SendInfo
 *
 * \brief Returned from a data send to be used to receive the data
 */
struct SendInfo {

  /**
   * \internal
   * \brief Construct a SendInfo
   *
   * \param[in] in_event the send event (parent event if multiple sends)
   * \param[in] in_tag the MPI tag it was sent with
   * \param[in] in_nchunks the number of send chunks for the entire payload
   */
  SendInfo(EventType in_event, TagType in_tag, int in_nchunks)
    : event(in_event),
      tag(in_tag),
      nchunks(in_nchunks)
  { }

  /**
   * \brief Get the send event (either an MPI event or a parent event containing
   * multiple MPI events)
   *
   * \return the send event
   */
  EventType getEvent() const { return event; }

  /**
   * \brief The MPI tag that it was sent with
   *
   * \return the tag
   */
  TagType getTag() const { return tag; }

  /**
   * \brief The number of Isend chunks that make up the entire payload
   *
   * \return the number of chunks
   */
  int getNumChunks() const { return nchunks; }

private:
  EventType const event = no_event; /**< The event for the send */
  TagType const tag = no_tag;       /**< The MPI tag for the send */
  int const nchunks = 0;            /**< The number of send chunks to receive */
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_SEND_INFO_H*/
