
#if !defined INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H
#define INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H

#include "vt/config.h"
#include "vt/collective/reduce/operators/default_op.h"
#include "vt/collective/reduce/reduce_msg.h"

#include <array>
#include <vector>

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename>
struct ReduceCombine;

template <typename DataType>
struct ReduceDataMsg : ReduceMsg, ReduceCombine<void> {
  ReduceDataMsg() = default;
  explicit ReduceDataMsg(DataType&& in_val)
    : ReduceMsg(), ReduceCombine<void>(),
      val_(std::forward<DataType>(in_val))
  { }
  explicit ReduceDataMsg(DataType const& in_val)
    : ReduceMsg(), ReduceCombine<void>(), val_(in_val)
  { }

  DataType const& getConstVal() const { return val_; }
  DataType& getVal() { return val_; }
  DataType&& getMoveVal() { return std::move(val_); }

  template <typename SerializerT>
  void invokeSerialize(SerializerT& s) {
    ReduceMsg::invokeSerialize(s);
    s | val_;
  }

protected:
  DataType val_ = {};
};

template <typename T>
struct ReduceTMsg : ReduceDataMsg<T> {
  using DataType = T;
  ReduceTMsg() = default;
  explicit ReduceTMsg(DataType&& in_val)
    : ReduceDataMsg<DataType>(std::forward<DataType>(in_val))
  { }
  explicit ReduceTMsg(DataType const& in_val)
    : ReduceDataMsg<DataType>(in_val)
  { }
};

template <typename T, std::size_t N>
struct ReduceArrMsg : ReduceDataMsg<std::array<T,N>> {
  using DataType = std::array<T,N>;
  ReduceArrMsg() = default;
  explicit ReduceArrMsg(DataType&& in_val)
    : ReduceDataMsg<DataType>(std::forward<DataType>(in_val))
  { }
  explicit ReduceArrMsg(DataType const& in_val)
    : ReduceDataMsg<DataType>(in_val)
  { }
};

template <typename T>
struct ReduceVecMsg : ReduceDataMsg<std::vector<T>> {
  using DataType = std::vector<T>;

  ReduceVecMsg() = default;
  explicit ReduceVecMsg(DataType&& in_val)
    : ReduceDataMsg<DataType>(std::forward<DataType>(in_val))
  { }
  explicit ReduceVecMsg(DataType const& in_val)
    : ReduceDataMsg<DataType>(in_val)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | ReduceDataMsg<DataType>::val_;
  }
};


}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using ReduceVecMsg = reduce::operators::ReduceVecMsg<T>;

template <typename T, std::size_t N>
using ReduceArrMsg = reduce::operators::ReduceArrMsg<T,N>;

template <typename T>
using ReduceTMsg = reduce::operators::ReduceTMsg<T>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H*/
