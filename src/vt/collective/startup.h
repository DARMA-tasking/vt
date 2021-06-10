/*
//@HEADER
// *****************************************************************************
//
//                                  startup.h
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

#if !defined INCLUDED_VT_COLLECTIVE_STARTUP_H
#define INCLUDED_VT_COLLECTIVE_STARTUP_H

#include "vt/config.h"
#include "vt/runtime/runtime_headers.h"

namespace vt {

/**
 * \brief Initialize the VT runtime (with threads)
 *
 * \param[in] argc argc from main to parse arguments and remove VT ones
 * \param[in] argv argv from main to parse arguments and remove VT ones
 * \param[in] num_workers the number of workers (threads)
 * \param[in] is_interop whether running in interop (no MPI init)
 * \param[in] comm the MPI communicator
 * \param[in] delay_startup_banner whether to delay printing the startup banner
 *            until the VT scheduler runs the first time
 *
 * \return the runtime pointer
 */
RuntimePtrType initialize(
  int& argc, char**& argv, WorkerCountType const num_workers,
  bool is_interop = false, MPI_Comm* comm = nullptr,
  bool delay_startup_banner = false
);

/**
 * \brief Initialize the VT runtime
 *
 * \param[in] argc argc from main to parse arguments and remove VT ones
 * \param[in] argv argv from main to parse arguments and remove VT ones
 * \param[in] comm the MPI communicator (optional)
 * \param[in] delay_startup_banner whether to delay printing the startup banner
 *            until the VT scheduler runs the first time
 *
 * \return the runtime pointer
 */
RuntimePtrType initialize(
  int& argc, char**& argv, MPI_Comm* comm = nullptr,
  bool delay_startup_banner = false
);

/**
 * \brief Initialize the VT runtime
 *
 * \param[in] comm the MPI communicator (optional)
 * \param[in] delay_startup_banner whether to delay printing the startup banner
 *            until the VT scheduler runs the first time
 *
 * \return the runtime pointer
 */
RuntimePtrType initialize(
  MPI_Comm* comm = nullptr, bool delay_startup_banner = false
);

/**
 * \brief Finalize the VT runtime
 *
 * \param[in] in_rt the runtime pointer
 */
void finalize(RuntimePtrType in_rt);

/**
 * \brief Finalize the VT runtime (the currently active one)
 */
void finalize();

} /* end namespace vt */

#endif /*INCLUDED_VT_COLLECTIVE_STARTUP_H*/
