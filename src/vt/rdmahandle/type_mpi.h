/*
//@HEADER
// *****************************************************************************
//
//                                  type_mpi.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_TYPE_MPI_H
#define INCLUDED_VT_RDMAHANDLE_TYPE_MPI_H

#include "vt/config.h"

namespace vt { namespace rdma {

template <typename T>
struct TypeMPI {
  static auto getType() { return MPI_BYTE; }
  static auto getTypeStr() { return "MPI_BYTE"; }
};

template <>
struct TypeMPI<char> {
  static auto getType() { return MPI_SIGNED_CHAR; }
  static auto getTypeStr() { return "MPI_SIGNED_CHAR"; }
};

template <>
struct TypeMPI<wchar_t> {
  static auto getType() { return MPI_WCHAR; }
  static auto getTypeStr() { return "MPI_WCHAR"; }
};

template <>
struct TypeMPI<double> {
  static auto getType() { return MPI_DOUBLE; }
  static auto getTypeStr() { return "MPI_DOUBLE"; }
};

template <>
struct TypeMPI<long double> {
  static auto getType() { return MPI_LONG_DOUBLE; }
  static auto getTypeStr() { return "MPI_LONG_DOUBLE"; }
};

template <>
struct TypeMPI<float> {
  static auto getType() { return MPI_FLOAT; }
  static auto getTypeStr() { return "MPI_FLOAT"; }
};

template <>
struct TypeMPI<int8_t> {
  static auto getType() { return MPI_INT8_T; }
  static auto getTypeStr() { return "MPI_INT8_T"; }
};

template <>
struct TypeMPI<int16_t> {
  static auto getType() { return MPI_INT16_T; }
  static auto getTypeStr() { return "MPI_INT16_T"; }
};

template <>
struct TypeMPI<int32_t> {
  static auto getType() { return MPI_INT32_T; }
  static auto getTypeStr() { return "MPI_INT32_T"; }
};

template <>
struct TypeMPI<int64_t> {
  static auto getType() { return MPI_INT64_T; }
  static auto getTypeStr() { return "MPI_INT64_T"; }
};

template <>
struct TypeMPI<uint8_t> {
  static auto getType() { return MPI_UINT8_T; }
  static auto getTypeStr() { return "MPI_UINT8_T"; }
};

template <>
struct TypeMPI<uint16_t> {
  static auto getType() { return MPI_UINT16_T; }
  static auto getTypeStr() { return "MPI_UINT16_T"; }
};

template <>
struct TypeMPI<uint32_t> {
  static auto getType() { return MPI_UINT32_T; }
  static auto getTypeStr() { return "MPI_UINT32_T"; }
};

template <>
struct TypeMPI<uint64_t> {
  static auto getType() { return MPI_UINT64_T; }
  static auto getTypeStr() { return "MPI_UINT64_T"; }
};

template <>
struct TypeMPI<std::size_t> {
  // @todo: this might not be true on some platforms!
  static auto getType() { return MPI_UINT64_T; }
  static auto getTypeStr() { return "MPI_UINT64_T"; }
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_TYPE_MPI_H*/
