
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H

#include "config.h"
#include "vrt/collection/balance/phase_msg.h"
#include "vrt/collection/balance/stats_msg.h"
#include "timing/timing.h"

#include <vector>
#include <unordered_map>
#include <tuple>
#include <functional>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct ProcStats {
  using ElementIDType = uint64_t;
  using MigrateFnType = std::function<void(NodeType)>;

public:
  template <typename ColT>
  static ElementIDType addProcStats(
    VirtualElmProxyType<ColT> const& elm_proxy, ColT* col_elm,
    PhaseType const& phase, TimeType const& time
  );

  static void clearStats();

private:
  static ElementIDType getNextElm();

  // @todo: make these private and friend appropriate classes
public:
  static ElementIDType next_elm_;
public:
  static std::vector<std::unordered_map<ElementIDType,TimeType>> proc_data_;
  static std::unordered_map<ElementIDType,MigrateFnType> proc_migrate_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#include "vrt/collection/balance/proc_stats.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H*/
