/*
//@HEADER
// *****************************************************************************
//
//                                 msg_rdma.cc
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
#include "vt/messaging/rdma/msg_rdma.h"
#include "vt/objgroup/manager.h"
#include "vt/rdmahandle/manager.h"

namespace vt { namespace messaging { namespace rdma {

MsgRDMA::MsgRDMA()
  : current_size_(64*1024)
{ }

/*static*/ std::unique_ptr<MsgRDMA> MsgRDMA::construct() {
  auto ptr = std::make_unique<MsgRDMA>();
  auto proxy = theObjGroup()->makeCollective<MsgRDMA>(ptr.get());
  proxy.get()->setProxy(proxy);
  return ptr;
}

void MsgRDMA::startup() {
  handle_ = proxy_.makeHandleRDMA<char>(current_size_, true);
}

std::size_t MsgRDMA::writeBytesForGet(char* ptr, std::size_t len) {
  auto const this_node = theContext()->getNode();
  int const offset = cur_offset_;
  handle_.put(this_node, ptr, len, offset, vt::rdma::Lock::Exclusive);
  cur_offset_ += static_cast<int>(len);
  return offset;
}

void MsgRDMA::getBytes(NodeType get_node, char* ptr, int offset, std::size_t len) {
  auto req = handle_.rget(get_node, ptr, len, offset, vt::rdma::Lock::Shared);
  req.addAction([]{
    fmt::print("done\n");
  });
  reqs_.emplace_back(std::move(req));
}

int MsgRDMA::progress() {
  int num_completed = 0;
  for (auto iter = reqs_.begin(); iter != reqs_.end();) {
    if (iter->test()) {
      // Call wait to trigger the actions
      iter->wait();
      iter = reqs_.erase(iter);
      num_completed++;
    } else {
      ++iter;
    }
  }
  return num_completed;
}

}}} /* end namespace vt::messaging::rdma */
