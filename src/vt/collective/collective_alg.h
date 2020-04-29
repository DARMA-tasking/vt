/*
//@HEADER
// *****************************************************************************
//
//                               collective_alg.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H
#define INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H

#include "vt/config.h"
#include "vt/collective/tree/tree.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/collective/barrier/barrier.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/collective/scatter/scatter.h"
#include "vt/utils/hash/hash_tuple.h"
#include "vt/runtime/component/component_pack.h"

#include <unordered_map>

namespace vt { namespace collective {

constexpr CollectiveAlgType const fst_collective_alg = 1;

struct CollectiveAlg :
    runtime::component::Component<CollectiveAlg>,
    virtual reduce::Reduce,
    virtual barrier::Barrier,
    virtual scatter::Scatter
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

  //----------------------------------------------------------------------------
  //----------------------------------------------------------------------------


  std::string name() override { return "Collective"; }

public:
  /**
   * \brief Enqueue a lambda with an embedded closed set of MPI operations
   * (including collectives) to execute in the future. Returns
   * immediately, enqueuing the action for the future.
   *
   * The set of operations specified in the lambda must be closed, meaning that
   * MPI requests must not escape the lambda. After the lambda finishes, the set
   * of MPI collective calls should be complete.
   *
   * Any buffers captured in the lambda to use with the MPI operations
   * are in use until \c isCollectiveDone returns \c true or \c
   * waitCollective returns on the returned \c TagType
   *
   * \param[in] action the action containing a closed set of MPI operations
   *
   * \return tag representing the operation set
   */
  TagType mpiCollectiveAsync(ActionType action);

  /**
   * \brief Query whether an enqueued MPI operation set is complete
   *
   * \param[in] tag MPI operation set identifier
   *
   * \return whether it has finished or not
   */
  bool isCollectiveDone(TagType tag);

  /**
   * \brief Wait on an MPI operation set to complete
   *
   * \param[in] tag MPI collective set identifier
   */
  void waitCollective(TagType tag);

  /**
   * \brief Enqueue a lambda with an embedded closed set of MPI operations
   * (including collectives). Spin in the VT scheduler until it terminates.
   *
   * \param[in] action the action containing a set of MPI operations
   */
  void mpiCollectiveWait(ActionType action);

private:
  struct CollectiveMsg : vt::collective::ReduceNoneMsg {
    CollectiveMsg(TagType in_tag, NodeType in_root)
      : tag_(in_tag),
        root_(in_root)
    { }

    TagType tag_ = 0;
    NodeType root_ = uninitialized_destination;
  };

  struct CollectiveInfo {
    CollectiveInfo(TagType in_tag, ActionType in_action)
      : tag_(in_tag), action_(in_action)
    { }

    TagType tag_ = no_tag;
    ActionType action_ = no_action;
  };

  static void runCollective(CollectiveMsg* msg);

private:
  TagType next_tag_ = 1;
  std::unordered_map<TagType, CollectiveInfo> planned_collective_;
  std::vector<MsgSharedPtr<CollectiveMsg>> postponed_collectives_;
};

using ReduceMsg = reduce::ReduceMsg;

}}  // end namespace vt::collective

namespace vt {

extern collective::CollectiveAlg *theCollective();

} //end namespace vt

#include "vt/collective/reduce/reduce.impl.h"
#include "vt/collective/scatter/scatter.impl.h"

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H*/
