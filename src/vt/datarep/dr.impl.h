/*
//@HEADER
// *****************************************************************************
//
//                                  dr.impl.h
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

#if !defined INCLUDED_VT_DATAREP_DR_IMPL_H
#define INCLUDED_VT_DATAREP_DR_IMPL_H

#include "vt/topos/location/manager.h"
#include "vt/datarep/dr.h"
#include "vt/datarep/msg.h"
#include "vt/datarep/datastore.h"
#include "vt/objgroup/manager.h"

namespace vt { namespace datarep {

template <typename T>
Reader<T> DataReplicator::makeReader(DataRepIDType handle) {
  vt_debug_print(
    normal, gen,
    "makeReader: handle_id={}\n",
    handle
  );
  return Reader<T>{typename Reader<T>::READER_CONSTRUCT_TAG{}, handle};
}

template <typename T>
DR<T> DataReplicator::makeHandle() {
  auto const handle_id = registerHandle<T>();
  vt_debug_print(
    normal, gen,
    "makeHandle: handle_id={}\n",
    handle_id
  );
  theLocMan()->dataRep->registerEntity(handle_id, theContext()->getNode());
  return DR<T>{typename DR<T>::DR_TAG_CONSTRUCT{}, handle_id};
}

template <typename T, typename ProxyType>
DR<T, typename ProxyType::IndexType> DataReplicator::makeIndexedHandle(
  ProxyType proxy, TagType tag
) {
  using IndexType = typename ProxyType::IndexType;

  auto proxy_bits = proxy.getCollectionProxy();
  auto index = proxy.getElementProxy().getIndex();
  vt_debug_print(
    normal, gen,
    "makeIndexedHandle: proxy={:x}, index={}, tag={}\n",
    proxy_bits, index, tag
  );

  using TagType = typename DR<T, IndexType>::DR_TAG_CONSTRUCT;
  return DR<T, IndexType>{TagType{}, proxy_bits, index, tag};
}

template <typename T, typename IndexT>
void DataReplicator::publishVersion(
  detail::DR_Base<IndexT> dr_base, DataVersionType version, T&& data
) {
  auto handle = dr_base.getHandleID();
  auto tag = dr_base.getTag();
  auto idx = dr_base.getIndex();
  auto id = detail::DataIdentifier{handle, tag};
  vt_debug_print(
    normal, gen,
    "publishVersion handle_id={}, version={}\n",
    handle, version
  );
  auto iter = local_store_.find(id);
  if (iter == local_store_.end()) {
    local_store_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(id),
      std::forward_as_tuple(
        std::make_unique<DataStore<T, IndexT>>(
          true, idx, version, std::make_shared<T>(std::forward<T>(data))
        )
      )
    );
  } else {
    auto ds = static_cast<DataStore<T, IndexT>*>(iter->second.get());
    ds->publishVersion(idx, version, std::make_shared<T>(std::forward<T>(data)));
  }
}

template <typename T, typename IndexT>
void DataReplicator::unpublishVersion(
  detail::DR_Base<IndexT> dr_base, DataVersionType version
) {
  auto handle = dr_base.getHandleID();
  auto tag = dr_base.getTag();
  auto idx = dr_base.getIndex();
  auto id = detail::DataIdentifier{handle, tag};
  vt_debug_print(
    normal, gen,
    "unpublishVersion handle_id={}, version={}\n",
    handle, version
  );
  auto iter = local_store_.find(id);
  vtAssert(iter != local_store_.end(), "Handle must exist");
  auto ds = static_cast<DataStore<T, IndexT>*>(iter->second.get());
  ds->unpublishVersion(idx, version);
}

template <typename T>
void DataReplicator::migrateHandle(DR<T>& handle, vt::NodeType migrated_to) {
  theLocMan()->dataRep->entityEmigrated(handle.handle_, migrated_to);
}

template <typename T>
DataRepIDType DataReplicator::registerHandle() {
  // generate the ID here
  auto id = identifier_++;
  auto node = theContext()->getNode();
  return DataRepIDType((static_cast<uint64_t>(node) << 48) | id);
}

template <typename T>
void DataReplicator::unregisterHandle(DataRepIDType handle_id) {
  // unregister the ID here
}

template <typename T, typename IndexT>
bool DataReplicator::requestData(
  detail::DR_Base<IndexT> dr_base, DataVersionType version,
  detail::ReaderBase* reader
) {
  auto const handle = dr_base.getHandleID();
  auto const tag = dr_base.getTag();
  auto const idx = dr_base.getIndex();
  auto const id = detail::DataIdentifier{handle, tag};
  auto iter = local_store_.find(id);
  if (
    iter != local_store_.end() and
    static_cast<DataStore<T, IndexT>*>(iter->second.get())->hasVersion(
      idx, version
    )
  ) {
    vt_debug_print(
      normal, gen,
      "requestData: handle_id={} found locally\n", handle
    );
    // found in cache
    // deliver to the Reader
    // nothing to do data is here
    if (reader) {
      auto ds = static_cast<DataStore<T, IndexT>*>(iter->second.get());
      auto tr = static_cast<Reader<T, IndexT>*>(reader);
      tr->data_ = ds->getSharedPtr(idx, version);
      tr->ready_ = true;
    }
    return true;
  } else {
    vt_debug_print(
      normal, gen,
      "requestData: handle_id={} remote request\n", handle
    );
    waiting_[handle].push_back(reader);

    using MsgType = detail::DataRequestMsg<T, IndexT>;
    auto const this_node = theContext()->getNode();
    auto msg = makeMessage<MsgType>(dr_base, this_node, version);
    theLocMan()->dataRep->routeMsgHandler<MsgType, staticRequestHandler<T, IndexT>>(
      handle, getHomeNode(handle), msg.get()
    );
    return false;
  }
}

template <typename T, typename IndexT>
/*static*/ void DataReplicator::staticRequestHandler(
  detail::DataRequestMsg<T, IndexT>* msg
) {
  auto proxy = objgroup::proxy::Proxy<DataReplicator>{theDR()->proxy_};
  auto loc = theContext()->getNode();
  proxy[loc].invoke<
    decltype(&DataReplicator::dataRequestHandler<T, IndexT>),
    &DataReplicator::dataRequestHandler<T, IndexT>
  >(msg);
}

