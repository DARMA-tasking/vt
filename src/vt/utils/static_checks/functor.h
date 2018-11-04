
#if !defined INCLUDED_UTILS_STATIC_CHECKS_FUNCTOR_H
#define INCLUDED_UTILS_STATIC_CHECKS_FUNCTOR_H

#include "vt/config.h"

namespace vt { namespace util {

template <typename FunctorT>
struct FunctorTraits;

template <typename FunctorT, typename ReturnT, typename MsgT>
struct FunctorTraits<ReturnT(FunctorT::*)(MsgT*)> {
  using FunctorType = FunctorT;
  using MessageType = MsgT;
  using ReturnType  = ReturnT;
};

template <
  typename FunctorT,
  typename FunctorFnT = decltype(&FunctorT::operator()),
  typename Traits = FunctorTraits<FunctorFnT>,
  typename MessageT = typename Traits::MessageType
>
struct FunctorExtractor {
  using FunctorType = FunctorT;
  using MsgT = MessageT;
  using MessageType = MessageT;
};

}} /* end namespace vt::util */

#endif /*INCLUDED_UTILS_STATIC_CHECKS_FUNCTOR_H*/
