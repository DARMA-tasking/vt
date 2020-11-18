/*
//@HEADER
// *****************************************************************************
//
//                                rdma_channel.h
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

#if !defined INCLUDED_RDMA_RDMA_CHANNEL_H
#define INCLUDED_RDMA_RDMA_CHANNEL_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/rdma/rdma_common.h"
#include "vt/rdma/channel/rdma_channel_lookup.h"
#include "vt/rdma/rdma_handle.h"

#include <mpi.h>

#include <unordered_map>

namespace vt { namespace rdma {

constexpr ByteType const rdma_elm_size = sizeof(char);
constexpr ByteType const rdma_empty_byte = 0;

struct Channel {
  using RDMA_HandleManagerType = HandleManager;
  using RDMA_TypeType = Type;
  using RDMA_GroupPosType = int;

  static constexpr RDMA_GroupPosType const no_group_pos = -1;

  Channel(
    RDMA_HandleType const& in_rdma_handle, RDMA_TypeType const& in_op_type,
    NodeType const& in_target, TagType const& in_channel_group_tag,
    NodeType const& in_non_target = uninitialized_destination,
    RDMA_PtrType const& in_ptr = nullptr, ByteType const& in_num_bytes = no_byte
  );

  virtual ~Channel();

  void initChannelGroup();
  void syncChannelLocal();
  void syncChannelGlobal();
  void lockChannelForOp();
  void unlockChannelForOp();
  void freeChannel();

  void writeDataToChannel(
    RDMA_PtrType const& ptr, ByteType const& ptr_num_bytes, ByteType const& offset
  );

  NodeType getTarget() const;
  NodeType getNonTarget() const;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | is_target_
      | initialized_
      | locked_
      | rdma_handle_
      | target_pos_
      | non_target_pos_
      | target_
      | non_target_
      | num_bytes_
      | ptr_
      | op_type_
      | channel_group_tag_
      | window_
      | channel_group_
      | channel_comm_;
  }

private:
  void initChannelWindow();

private:
  bool is_target_ = false;

  bool initialized_ = false, locked_ = false;
  RDMA_HandleType const rdma_handle_ = no_rdma_handle;
  RDMA_GroupPosType target_pos_ = no_group_pos;
  RDMA_GroupPosType non_target_pos_ = no_group_pos;
  NodeType target_ = uninitialized_destination;
  NodeType non_target_ = uninitialized_destination;
  ByteType num_bytes_ = no_byte;
  RDMA_PtrType ptr_ = no_rdma_ptr;
  RDMA_TypeType op_type_ = uninitialized_rdma_type;

  TagType channel_group_tag_ = no_tag;

  MPI_Win window_;
  MPI_Group channel_group_;
  MPI_Comm channel_comm_;
};

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_CHANNEL_H*/