template <typename T, typename IndexT>
T const& DataReplicator::getDataRef(
  detail::DR_Base<IndexT> dr_base, DataVersionType version
) const {
  auto const handle = dr_base.getHandleID();
  auto const tag = dr_base.getTag();
  auto const idx = dr_base.getIndex();
  auto const id = detail::DataIdentifier{handle, tag};
  auto iter = local_store_.find(id);
  vtAssert(iter != local_store_.end(), "Must exist at this point");
  vt_debug_print(
    normal, gen,
    "getDataRef: handle_id={}, version={}, idx={}\n", handle, version, idx
  );
  auto ds = static_cast<DataStore<T, IndexT>*>(iter->second.get());
  return *static_cast<T const*>(ds->get(idx, version));
}

template <typename T, typename IndexT>
void DataReplicator::dataIncomingHandler(
  detail::DataResponseMsg<T, IndexT>* msg
) {
  auto const dr_base = msg->dr_base_;
  auto const handle = dr_base.getHandleID();
  auto const tag = dr_base.getTag();
  auto const idx = dr_base.getIndex();
  auto const id = detail::DataIdentifier{handle, tag};
  auto const version = msg->version_;
  vt_debug_print(
    normal, gen,
    "dataIncomingHandler: han_id={}, version={}\n", handle, version
  );
  auto iter = local_store_.find(id);
  vtAssert(iter == local_store_.end(), "Must not exist if requested");
  local_store_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(id),
    std::forward_as_tuple(
      std::make_unique<DataStore<T, IndexT>>(
        false, idx, version, std::make_shared<T>(std::move(*msg->data_.get()))
      )
    )
  );
  auto ds = static_cast<DataStore<T, IndexT>*>(
    local_store_.find(id)->second.get()
  );
  // Inform that the data is ready
  auto witer = waiting_.find(id);
  if (witer != waiting_.end()) {
    for (auto&& elm : witer->second) {
      auto tr = static_cast<Reader<T>*>(elm);
      tr->data_ = ds->getSharedPtr(idx, version);
      tr->ready_ = true;
    }
    waiting_.erase(witer);
  }
}

template <typename T, typename IndexT>
void DataReplicator::dataRequestHandler(detail::DataRequestMsg<T, IndexT>* msg) {
  auto const dr_base = msg->dr_base_;
  auto const handle = dr_base.getHandleID();
  auto const requestor = msg->requestor_;
  auto const version = msg->version_;
  auto const found = requestData<T, IndexT>(dr_base, version, nullptr);
  vt_debug_print(
    normal, gen,
    "dataRequestHandle: handle={}, requestor={}, version={}, found={}\n",
    handle, requestor, version, found
  );
  if (found) {
    auto proxy = objgroup::proxy::Proxy<DataReplicator>{proxy_};
    proxy[requestor].template send<
      detail::DataResponseMsg<T, IndexT>,
      &DataReplicator::dataIncomingHandler<T, IndexT>
    >(dr_base, getDataRef<T>(dr_base, version), version);
  }
}

}} /* end namespace vt::datarep */

#endif /*INCLUDED_VT_DATAREP_DR_IMPL_H*/
