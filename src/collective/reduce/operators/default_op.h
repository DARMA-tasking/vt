
#if !defined INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_OP_H
#define INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_OP_H

#include "config.h"
#include "collective/reduce/reduce_msg.h"
#include "collective/reduce/operators/default_msg.h"

#include <algorithm>

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename T>
struct PlusOp {
  void operator()(T& v1, T const& v2) {
    v1 += v2;
  }
};

template <typename T>
struct MaxOp {
  void operator()(T& v1, T const& v2) {
    v1 = std::max(v1,v2);
  }
};

template <typename T = void>
struct ReduceCombine {
  ReduceCombine() = default;
private:
  template <typename MsgT, typename Op, typename ActOp>
  static void combine(MsgT* m1, MsgT* m2) {
    Op()(m1->getVal(), m2->getConstVal());
  }
public:
  template <typename MsgT, typename Op, typename ActOp>
  static void msgHandler(MsgT* msg) {
    if (msg->isRoot()) {
      ActOp()(msg);
    } else {
      MsgT* fst_msg = msg;
      MsgT* cur_msg = msg->template getNext<MsgT>();
      while (cur_msg != nullptr) {
        ReduceCombine<>::combine<MsgT,Op,ActOp>(fst_msg, cur_msg);
        cur_msg = cur_msg->template getNext<MsgT>();
      }
    }
  }
};

}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using PlusOp = reduce::operators::PlusOp<T>;

template <typename T>
using MaxOp = reduce::operators::MaxOp<T>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_OP_H*/
