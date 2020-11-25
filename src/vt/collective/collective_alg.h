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
#include "vt/collective/reduce/reduce_manager.h"
#include "vt/collective/reduce/operators/default_msg.h"
#include "vt/collective/scatter/scatter.h"
#include "vt/utils/hash/hash_tuple.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/collective/collective_scope.h"

#include <memory>
#include <unordered_map>

namespace vt { namespace collective {

constexpr CollectiveAlgType const fst_collective_alg = 1;

/**
 * \struct CollectiveAlg
 *
 * \brief Perform asynchronous collectives within VT
 *
 * CollectiveAlg is a core VT component that provides the ability to perform
 * reductions, scatters, barriers, and safe MPI (collective) operations while
 * inside a VT handler.
 */
struct CollectiveAlg :
    runtime::component::Component<CollectiveAlg>,
    virtual reduce::ReduceManager,
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
   * \brief Create a new scope for sequenced MPI operations. Each scope has a
   * distinct, independent collective sequence of operations.
   *
   * \param[in] tag integer identifier (default value means allocate a new
   * system scope)
   *
   * \return a new collective scope with sequenced operations
   */
  CollectiveScope makeCollectiveScope(TagType scope_tag = no_tag);

private:
  friend struct CollectiveScope;

  struct CollectiveMsg : vt::collective::ReduceNoneMsg {
    CollectiveMsg(
      bool in_is_user_tag, TagType in_scope, TagType in_seq, NodeType in_root
    ) : is_user_tag_(in_is_user_tag),
        scope_(in_scope),
        seq_(in_seq),
        root_(in_root)
    { }

    bool is_user_tag_ = false;
    TagType scope_ = no_tag;
    TagType seq_ = no_tag;
    NodeType root_ = uninitialized_destination;
  };

  static void runCollective(CollectiveMsg* msg);

public:
  /**
   * \internal \brief Check if a scope has been deallocated
   *
   * \note Used for testing purposes
   *
   * \param[in] is_user_tag whether it's a user-tagged scope
   * \param[in] scope_bits the scope bits
   *
   * \return whether it is deallocated
   */
  bool isDeallocated(bool is_user_tag, TagType scope_bits) const;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | next_system_scope_
      | user_scope_
      | system_scope_
      | postponed_collectives_;
  }

private:
  using ScopeMapType = std::unordered_map<TagType, std::unique_ptr<detail::ScopeImpl>>;

  TagType next_system_scope_ = 1;     /**< The next system allocated scope */
  ScopeMapType user_scope_;           /**< Live scopes with user tag */
  ScopeMapType system_scope_;         /**< Live scopes with system tag */
  std::vector<MsgSharedPtr<CollectiveMsg>> postponed_collectives_;
};

using ReduceMsg = reduce::ReduceMsg;

}}  // end namespace vt::collective

namespace vt {

extern collective::CollectiveAlg *theCollective();

} //end namespace vt

#include "vt/collective/reduce/reduce_manager.impl.h"
#include "vt/collective/scatter/scatter.impl.h"

#endif /*INCLUDED_COLLECTIVE_COLLECTIVE_ALG_H*/
