
#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H

#include "config.h"
#include "tree/tree.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "collective/barrier/barrier.h"
#include "collective/reduce/reduce.h"
#include "utils/hash/hash_tuple.h"

#include <unordered_map>

namespace vt { namespace collective {

constexpr CollectiveAlgType const fst_collective_alg = 1;

struct CollectiveAlg :
    public virtual reduce::Reduce,
    public virtual barrier::Barrier
{
/*----------------------------------------------------------------------------
 *
 *  CollectiveAlg class implements all collective operations:
 *    1) Barrier
 *    2) One to all: broadcast, scatter
 *    3) All to one: reduce, gather
 *    4) All to all: allreduce, allgather, alltoall, reduce_scatter
 *    5) Scan etc.
 *
 *------------------------------------------------------------------------------
 */

  CollectiveAlg();

  template <typename T>
  using ContainerType = std::unordered_map<CollectiveAlgType, T>;

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------


};

using ReduceMsg = reduce::ReduceMsg;

}}  // end namespace vt::collective

namespace vt {

extern collective::CollectiveAlg *theCollective();

} //end namespace vt

#include "collective/reduce/reduce.impl.h"

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H*/
