/*
//@HEADER
// *****************************************************************************
//
//                              collective_scope.h
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

#if !defined INCLUDED_VT_COLLECTIVE_COLLECTIVE_SCOPE_H
#define INCLUDED_VT_COLLECTIVE_COLLECTIVE_SCOPE_H

#include "vt/config.h"

#include <unordered_map>

namespace vt { namespace collective {

struct CollectiveScope;
struct CollectiveAlg;

namespace detail {

struct ScopeImpl {

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | live_
      | next_seq_
      | planned_collective_;
  }

private:
  ScopeImpl() = default;

  friend collective::CollectiveScope;
  friend collective::CollectiveAlg;

private:
  struct CollectiveInfo {
    CollectiveInfo(TagType in_seq, ActionType in_action, EpochType in_epoch)
      : seq_(in_seq), action_(in_action), epoch_(in_epoch)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      s | seq_
        | action_
        | epoch_;
    }

    TagType seq_ = no_tag;
    ActionType action_ = no_action;
    EpochType epoch_ = no_epoch;
  };

private:
  bool live_ = true;            /**< Whether the \c CollectiveScope is live */
  TagType next_seq_ = 1;        /**< The next sequence tag for this scope */
  std::unordered_map<TagType, CollectiveInfo> planned_collective_;
};

} /* end namespace detail */

/**
 * \struct CollectiveScope
 *
 * \brief A distinct scope for enqueuing ordered collective operations.
 *
 * Each collective scope builds an individual sequence of collective operations
 * that get orchestrated across nodes using a consensus algorithm so all nodes
 * agree on a particular operation to execute in the sequence.
 *
 * An example use case is running a blocking MPI collective (like a
 * \c * MPI_Allreduce) inside VT handlers without causing progress to halt due
 * to asynchrony in VT.
 */
struct CollectiveScope {

private:
  /**
   * \internal \brief Construct a collective scope for MPI operations.
   *
   * \param[in] in_scope the scope tag
   * \param[in] in_is_user_tag whether the scope tag is a user or system tag
   */
  explicit CollectiveScope(bool in_is_user_tag, TagType in_scope)
    : is_user_tag_(in_is_user_tag),
      scope_(in_scope)
  { }

  friend struct CollectiveAlg;

public:
  CollectiveScope(CollectiveScope&& other)
    : is_user_tag_(other.is_user_tag_),
      scope_(other.scope_)
  {
    // invalidate scope for deallocation
    other.scope_ = no_tag;
  }

  CollectiveScope(CollectiveScope const&) = delete;
  CollectiveScope& operator=(CollectiveScope&&) = delete;
  CollectiveScope& operator=(CollectiveScope const&) = delete;

  ~CollectiveScope();

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
   * The operation is counted as activity in the active termination
   * detection epoch
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

  /**
   * \internal \brief Get whether this scope is a user-tagged scope
   *
   * \note Used for testing purposes
   *
   * \return whether this scope is a user tag
   */
  bool isUserTag() const { return is_user_tag_; }

  /**
   * \internal \brief Get the scope tag bits
   *
   * \note Used for testing purposes
   *
   * \return the scope bits
   */
  TagType getScopeBits() const { return scope_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | is_user_tag_
      | scope_;
  }

private:

  /**
   * \internal \brief Get the scope state
   *
   * \return the scope state
   */
  detail::ScopeImpl* getScope();

private:
  bool is_user_tag_ = false;
  TagType scope_ = no_tag;
};

}} /* end namespace vt::collective */

#endif /*INCLUDED_VT_COLLECTIVE_COLLECTIVE_SCOPE_H*/
