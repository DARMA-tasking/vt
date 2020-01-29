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

/** \file */

/**
 * \brief Intialization with parsing of command-line parameters
 *
 * \param[in,out] argc number of command-line parameters
 * \param[in,out] argv array of command-line parameters
 * \param[in] is_interop (Default value = false)
 * \param[in] num_workers (Default value = no_workers)
 * \param[in] comm Pointer to MPI_Comm (Default value = nullptr)
 *
 * \return RuntimePtrType Pointer to the new runtime object
 *
 */
RuntimePtrType initialize(
  int& argc, char**& argv, WorkerCountType const num_workers,
  bool is_interop = false, MPI_Comm* comm = nullptr
);

/**
 * \brief Intialization with parsing of command-line parameters
 *
 * \param[in,out] argc number of command-line parameters
 * \param[in,out] argv array of command-line parameters
 * \param[in] comm Pointer to MPI_Comm (Default value = nullptr)
 *
 * \return RuntimePtrType Pointer to the new runtime object
 *
 */
RuntimePtrType initialize(int& argc, char**& argv, MPI_Comm* comm = nullptr);

/**
 * \brief Intialization
 *
 * \param[in] comm Pointer to MPI_Comm (Default value = nullptr)
 *
 * \return RuntimePtrType Pointer to the new runtime object
 *
 */
RuntimePtrType initialize(MPI_Comm* comm = nullptr);

/**
 * \brief Allocate routine to allow for two-step initialization
 *
 * This routine must be called after initializing the runtime object.
 * It allows to parse and resolve parameters from command line
 * and from an input file.
 *
 * \param[in] is_interop (Default value = false)
 * \param[in] num_workers (Default value = no_workers)
 * \param[in] comm Pointer to MPI_Comm (Default value = nullptr)
 *
 * \return RuntimePtrType Pointer to the new runtime object
 *
 */
RuntimePtrType allocate(
  bool is_interop = false,
  WorkerCountType const num_workers = no_workers,
  MPI_Comm* comm = nullptr
);

/**
 * \brief Finalize
 *
 * This routine ...
 *
 * \param[in] in_rt
 *
 */
void finalize(RuntimePtrType in_rt);

/**
 * \brief Finalize
 *
 * This routine ...
 *
 */
void finalize();

} /* end namespace vt */

#endif /*INCLUDED_VT_COLLECTIVE_STARTUP_H*/
