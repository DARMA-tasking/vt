/*
//@HEADER
// *****************************************************************************
//
//                              multitag_holder.h
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

#if !defined INCLUDED_VT_MESSAGING_MULTITAG_HOLDER_H
#define INCLUDED_VT_MESSAGING_MULTITAG_HOLDER_H

#include <memory>

namespace vt { namespace messaging {

struct InProgressIRecv;

/**
 * \struct MultiTagHolder
 *
 * \internal \brief Holds a irecv holder along with a counter for when all
 * receives fill the buffer from split sends arriving
 */
struct MultiTagHolder {
  using IRecvPtrType = std::unique_ptr<InProgressIRecv>;

  /**
   * \brief Construct the holder
   *
   * \param[in] in_cnt the number of sends/recvs it was broken into
   * \param[in] in_irecv the in-progress irecv holder with the buffer
   */
  MultiTagHolder(int in_cnt, IRecvPtrType in_irecv)
    : cnt(in_cnt),
      irecv(std::move(in_irecv))
  { }

  /**
   * \brief Indicate that a recv has arrived
   */
  void decrement() { cnt--; }

  /**
   * \brief Check if the counter is zero indicating all that receives have
   * arrived
   *
   * \return whether it is ready
   */
  bool ready() const { return cnt == 0; }

  /**
   * \brief Get the in-progress irecv holder
   *
   * \return passes ownership out of this container
   */
  IRecvPtrType getIrecv() { return std::move(irecv); }

private:
  int cnt = 0;                  /**< The number of split sends/recvs */
  IRecvPtrType irecv = nullptr; /**< The in-progress irecv holder */
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_MULTITAG_HOLDER_H*/
