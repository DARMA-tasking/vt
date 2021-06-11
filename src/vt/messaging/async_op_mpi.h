/*
//@HEADER
// *****************************************************************************
//
//                                async_op_mpi.h
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

#if !defined INCLUDED_VT_MESSAGING_ASYNC_OP_MPI_H
#define INCLUDED_VT_MESSAGING_ASYNC_OP_MPI_H

#include "vt/messaging/async_op.h"

namespace vt { namespace messaging {

/**
 * \struct AsyncOpMPI
 *
 * \brief An asynchronous MPI request that VT can poll on until completion.
 */
struct AsyncOpMPI : AsyncOp {

  /**
   * \brief Construct with a \c MPI_Request
   *
   * \param[in] in_req the mpi request
   * \param[in] in_cont the continuation to execute when complete (optional)
   */
  explicit AsyncOpMPI(MPI_Request in_req, ActionType in_cont = nullptr)
    : req_(in_req),
      cont_(in_cont)
  { }

  /**
   * \brief Poll completion of the \c MPI_Request
   *
   * \return whether the \c MPI_Request is complete or not
   */
  bool poll() override {
    VT_ALLOW_MPI_CALLS; // MPI_Test
    int flag = 0;
    MPI_Status stat;
    MPI_Test(&req_, &flag, &stat);
    return flag;
  }

  /**
   * \brief Cont continuation after completion
   */
  void done() override {
    if (cont_) {
      cont_();
    }
  }

private:
  MPI_Request req_ = MPI_REQUEST_NULL; /**< the MPI request */
  ActionType cont_ = nullptr;          /**< the continuation to execute */
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_ASYNC_OP_MPI_H*/
