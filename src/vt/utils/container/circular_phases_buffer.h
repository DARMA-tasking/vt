/*
//@HEADER
// *****************************************************************************
//
//                           circular_phases_buffer.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H
#define INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H

#include "vt/config.h"
#include <unordered_map>
#include <queue>

namespace vt { namespace util { namespace container {

/**
 * \struct CircularPhasesBuffer
 *
 * \brief The circular buffer which holds data related to a set of phases.
 * PhaseType is used as a key when storing or retrieving data. If capacity was not specified then buffer is allow to grow without limit.
 */
template <typename StoredType>
struct CircularPhasesBuffer {
  using StoredPair = std::pair<PhaseType, StoredType>;

  /**
   * \brief Construct a CircularPhasesBuffer
   *
   * \param[in] in_capacity the requested capacity of the buffer
   */
  CircularPhasesBuffer(std::size_t in_capacity = 0) {
    requested_capacity_ = in_capacity;
    buffer_.reserve(in_capacity);
  }

  /**
   * \brief Construct a CircularPhasesBuffer. The max size of buffer will be equal to the size of the list
   *
   * \param[in] in_list the initializer list with elements to be put into the buffer
   */
  CircularPhasesBuffer(std::initializer_list<StoredPair> in_list) {
    requested_capacity_ = in_list.size();
    buffer_.reserve(in_list.size());

    for (auto pair : in_list) {
      addToCache(pair.first, std::move(pair.second));
    }
  }

  /**
   * \brief Store data in the buffer
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  void store(const PhaseType phase, StoredType&& obj) {
    addToCache(phase, std::move(obj));
  }

  /**
   * \brief Store data in the buffer
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  void store(const PhaseType phase, StoredType const& obj) {
    addToCache(phase, obj);
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  const StoredType& at(const PhaseType phase) const {
    return buffer_.at(phase);
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& at(const PhaseType phase) { return buffer_.at(phase); }

  /**
   * \brief Get data related to the phase. Insert new data if phase is not present in the buffer.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& operator[](const PhaseType phase) {
    if (!contains(phase)) {
      addToCache(phase, StoredType{});
    }
    return buffer_[phase];
  }

  /**
   * \brief Find an element for passed phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return iterator to the requested element.
   */
  auto find(const PhaseType& phase) {
    return buffer_.find(phase);
  }

  /**
   * \brief Find an element for passed phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return const iterator to the requested element.
   */
  auto find(const PhaseType& phase) const {
    return buffer_.find(phase);
  }

  /**
   * \brief Check if phase is present in the buffer
   *
   * \param[in] phase the phase to look for
   *
   * \return whether buffer contains the phase or not
   */
  bool contains(const PhaseType phase) const {
    return buffer_.find(phase) != buffer_.end();
  }

  /**
   * \brief Resize buffer to the requested size
   *
   * \param[in] new_size the requested new size of the buffer
   */
  void resize(const std::size_t new_size) {
    if (new_size < buffer_.size()) {
      std::size_t remove_last = buffer_.size() - new_size;
      for (std::size_t i = 0; i < remove_last; i++) {
        removeOldest();
      }
    }

    requested_capacity_ = new_size;
  }

  /**
   * \brief Get current size of the buffer
   *
   * \return the current size
   */
  std::size_t size() const { return buffer_.size(); }

  /**
   * \brief Check if the buffer is empty
   *
   * \return whether the buffer is empty or not
   */
  bool empty() const { return buffer_.empty(); }

  /**
   * \brief Clears content of the buffer. Does not change the buffer maximum capacity.
   */
  void clear() {
    std::queue<PhaseType> empty;
    indexes_.swap(empty);
    buffer_.clear();
  }

  /**
   * @brief Clears content of the buffer and allow the buffer maximum capacity to grow
   */
  void reset() {
    requested_capacity_ = 0;
    clear();
  }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    s | requested_capacity_;
    s | indexes_;
    s | buffer_;
  }

  /**
   * @brief Get iterator to the beginning of the buffer
   *
   * @return auto the begin iterator
   */
  auto begin() { return buffer_.begin(); }

  /**
   * @brief Get const iterator to the beginning of the buffer
   *
   * @return auto the const begin iterator
   */
  auto begin() const { return buffer_.begin(); }

  /**
   * @brief Get const iterator to the beginning of the buffer
   *
   * @return auto the const begin iterator
   */
  auto cbegin() const { return buffer_.cbegin(); }

  /**
   * @brief Get iterator to the space after buffer
   *
   * @return auto the end iterator
   */
  auto end() { return buffer_.end(); }

  /**
   * @brief Get const iterator to the space after buffer
   *
   * @return auto the const end iterator
   */
  auto end() const { return buffer_.end(); }

  /**
   * @brief Get const iterator to the space after buffer
   *
   * @return auto the const end iterator
   */
  auto cend() const { return buffer_.cend(); }

private:
  /**
   * @brief Add new phase to the cache and remove oldest one if buffer exceeds requested size
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] data the data to store
   */
  void addToCache(const PhaseType& phase, StoredType&& data) {
    if (requested_capacity_ > 0 && buffer_.size() >= requested_capacity_) {
      removeOldest();
    }
    indexes_.push(phase);
    buffer_.emplace(phase, std::move(data));
  }

  /**
   * @brief Remove oldest phase from cache
   */
  void removeOldest() {
    buffer_.erase(indexes_.front());
    indexes_.pop();
  }

private:
  std::size_t requested_capacity_;
  std::queue<PhaseType> indexes_;
  std::unordered_map<PhaseType, StoredType> buffer_;
};

}}} /* end namespace vt::util::container */

#endif /*INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H*/
