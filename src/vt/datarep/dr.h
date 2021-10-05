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

namespace vt { namespace datarep { namespace detail {

template <typename T, typename IndexT>
struct DataResponseMsg;

template <typename T, typename IndexT>
struct DataRequestMsg;

struct DataIdentifier {
  DataIdentifier(DataRepIDType in_handle_id, TagType in_tag = no_tag)
    : handle_id_(in_handle_id),
      tag_(in_tag)
  { }

  bool operator==(DataIdentifier const& other) const {
    return other.handle_id_ == handle_id_ and other.tag_ == tag_;
  }

  DataRepIDType handle_id_ = no_datarep;
  TagType tag_ = no_tag;
};

}}} /* end namespace vt::datarep::detail */

namespace std {

template <>
struct hash<vt::datarep::detail::DataIdentifier> {
  size_t operator()(vt::datarep::detail::DataIdentifier const& in) const {
    return std::hash<uint64_t>()(in.handle_id_ ^ in.tag_);
  }
};

} /* end namespace std */

namespace vt { namespace datarep {

struct DataReplicator : runtime::component::Component<DataReplicator> {
  using DataIdentifier = detail::DataIdentifier;
  using ReaderBase = detail::ReaderBase;

  std::string name() override { return "DataReplicator"; }

  static std::unique_ptr<DataReplicator> construct();

  void setup(ObjGroupProxyType in_proxy) {
    proxy_ = in_proxy;
  }

  template <typename T>
  DataRepIDType registerHandle();

  template <typename T>
  void unregisterHandle(DataRepIDType handle_id);

  template <typename T>
  DR<T> makeHandle();

  template <typename T, typename ProxyType>
  DR<T, typename ProxyType::IndexType> makeIndexedHandle(
    ProxyType proxy, TagType tag = no_tag
  );

  template <typename T>
  Reader<T> makeReader(DataRepIDType handle);

  template <typename T, typename ProxyType>
  Reader<T, typename ProxyType::IndexType> makeIndexedReader(
    ProxyType proxy, TagType tag = no_tag
  );

  template <typename T, typename IndexT>
  void publishVersion(
    detail::DR_Base<IndexT> dr_base, DataVersionType version, T&& data
  );

  template <typename T, typename IndexT>
  void unpublishVersion(detail::DR_Base<IndexT> dr_base, DataVersionType version);

  template <typename T>
  void migrateHandle(DR<T>& handle, vt::NodeType migrated_to);

  template <typename T, typename IndexT>
  bool requestData(
    detail::DR_Base<IndexT> dr_base, DataVersionType version, ReaderBase* reader
  );

private:
  template <typename T, typename IndexT>
  T const& getDataRef(detail::DR_Base<IndexT> dr_base, DataVersionType version) const;

  template <typename T, typename IndexT>
  static void staticRequestHandler(detail::DataRequestMsg<T, IndexT>* msg);

  template <typename T, typename IndexT>
  void dataIncomingHandler(detail::DataResponseMsg<T, IndexT>* msg);

  template <typename T, typename IndexT>
  void dataRequestHandler(detail::DataRequestMsg<T, IndexT>* msg);

  NodeType getHomeNode(DataRepIDType handle_id) const {
    return handle_id >> 48;
  }

private:
  DataRepIDType identifier_ = 1;
  ObjGroupProxyType proxy_ = no_obj_group;
  std::unordered_map<DataIdentifier, std::unique_ptr<DataStoreBase>> local_store_;
  std::unordered_map<DataIdentifier, std::vector<ReaderBase*>> waiting_;
};

}} /* end namespace vt::datarep */

namespace vt {

extern datarep::DataReplicator* theDR();

} // end namespace vt

#include "vt/datarep/dr.impl.h"
#include "vt/datarep/reader.impl.h"
#include "vt/datarep/handle.impl.h"

#endif /*INCLUDED_VT_DATAREP_DR_H*/
