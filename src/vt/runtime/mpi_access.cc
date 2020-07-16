/*
//@HEADER
// *****************************************************************************
//
//                               mpi_access.cc
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

#include "vt/config.h"
#include "vt/runtime/mpi_access.h"

#if vt_check_enabled(mpi_access_guards)
namespace vt { namespace runtime {

ScopedMPIAccess::ScopedMPIAccess() {
  grants_++;
  // Strong lock-down to prevent accidental nesting.
  vtAssert(
    grants_ <= 1,
    "ScopedMPIAccess should not be logically nested."
    " Open/close a new scope over the minimal MPI operation."
  );
}

ScopedMPIAccess::~ScopedMPIAccess() {
  assert(grants_ >= 1 && "Unbalanced access");
  grants_--;
}

/*static*/ void ScopedMPIAccess::prohibitByDefault(bool prohibit) {
  default_prohibit_ = prohibit;
}

/*static*/ bool ScopedMPIAccess::isExplicitlyGranted() {
  return grants_ > 0;
}

/*static*/ bool ScopedMPIAccess::mpiCallsAllowed() {
  return not default_prohibit_ or grants_ > 0;
}

/*static*/ int ScopedMPIAccess::grants_ = 0;
/*static*/ bool ScopedMPIAccess::default_prohibit_ = false;

}} // end namespace vt::runtime
#endif // vt_check_enabled(mpi_access_guards)
