/*
//@HEADER
// *****************************************************************************
//
//                                  startup.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/collective/startup_config.h"
#include <mpi.h>

#include <memory>

namespace vt {

/**
 * \brief Preconfigure VT with argc/argv. This will remove all VT arguments and
 * create a \c StartupConfig for VT that should be passed to \c
 * initializePreconfigured. Optionally, one many specify an MPI communicator to
 * use (otherwise, it defaults to \c MPI_COMM_WORLD).
 *
 * \note MPI must be initialized to call this function because if an error
 * occurs it uses MPI rank to limit how many times the error text gets printed.
 *
 * \param[in] argc argc (modifies it to remove VT arguments)
 * \param[in] argv argv (modifies it to remove VT arguments)
 *
 * \return the \c StartupConfig to pass to VT
 */
std::unique_ptr<StartupConfig> preconfigure(
  int& argc, char**& argv
);

/**
 * \brief Initialize VT after it has been preconfigured
 *
 * \param[in] startup_config the arg config
 * \param[in] comm optional communicator
 * \param[in] app_config (optional) base VT configuration to use
 * \param[in] print_startup_banner (optional) whether to print startup banner
 *
 * \return the runtime pointer
 */
RuntimePtrType initializePreconfigured(
  std::unique_ptr<StartupConfig> startup_config,
  MPI_Comm* comm = nullptr,
  arguments::AppConfig const* app_config = nullptr,
  bool print_startup_banner = true
);

/**
 * \brief Initialize VT
 *
 * \param[in] argc (to modify)
 * \param[in] argv (to modify)
 * \param[in] comm optional communicator
 * \param[in] app_config (optional) base VT configuration to use
 * \param[in] print_startup_banner (optional) whether to print startup banner
 *
 * \return the runtime pointer
 */
RuntimePtrType initialize(
  int& argc, char**& argv, MPI_Comm* comm = nullptr,
  arguments::AppConfig const* appConfig = nullptr,
  bool print_startup_banner = true
);

RuntimePtrType initialize(MPI_Comm* comm = nullptr);

RuntimePtrType initialize(
  int& argc, char**& argv, arguments::AppConfig const* appConfig
);

void finalize(RuntimePtrType in_rt);

void finalize();

} /* end namespace vt */

#endif /*INCLUDED_VT_COLLECTIVE_STARTUP_H*/
