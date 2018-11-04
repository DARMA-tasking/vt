
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_IMPL_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/proc_stats.h"

#include <vector>
#include <unordered_map>
#include <cassert>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename ColT>
/*static*/ ProcStats::ElementIDType ProcStats::addProcStats(
  VirtualElmProxyType<ColT> const& elm_proxy, ColT* col_elm,
  PhaseType const& phase, TimeType const& time
) {
  auto const& next_elm = ProcStats::getNextElm();

  debug_print(
    vrt_coll, node,
    "ProcStats::addProcStats: element={}, phase={}, load={}\n",
    next_elm, phase, time
  );

  proc_data_.resize(phase + 1);
  auto elm_iter = proc_data_.at(phase).find(next_elm);
  assert(elm_iter == proc_data_.at(phase).end() && "Must not exist");
  proc_data_.at(phase).emplace(
    std::piecewise_construct,
    std::forward_as_tuple(next_elm),
    std::forward_as_tuple(time)
  );
  auto migrate_iter = proc_migrate_.find(next_elm);
  assert(migrate_iter == proc_migrate_.end() && "Migrate func must not exist");
  proc_migrate_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(next_elm),
    std::forward_as_tuple([elm_proxy,col_elm](NodeType node){
      col_elm->migrate(node);
    })
  );
  return next_elm;
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_IMPL_H*/
