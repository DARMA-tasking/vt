
#if !defined INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H
#define INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H

#include "config.h"
#include "collective/reduce/reduce_msg.h"

#include <array>
#include <vector>

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename MsgT, typename Op, typename ActOp>
struct ReduceCombine;

template <typename MsgT, typename DataType, typename Op, typename ActOp>
struct ReduceDataMsg : ReduceMsg, ReduceCombine<MsgT,Op,ActOp>
{
  ReduceDataMsg() = default;
  explicit ReduceDataMsg(DataType&& in_val)
    : val_(std::forward<DataType>(in_val)),
      ReduceMsg(), ReduceCombine<MsgT,Op,ActOp>()
  { }
  explicit ReduceDataMsg(DataType const& in_val)
    : val_(in_val), ReduceMsg(), ReduceCombine<MsgT,Op,ActOp>()
  { }

  DataType const& getConstVal() const { return val_; }
  DataType& getVal() { return val_; }

protected:
  DataType val_ = {};
};

template <typename MsgT, typename T, typename Op, typename ActOp>
struct ReduceTMsg : ReduceDataMsg<MsgT,T,Op,ActOp> {
  using DataType = T;
  ReduceTMsg() = default;
  explicit ReduceTMsg(DataType&& in_val)
    : ReduceDataMsg<MsgT,DataType,Op,ActOp>(std::forward<DataType>(in_val))
  { }
  explicit ReduceTMsg(DataType const& in_val)
    : ReduceDataMsg<MsgT,DataType,Op,ActOp>(in_val)
  { }
};

template <typename MsgT, typename T, std::size_t N, typename Op, typename ActOp>
struct ReduceArrMsg : ReduceDataMsg<MsgT,std::array<T,N>,ActOp,Op> {
  using DataType = std::array<T,N>;
  ReduceArrMsg() = default;
  explicit ReduceArrMsg(DataType&& in_val)
    : ReduceDataMsg<MsgT,DataType,Op,ActOp>(std::forward<DataType>(in_val))
  { }
  explicit ReduceArrMsg(DataType const& in_val)
    : ReduceDataMsg<MsgT,DataType,Op,ActOp>(in_val)
  { }
};

template <typename MsgT, typename T, typename Op, typename ActOp>
struct ReduceVecMsg : ReduceDataMsg<MsgT,std::vector<T>,ActOp,Op> {
  using DataType = std::vector<T>;

  ReduceVecMsg() = default;
  explicit ReduceVecMsg(DataType&& in_val)
    : ReduceDataMsg<MsgT,DataType,Op,ActOp>(std::forward<DataType>(in_val))
  { }
  explicit ReduceVecMsg(DataType const& in_val)
    : ReduceDataMsg<MsgT,DataType,Op,ActOp>(in_val)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | ReduceDataMsg<MsgT,DataType,Op,ActOp>::val_;
  }
};


}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename MsgT, typename T, typename Op, typename ActOp>
using ReduceVecMsg = reduce::operators::ReduceVecMsg<MsgT,T,Op,ActOp>;

template <typename MsgT, typename T, std::size_t N, typename Op, typename ActOp>
using ReduceArrMsg = reduce::operators::ReduceArrMsg<MsgT,T,N,Op,ActOp>;

template <typename MsgT, typename T, typename Op, typename ActOp>
using ReduceTMsg = reduce::operators::ReduceTMsg<MsgT,T,Op,ActOp>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H*/
