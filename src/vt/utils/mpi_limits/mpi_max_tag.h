/*
//@HEADER
// *****************************************************************************
//
//                                mpi_max_tag.h
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

#if !defined INCLUDED_VT_UTILS_MPI_LIMITS_MPI_MAX_TAG_H
#define INCLUDED_VT_UTILS_MPI_LIMITS_MPI_MAX_TAG_H

#include "vt/config.h"
#include "vt/context/context.h"

#include <mpi.h>

namespace vt { namespace util { namespace mpi_limits {

/**
 * \struct MPI_Attr
 *
 * \brief Get MPI attributes, including the MPI maximum value for a tag.
 */
struct MPI_Attr {

  /**
   * \brief Get the maximum tag for the current context communicator. VT must be
   * initialized for this to run correctly. This can vary across MPI
   * implementations, but the MPI standard dictates it be at least 32767.
   *
   * \return the max tag
   */
  static int getMaxTag();

  /**
   * \brief Get the MPI version and subversion.
   *
   * \return a tuple: (version, subversion)
   */
  static std::tuple<int, int> getVersion();

private:
  static int max_tag_;          /**< The cached max tag used. */
  static int version_;          /**< The cached MPI version */
  static int subversion_;       /**< The cached MPI sub-version */
};

}}} /* end namespace vt::util::mpi_limits */

namespace vt { namespace util {

using MPI_Attr = mpi_limits::MPI_Attr;

}} /* end namespace vt::util */

#endif /*INCLUDED_VT_UTILS_MPI_LIMITS_MPI_MAX_TAG_H*/
