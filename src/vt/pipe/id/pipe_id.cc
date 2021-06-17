/*
//@HEADER
// *****************************************************************************
//
//                                  pipe_id.cc
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
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/utils/bits/bits_common.h"

namespace vt { namespace pipe {

/*static*/ PipeType PipeIDBuilder::createPipeID(
  PipeIDType const& id, NodeType const& node, bool const& is_send_back,
  bool const& is_persist
) {
  PipeType new_pipe = 0;

  setIsSendback(new_pipe, is_send_back);
  setIsPersist(new_pipe, is_persist);
  setNode(new_pipe, node);
  setID(new_pipe, id);

  return new_pipe;
}

/*static*/ void PipeIDBuilder::setIsSendback(
  PipeType& pipe, bool const& is_send_back
) {
  BitPackerType::boolSetField<ePipeIDBits::Sendback>(pipe, is_send_back);
}

/*static*/ void PipeIDBuilder::setIsPersist(
  PipeType& pipe, bool const& is_persist
) {
  BitPackerType::boolSetField<ePipeIDBits::Persist>(pipe, is_persist);
}

/*static*/ void PipeIDBuilder::setNode(
  PipeType& pipe, NodeType const& node
) {
  BitPackerType::setField<ePipeIDBits::Node, pipe_node_num_bits>(pipe, node);
}

/*static*/ void PipeIDBuilder::setID(
  PipeType& pipe, PipeIDType const& id
) {
  BitPackerType::setField<ePipeIDBits::ID, pipe_id_num_bits>(pipe, id);
}

/*static*/ bool PipeIDBuilder::isSendback(PipeType const& pipe) {
  return BitPackerType::boolGetField<ePipeIDBits::Sendback>(pipe);
}

/*static*/ bool PipeIDBuilder::isPersist(PipeType const& pipe) {
  return BitPackerType::boolGetField<ePipeIDBits::Persist>(pipe);
}

/*static*/ NodeType PipeIDBuilder::getNode(PipeType const& pipe) {
  return BitPackerType::getField<
    ePipeIDBits::Node, pipe_node_num_bits, NodeType
  >(pipe);
}

/*static*/ PipeIDType PipeIDBuilder::getPipeID(PipeType const& pipe) {
  return BitPackerType::getField<
    ePipeIDBits::ID, pipe_id_num_bits, PipeIDType
  >(pipe);
}

}} /* end namespace vt::pipe */
