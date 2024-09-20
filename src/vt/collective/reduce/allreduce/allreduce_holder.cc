#include "allreduce_holder.h"
#include "vt/objgroup/manager.h"

namespace vt::collective::reduce::allreduce {

objgroup::proxy::Proxy<Rabenseifner> AllreduceHolder::addRabensifnerAllreducer(
  detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
  size_t num_elems) {
  auto const coll_proxy = strong_proxy.get();

  auto obj_proxy = theObjGroup()->makeCollective<Rabenseifner>(
    "rabenseifer_allreducer", strong_proxy, strong_group, num_elems);

  col_reducers_[coll_proxy].first = obj_proxy.getProxy();

  fmt::print(
    "Adding new Rabenseifner reducer for collection={:x}\n", coll_proxy);

  return obj_proxy;
}

objgroup::proxy::Proxy<RecursiveDoubling>
AllreduceHolder::addRecursiveDoublingAllreducer(
  detail::StrongVrtProxy strong_proxy, detail::StrongGroup strong_group,
  size_t num_elems) {
  auto const coll_proxy = strong_proxy.get();
  auto obj_proxy = theObjGroup()->makeCollective<RecursiveDoubling>(
    "recursive_doubling_allreducer", strong_proxy, strong_group, num_elems);

  col_reducers_[coll_proxy].second = obj_proxy.getProxy();
  fmt::print(
    "Adding new RecursiveDoubling reducer for collection={:x}\n", coll_proxy);

  return obj_proxy;
}

objgroup::proxy::Proxy<Rabenseifner>
AllreduceHolder::addRabensifnerAllreducer(detail::StrongGroup strong_group) {
  auto const group = strong_group.get();

  auto obj_proxy = theObjGroup()->makeCollective<Rabenseifner>(
    "rabenseifer_allreducer", strong_group);

  group_reducers_[group].first = obj_proxy.getProxy();

  fmt::print(
    "Adding new Rabenseifner reducer for group={:x} Size={}\n", group,
    group_reducers_.size());

  return obj_proxy;
}

objgroup::proxy::Proxy<RecursiveDoubling>
AllreduceHolder::addRecursiveDoublingAllreducer(
  detail::StrongGroup strong_group) {
  auto const group = strong_group.get();

  auto obj_proxy = theObjGroup()->makeCollective<RecursiveDoubling>(
    "recursive_doubling_allreducer", strong_group);

  fmt::print("Adding new RecursiveDoubling reducer for group={:x}\n", group);

  group_reducers_[group].second = obj_proxy.getProxy();

  return obj_proxy;
}

void AllreduceHolder::remove(detail::StrongVrtProxy strong_proxy) {
  auto const key = strong_proxy.get();

  auto it = col_reducers_.find(key);

  if (it != col_reducers_.end()) {
    col_reducers_.erase(key);
  }
}

void AllreduceHolder::remove(detail::StrongGroup strong_group) {
  auto const key = strong_group.get();

  auto it = group_reducers_.find(key);

  if (it != group_reducers_.end()) {
    group_reducers_.erase(key);
  }
}

} // namespace vt::collective::reduce::allreduce
