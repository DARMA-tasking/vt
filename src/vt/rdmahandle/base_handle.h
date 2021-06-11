/*
//@HEADER
// *****************************************************************************
//
//                                base_handle.h
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

#if !defined INCLUDED_VT_RDMAHANDLE_BASE_HANDLE_H
#define INCLUDED_VT_RDMAHANDLE_BASE_HANDLE_H

#include "vt/config.h"
#include "vt/rdmahandle/common.h"
#include "vt/rdmahandle/handle_key.h"
#include "vt/rdmahandle/request_holder.h"
#include "vt/rdmahandle/lock_mpi.h"

namespace vt { namespace rdma {

/** \file */

/**
 * \struct BaseHandle base_handle.h vt/rdmahandle/base_handle.h
 *
 * \brief The untyped, lowest base class for all handles
 */
struct BaseHandle { };

struct Manager;

template <typename T, HandleEnum E, typename IndexT>
struct SubHandle;

/**
 * \struct BaseTypedHandle base_handle.h vt/rdmahandle/base_handle.h
 *
 * \brief The typed base class for all types of handles
 */
template <typename T, HandleEnum E, typename IndexT>
struct BaseTypedHandle : BaseHandle {
  using DataT          = T;
  using RequestType    = RequestHolder;
  using ActionDataType = std::function<void(T*)>;
  using IndexType      = IndexT;

  static constexpr HandleEnum handle_type = E;

  friend struct Manager;
  friend struct SubHandle<T, E, IndexT>;

  BaseTypedHandle() = default;
  BaseTypedHandle(BaseTypedHandle const&) = default;
  BaseTypedHandle(BaseTypedHandle&&) = default;
  BaseTypedHandle& operator=(BaseTypedHandle const&) = default;
  BaseTypedHandle& operator=(BaseTypedHandle&&) = default;

protected:
  /**
   * \brief Protected construction of a base handle, shared amongst all handle
   * types
   *
   * \param[in] in_count count of handle
   * \param[in] in_hoff local offset for handle
   * \param[in] in_lock shared pointer to lock
   */
  BaseTypedHandle(
    std::size_t in_count,
    std::size_t in_hoff = 0,
    std::shared_ptr<LockMPI> in_lock = nullptr
  ) : count_(in_count),
      hoff_(in_hoff),
      lock_(in_lock)
  { }

public:
  /**
   * \brief Check if the handle has an action registered
   *
   * \return whether an action is registered
   */
  bool hasAction() const { return actions_.size() > 0; }

  /**
   * \brief Clear all registered actions
   */
  void clearActions() const { return actions_.clear(); }

  /**
   * \brief Add an action to the handle
   *
   * \param[in] in_action action to add
   */
  void addAction(ActionDataType in_action) { actions_.push_back(in_action); }

  /**
   * \brief Register a buffer for fetching data into for this handle
   *
   * \param[in] in_buffer buffer to set
   */
  void setBuffer(T* in_buffer) { user_buffer_ = in_buffer; }

  /**
   * \brief Get the registered buffer
   *
   * \return pointer to the buffer
   */
  T* getBuffer() const { return user_buffer_; }

  /**
   * \brief Get the local handle offset
   *
   * \return the handle offset
   */
  std::size_t hoff() const { return hoff_; }

  /**
   * \brief Get the local handle count
   *
   * \return the handle size
   */
  std::size_t count() const { return count_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | count_;
    s | hoff_;

    s.skip(actions_);
    s.skip(lock_);
    s.skip(user_buffer_);
  }

protected:
  std::size_t count_                    = 0;      /**< The count of the handle */
  std::vector<ActionDataType> actions_ = {};      /**< The registered actions */
  T* user_buffer_                      = nullptr; /**< The registered buffer */
  std::size_t hoff_                    = 0;       /**< The local handle offset */
  std::shared_ptr<LockMPI> lock_       = nullptr; /**< The active lock */
};

}} /* end namespace vt::rdma */

#endif /*INCLUDED_VT_RDMAHANDLE_BASE_HANDLE_H*/
