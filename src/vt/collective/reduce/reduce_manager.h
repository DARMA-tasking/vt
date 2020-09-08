/*
//@HEADER
// *****************************************************************************
//
//                               reduce_manager.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_MANAGER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_MANAGER_H

#include "vt/collective/reduce/reduce_scope.h"
#include "vt/collective/tree/tree.h"

namespace vt { namespace collective { namespace reduce {

struct Reduce;

/**
 * \struct ReduceManager
 *
 * \brief Manage distinct scopes for reductions
 *
 * Manages distinct reducers with an associated scope to orchestrate
 *  multiple asynchronous sequences of reduce operations.
 */
struct ReduceManager {
  using ReducePtrType   = std::unique_ptr<Reduce>;
  using ReduceScopeType = detail::ReduceScopeHolder<ReducePtrType>;

  ReduceManager();

  /**
   * \brief Get the global reducer
   *
   * \warning Using the global reducer is not recommended because it might
   * conflict with other reductions in the global context. To create a new
   * scope, one may call \c makeReducerCollective to collectively create a new
   * reducer scope.
   *
   * \return the reducer
   */
  Reduce* global();

  /**
   * \brief Get the reducer for a given scope
   *
   * \param[in] scope the scope
   *
   * \return the reducer
   */
  Reduce* getReducer(detail::ReduceScope const& scope);

  /**
   * \internal \brief Get the reducer for an objgroup
   *
   * \param[in] proxy the objgroup proxy
   *
   * \return the reducer
   */
  Reduce* getReducerObjGroup(ObjGroupProxyType const& proxy);

  /**
   * \internal \brief Get the reducer for a collection
   *
   * \param[in] proxy the collection proxy
   *
   * \return the reducer
   */
  Reduce* getReducerVrtProxy(VirtualProxyType const& proxy);

  /**
   * \internal \brief Get the reducer for a group
   *
   * \param[in] group the group ID
   *
   * \return the reducer
   */
  Reduce* getReducerGroup(GroupType const& group);

  /**
   * \internal \brief Get the reducer for a VT component
   *
   * Each VT component that inherits from \c runtime::component::Component<T>
   * gets its own scope.
   *
   * \param[in] cid the component ID
   *
   * \return the reducer
   */
  Reduce* getReducerComponent(ComponentIDType const& cid);

  /**
   * \brief Collectively make a new reducer that creates a unique reduction
   * scope
   *
   * \return the reducer with a new scope
   */
  Reduce* makeReducerCollective();

  /**
   * \internal \brief Create the reducer for a group when a custom (or
   * non-global) spanning tree is required (cannot be created on demand)
   *
   * \param[in] group the group ID
   * \param[in] tree the associated spanning tree for the group
   */
  void makeReducerGroup(GroupType const& group, collective::tree::Tree* tree);

  /**
   * \internal \brief Active function when a message reaches the root of the
   * spanning tree and the reduction is complete
   *
   * \param[in] msg the reduce message
   */
  template <typename MsgT>
  static void reduceRootRecv(MsgT* msg);

  /**
   * \internal \brief Active function when a message arrives for a given scope
   * at some level in the spanning tree
   *
   * \param[in] msg the reduce message
   */
  template <typename MsgT>
  static void reduceUpHan(MsgT* msg);

private:
  ReduceScopeType reducers_;            /**< Live reducers by scope */
  detail::UserIDType cur_user_id_ = 0;  /**< The next user ID for a scope */
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_MANAGER_H*/
