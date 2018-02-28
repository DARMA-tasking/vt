
#if !defined INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H
#define INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H

#include "config.h"
#include "messaging/message.h"
#include "vrt/proxy/proxy_collection.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct MigrateMsg : ::vt::PayloadMessage {

  MigrateMsg() = default;
  MigrateMsg(
    VrtElmProxy<IndexT> const& in_elm_proxy, NodeType const& in_from,
    NodeType const& in_to
  ) : elm_proxy_(in_elm_proxy), from_(in_from), to_(in_to)
  { }

  VrtElmProxy<IndexT> getElementProxy() const { return elm_proxy_; }
  NodeType getFromNode() const { return from_; }
  NodeType getToNode() const { return to_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | elm_proxy_ | from_ | to_;
  }

private:
  VrtElmProxy<IndexT> elm_proxy_;
  NodeType from_ = uninitialized_destination;
  NodeType to_ = uninitialized_destination;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H*/
