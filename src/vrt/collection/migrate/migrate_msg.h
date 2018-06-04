
#if !defined INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H
#define INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H

#include "config.h"
#include "messaging/message.h"
#include "vrt/proxy/proxy_collection.h"
#include "vrt/collection/collection_info.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct MigrateMsg final : ::vt::PayloadMessage {

  MigrateMsg() = default;
  MigrateMsg(
    VrtElmProxy<ColT, IndexT> const& in_elm_proxy, NodeType const& in_from,
    NodeType const& in_to, HandlerType const& in_map_fn, IndexT const& in_range
  ) : elm_proxy_(in_elm_proxy), from_(in_from), to_(in_to), map_fn_(in_map_fn),
      range_(in_range)
  { }

  VrtElmProxy<ColT, IndexT> getElementProxy() const { return elm_proxy_; }
  NodeType getFromNode() const { return from_; }
  NodeType getToNode() const { return to_; }
  HandlerType getMapHandler() const { return map_fn_; }
  IndexT getRange() const { return range_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | elm_proxy_ | from_ | to_ | map_fn_ | range_;
  }

private:
  VrtElmProxy<ColT, IndexT> elm_proxy_;
  NodeType from_ = uninitialized_destination;
  NodeType to_ = uninitialized_destination;
  HandlerType map_fn_ = uninitialized_handler;
  IndexT range_;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H*/
