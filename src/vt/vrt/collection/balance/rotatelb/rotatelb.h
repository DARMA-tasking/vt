
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ROTATELB_ROTATELB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ROTATELB_ROTATELB_H

#include "config.h"
#include "messaging/message.h"
#include "vrt/collection/balance/proc_stats.h"
#include "timing/timing.h"

#include <memory>
#include <list>
#include <map>
#include <cstdlib>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct RotateLBTypes {
  using ObjIDType = balance::ProcStats::ElementIDType;
  using ObjBinType = int32_t;
  using ObjBinListType = std::list<ObjIDType>;
  using ObjSampleType = std::map<ObjBinType, ObjBinListType>;
  using LoadType = double;
  using LoadProfileType = std::unordered_map<NodeType,LoadType>;
};

struct RotateObjMsg : ::vt::Message {};

struct RotateLB : RotateLBTypes {
  using ElementLoadType = std::unordered_map<ObjIDType,TimeType>;
  using ProcStatsMsgType = balance::ProcStatsMsg;
  using TransferType = std::map<NodeType, std::vector<ObjIDType>>;
  using LoadType = double;

  RotateLB() = default;

private:
  void finishedMigrate();
  void procDataIn(ElementLoadType const& data_in);
  static std::unique_ptr<RotateLB> rotate_lb_inst;

public:
  int64_t transfer_count = 0;
  static void rotateLBHandler(balance::RotateLBMsg* msg);
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ROTATELB_ROTATELB_H*/
