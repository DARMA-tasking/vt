#ifndef KDLB_H
#define KDLB_H

#include "TreeStrategyBase.h"
#include "kd.h"
#include <iostream>

namespace TreeStrategy
{
template <typename O, typename P, typename S, typename T>
class BaseKdLB : public Strategy<O, P, S>
{
public:
  BaseKdLB() = default;
  void solve(std::vector<O>& objs, std::vector<P>& procs, S& solution, bool objsSorted)
  {
    if constexpr (O::dimension > 1) {
      for (const auto& p : procs) {
        vt_debug_print(
          normal, lb, "\tCharmLB::Initial proc load for PE {} ({})\n", p.id,
          join(p.load.begin(), p.load.end(), " "));
      }
    }

    // Sorts by maxload in vector
    if (!objsSorted)
      std::sort(objs.begin(), objs.end(), CmpLoadGreater<O>());

    auto objsIter = objs.begin();
    T* tree = nullptr;
    for (size_t i = 0; i < procs.size() && objsIter != objs.end();
         i++, objsIter++) {
      if constexpr (O::dimension > 1) {
        vt_debug_print(
          normal, lb, "\tCharmLB::Assigning obj {} ({}), to PE {} ({})\n",
          objsIter->id.id,
          join(objsIter->load.begin(), objsIter->load.end(), " "), procs[i].id,
          join(procs[i].load.begin(), procs[i].load.end(), " "));
      }

      solution.assign(*objsIter, procs[i]);
      tree = T::insert(tree, procs[i]);
    }

    for (; objsIter != objs.end(); objsIter++) {
      auto proc = *(T::findMinNorm(tree, *objsIter));

      if constexpr (O::dimension > 1) {
        vt_debug_print(
          normal, lb, "\tCharmLB::Assigning obj {} ({}), to PE {} ({})\n",
          objsIter->id.id,
          join(objsIter->load.begin(), objsIter->load.end(), " "), proc.id,
          join(proc.load.begin(), proc.load.end(), " "));
      }
      tree = T::remove(tree, proc);
      solution.assign(*objsIter, proc);
      tree = T::insert(tree, proc);
    }

    if constexpr (O::dimension > 1) {
      for (const auto& p : procs) {
        vt_debug_print(
          normal, lb, "\tCharmLB::Final proc load for PE {} ({})\n", p.id,
          join(p.load.begin(), p.load.end(), " "));
      }
    }
  }
};

template <typename O, typename P, typename S>
class KdLB : public BaseKdLB<O, P, S, KDNode<P>>
{
};

template <typename O, typename P, typename S>
class RKdLB : public BaseKdLB<O, P, S, RKDNode<P>>
{
};

}  // namespace TreeStrategy

#endif /* KDLB_H */
