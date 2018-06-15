
#include "config.h"
#include "vrt/collection/balance/rotatelb/rotatelb.h"

#include <memory>

namespace vt { namespace vrt { namespace collection { namespace lb {

/*static*/ void RotateLB::rotateObjHan(RotateObjMsg* msg) {
  auto objs = reinterpret_cast<ObjIDType*>(
    reinterpret_cast<char*>(msg) + sizeof(RotateObjMsg)
  );
  auto const num_objs = *objs;
  auto objs_start = objs + 1;
  for (auto i = 0; i < num_objs; i++) {
  }
}

void RotateLB::procDataIn(ElementLoadType const& data_in) {
  auto const& this_node = theContext()->getNode();
  debug_print(
    lblite, node,
    "{}: procDataIn: size={}\n", this_node, data_in.size()
  );
  std::vector<ObjIDType> objs;
  for (auto&& stat : data_in) {
    auto const& obj = stat.first;
    auto const& load = stat.second;
    objs.push_back(obj);
    debug_print(
      lblite, node,
      "\t {}: procDataIn: obj={}, load={}\n", this_node, obj, load
    );
  }
  auto const& bytes = (objs.size()+1)*sizeof(ObjIDType);
  auto msg = makeSharedMessageSz<RotateObjMsg>(bytes);
  auto const& num_nodes = theContext()->getNumNodes();
  auto const next_node = this_node + 1 > num_nodes-1 ? 0 : this_node + 1;
  theMsg()->sendMsgSz<RotateObjMsg,rotateObjHan>(
    next_node, msg, sizeof(RotateObjMsg) + bytes
  );
}

/*static*/ void RotateLB::rotateLBHandler(balance::RotateLBMsg* msg) {
  auto const& phase = msg->getPhase();
  RotateLB::rotate_lb_inst = std::make_unique<RotateLB>();
  assert(balance::ProcStats::proc_data_.size() >= phase);
  RotateLB::rotate_lb_inst->procDataIn(balance::ProcStats::proc_data_[phase]);
}

/*static*/ std::unique_ptr<RotateLB> RotateLB::rotate_lb_inst;

}}}} /* end namespace vt::vrt::collection::lb */

