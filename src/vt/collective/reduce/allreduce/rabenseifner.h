

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H

#include "vt/messaging/message/shared_message.h"
#include "vt/objgroup/manager.h"
#include "vt/collective/reduce/allreduce/allreduce.h"

#include <utility>

namespace vt::collective::reduce::allreduce {

template <auto f, template <typename Arg> class Op, typename... Args>
void allreduce_r(Args&&... data) {
  auto msg = vt::makeMessage<AllreduceMsg>(std::forward<Args>(data)...);
  auto const this_node = vt::theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  using Reducer = Allreduce<Args...>;

  auto grp_proxy =
    vt::theObjGroup()->makeCollective<Reducer>("allreduce_rabenseifner");

  auto const lastNode = num_nodes - 1;
  auto const num_steps = static_cast<int32_t>(log2(num_nodes));
  auto const nprocs_pof2 = 1 << num_steps;
  auto const nprocs_rem = num_nodes - nprocs_pof2;

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// STEP 1 ////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////

  int vrt_node;
  bool const is_part_of_adjustment_group = this_node < (2 * nprocs_rem);
  bool const is_even = this_node % 2 == 0;
  vt::runInEpochCollective([=, &vrt_node] {
    vt::runInEpochCollective([=] {
      if (is_part_of_adjustment_group) {
        auto const partner = is_even ? this_node + 1 : this_node - 1;
        grp_proxy[partner].send<&Reducer::sendHandler>(
          std::forward<Args...>(data...));
      }
    });

    vt::runInEpochCollective([=] {
      if (is_part_of_adjustment_group and not is_even) {
        auto& vec = grp_proxy[this_node].get()->data_;
        grp_proxy[this_node - 1].send<&Reducer::reducedHan>(
          std::vector<int32_t>{vec.begin() + (vec.size() / 2), vec.end()});
      }
    });

    if (is_part_of_adjustment_group) {
      if (is_even) {
        vrt_node = this_node / 2;
      } else {
        vrt_node = -1;
      }

    } else { /* rank >= 2 * nprocs_rem */
      vrt_node = this_node - nprocs_rem;
    }
  });

  ////////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////////////////////////// STEP 2 ////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////////////////////////////////

  // int step = 0;
  // auto const wsize = data.size();

  // auto& vec = grp_proxy[this_node].get()->data_;

  // /*
  //   Scatter Reduce (distance doubling with vector halving)
  // */
  // for (int mask = 1; mask < (1 << num_steps); mask <<= 1) {
  //   int vdest = vrt_node ^ mask;
  //   int dest = (vdest < nprocs_rem) ? vdest * 2 : vdest + nprocs_rem;

  //   vt::runInEpochCollective([=] {
  //     if (vrt_node != -1) {
  //       if (this_node < dest) {
  //         grp_proxy[dest].send<&NodeObj::rightHalf>(
  //           std::vector<int32_t>{vec.begin() + (vec.size() / 2), vec.end()});
  //       } else {
  //         grp_proxy[dest].send<&NodeObj::leftHalf>(
  //           std::vector<int32_t>{vec.begin(), vec.end() - (vec.size() / 2)});
  //       }
  //     }
  //   });
  // }

  // step = num_steps - 1;

  // /*
  //   AllGather (distance halving with vector halving)
  // */
  // for (int mask = (1 << num_steps) >> 1; mask > 0; mask >>= 1) {
  //   int vdest = vrt_node ^ mask;
  //   /* Translate vdest virtual rank to real rank */
  //   int dest = (vdest < nprocs_rem) ? vdest * 2 : vdest + nprocs_rem;
  //   vt::runInEpochCollective([=] {
  //     if (vrt_node != -1) {
  //       if (this_node < dest) {
  //         grp_proxy[dest].send<&NodeObj::leftHalfComplete>(
  //           std::vector<int32_t>{vec.begin(), vec.end() - (vec.size() / 2)});
  //       } else {
  //         grp_proxy[dest].send<&NodeObj::rightHalfComplete>(
  //           std::vector<int32_t>{vec.begin() + (vec.size() / 2), vec.end()});
  //       }
  //     }
  //   });
  // }

  /*
    Send to excluded nodes (if needed)
  */

  /*
    Local invoke of the handler
  */
}

} // namespace vt::collective::reduce::allreduce

#endif // INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H
