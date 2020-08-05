/*
//@HEADER
// *****************************************************************************
//
//                                mpi_access.h
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

#if !defined INCLUDED_RUNTIME_MPI_ACCESS_H
#define INCLUDED_RUNTIME_MPI_ACCESS_H

#if vt_check_enabled(mpi_access_guards)
#define VT_ALLOW_MPI_CALLS vt::runtime::ScopedMPIAccess _vt_allow_scoped_mpi{};
#else
#define VT_ALLOW_MPI_CALLS
#endif // vt_check_enabled(mpi_access_guards)

#if vt_check_enabled(mpi_access_guards)
namespace vt { namespace runtime {

  /**
   * \internal
   * \brief RAII control to allow access to MPI_* functions.
   *
   * This type is expected to be used with relevant scopes.
   * Once activated, code can call into MPI_* functions
   */
  struct ScopedMPIAccess {
    ScopedMPIAccess();
    ScopedMPIAccess(ScopedMPIAccess const&) = delete;
    ScopedMPIAccess(ScopedMPIAccess&&) = delete;
    ~ScopedMPIAccess();

    /**
     * \brief Enable/disable is MPI_* functions are prohibited from use.
     *
     * Explicitly granting access overrides the default.
     */
    static void prohibitByDefault(bool prohibit);

    /**
     * \brief Returns true if (scoped) access has been granted.
     */
    static bool isExplicitlyGranted();

    /**
     * \brief Returns true if MPI calls are allowed.
     */
    static bool mpiCallsAllowed();

    /**
     * \brief Returns true if MPI calls should be traced.
     */
    static bool mpiCallsTraced();

    static int grants_;
    static bool default_prohibit_;
  };

}} // end namespace vt::runtime
#endif // vt_check_enabled(mpi_access_guards)

#endif /* INCLUDED_RUNTIME_MPI_ACCESS_H */
