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
   * \param[in] capacity the requested capacity of the buffer
   */
  CircularPhasesBuffer(std::size_t capacity = 0) {
    m_requested_capacity = capacity;
    m_buffer.reserve(capacity);
  }

  /**
   * \brief Construct a CircularPhasesBuffer. The max size of buffer will be equal to the size of the list
   *
   * \param[in] list the initializer list with elements to be put into the buffer
   */
  CircularPhasesBuffer(std::initializer_list<StoredPair> list) {
    m_requested_capacity = list.size();
    m_buffer.reserve(list.size());

    for (auto&& pair : list) {
      addToCache(std::make_pair(pair.first, pair.second));
    }
  }

  /**
   * \brief Store data in the buffer
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  void store(const PhaseType phase, StoredType&& obj) {
    addToCache(std::make_pair(phase, std::move(obj)));
  }

  /**
   * \brief Store data in the buffer
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  void store(const PhaseType phase, StoredType const& obj) {
    addToCache(std::make_pair(phase, obj));
  }

  /**
   * \brief Check if phase is present in the buffer
   *
   * \param[in] phase the phase to look for
   *
   * \return whether buffer contains the phase or not
   */
  bool contains(const PhaseType phase) const {
    return m_buffer.find(phase) != m_buffer.end();
  }

  /**
   * \brief Get data related to the phase. Insert new data if phase is not present in the buffer.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& operator[](const PhaseType phase) {
    if (!contains(phase)) {
      addToCache(std::make_pair(phase, StoredType{}));
    }
    return m_buffer[phase];
  }

  /**
   * \brief Find data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return pointer to the stored data or null if not present
   */
  const StoredType* find(const PhaseType phase) const {
    auto iter = m_buffer.find(phase);
    if (iter != m_buffer.end()) {
      return &iter->second;
    }
    return nullptr;
  }

  /**
   * \brief Find data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return pointer to the stored data or null if not present
   */
  StoredType* find(const PhaseType phase) {
    auto iter = m_buffer.find(phase);
    if (iter != m_buffer.end()) {
      return &iter->second;
    }
    return nullptr;
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  const StoredType& at(const PhaseType phase) const {
    return m_buffer.at(phase);
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& at(const PhaseType phase) { return m_buffer.at(phase); }

  /**
   * \brief Resize buffer to the requested size
   *
   * \param[in] new_size the requested new size of the buffer
   */
  void resize(const std::size_t new_size) {
    if (new_size < m_buffer.size()) {
      std::size_t remove_last = m_buffer.size() - new_size;
      for (std::size_t i = 0; i < remove_last; i++) {
        removeOldest();
      }
    }

    m_requested_capacity = new_size;
  }

  /**
   * \brief Get current size of the buffer
   *
   * \return the current size
   */
  std::size_t size() const { return m_buffer.size(); }

  /**
   * \brief Check if the buffer is empty
   *
   * \return whether the buffer is empty or not
   */
  bool empty() const { return m_buffer.empty(); }

  /**
   * \brief Clears content of the buffer. Does not change the buffer maximum capacity.
   */
  void clear() {
    std::queue<PhaseType> empty;
    m_indexes.swap(empty);
    m_buffer.clear();
  }

  /**
   * @brief Clears content of the buffer and allow the buffer maximum capacity to grow
   */
  void reset() {
    m_requested_capacity = 0;
    clear();
  }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    s | m_requested_capacity;
    s | m_indexes;
    s | m_buffer;
  }

  /**
   * @brief Get iterator to the begging of the buffer
   *
   * @return auto the begin iterator
   */
  auto begin() { return m_buffer.begin(); }

  /**
   * @brief Get iterator to the space after buffer
   *
   * @return auto the end iterator
   */
  auto end() { return m_buffer.end(); }

private:
  /**
   * @brief Add new phase to the cache and remove oldest one if buffer exceeds requested size
   *
   * @param pair the pair<PhaseType, StoredType> to be stored
   */
  void addToCache(StoredPair&& pair) {
    if (m_requested_capacity > 0 && m_buffer.size() >= m_requested_capacity) {
      removeOldest();
    }
    m_indexes.push(pair.first);
    m_buffer[pair.first] = std::move(pair.second);
  }

  /**
   * @brief Remove oldest phase from cache
   */
  void removeOldest() {
    m_buffer.erase(m_indexes.front());
    m_indexes.pop();
  }

private:
  std::size_t m_requested_capacity;
  std::queue<PhaseType> m_indexes;
  std::unordered_map<PhaseType, StoredType> m_buffer;
};

}}} /* end namespace vt::util::container */

#endif /* INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H */
