/*
//@HEADER
// *****************************************************************************
//
//                                mpi_max_tag.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/utils/mpi_limits/mpi_max_tag.h"

namespace vt { namespace util { namespace mpi_limits {

/*static*/ int MPI_Attr::getMaxTag() {

  if (max_tag_ != 0) {
    return max_tag_;
  }

  void* p = nullptr;
  int flag = 0;
  MPI_Comm_get_attr(MPI_COMM_WORLD, MPI_TAG_UB, &p, &flag);

  if (not flag) {
    vtAbort("Failed to obtain MPI_TAG_UB attribute");
  } else {
    max_tag_ = *static_cast<int*>(p);
    vtAssert(max_tag_ >= 32767, "MPI standard requires tag >= 32767");
  }

  return max_tag_;
}

/*static*/ std::tuple<int, int> MPI_Attr::getVersion() {
  if (version_ == 0) {
    MPI_Get_version(&version_, &subversion_);
  }

  return std::make_tuple(version_, subversion_);
}

/*static*/ int MPI_Attr::max_tag_ = 0;

/*static*/ int MPI_Attr::version_ = 0;

/*static*/ int MPI_Attr::subversion_ = 0;

}}} /* end namespace vt::util::mpi_limits */
