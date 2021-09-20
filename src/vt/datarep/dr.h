/*
//@HEADER
// *****************************************************************************
//
//                                     dr.h
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

#if !defined INCLUDED_VT_DATAREP_DR_H
#define INCLUDED_VT_DATAREP_DR_H

#include "vt/config.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/messaging/message/message.h"
#include "vt/messaging/message/message_serialize.h"
#include "vt/datarep/reader.h"
#include "vt/datarep/handle.h"
#include "vt/datarep/datastore.h"

#include <memory>
#include <unordered_map>

namespace vt { namespace datarep {

struct DataReplicator : runtime::component::Component<DataReplicator> {

  std::string name() override { return "DataReplicator"; }

  static std::unique_ptr<DataReplicator> construct();

  void setup(ObjGroupProxyType in_proxy) {
    proxy_ = in_proxy;
  }

  template <typename T>
  Reader<T> makeReader(DataRepIDType handle);

  template <typename T>
  DR<T> makeHandle(T&& data);

  template <typename T>
  void migrateHandle(DR<T>& handle, vt::NodeType migrated_to);

  template <typename T>
  DataRepIDType registerHandle();

  template <typename T>
  void unregisterHandle(DataRepIDType handle_id);

  template <typename T>
  struct DataRequestMsg : vt::Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_prohibited();

    DataRequestMsg(NodeType in_requestor, DataRepIDType in_handle_id)
      : requestor_(in_requestor),
        handle_id_(in_handle_id)
    { }

    NodeType requestor_ = uninitialized_destination;
    DataRepIDType handle_id_ = no_datarep;
  };

  template <typename T>
  struct DataResponseMsg : vt::Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_if_needed_by_parent_or_type1(T);

    DataResponseMsg() = default; // for serializer
    DataResponseMsg(DataRepIDType in_handle_id, T const& data)
      : handle_id_(in_handle_id),
        data_(std::make_unique<T>(data))
    { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | handle_id_;
      s | data_;
    }

    DataRepIDType handle_id_ = no_datarep;
    std::unique_ptr<T> data_;
  };

  template <typename T>
  bool requestData(DataRepIDType handle_id, bool* ready_ptr);

  template <typename T>
  T const& getDataRef(DataRepIDType handle_id) const;

private:

  template <typename T>
  void dataIncomingHandler(DataResponseMsg<T>* msg);

  template <typename T>
  void dataRequestHandler(DataRequestMsg<T>* msg);

  NodeType getHomeNode(DataRepIDType handle_id) const {
    return handle_id >> 48;
  }

private:
  DataRepIDType identifier_ = 1;
  ObjGroupProxyType proxy_ = no_obj_group;
  std::unordered_map<DataRepIDType, std::unique_ptr<DataStoreBase>> local_store_;
  std::unordered_map<DataRepIDType, std::vector<bool*>> waiting_;
};

}} /* end namespace vt::datarep */

namespace vt {

extern datarep::DataReplicator* theDR();

} // end namespace vt

#include "vt/datarep/dr.impl.h"
#include "vt/datarep/reader.impl.h"
#include "vt/datarep/handle.impl.h"

#endif /*INCLUDED_VT_DATAREP_DR_H*/
