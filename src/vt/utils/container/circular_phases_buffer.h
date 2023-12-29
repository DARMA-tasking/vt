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

namespace vt { namespace util { namespace container {

/**
 * \struct CircularPhasesBuffer
 *
 * \brief The circular buffer which holds data related to a continuous set of phases.
 * PhaseType is used as a key when storing or retrieving data.
 */
template <typename StoredType>
struct CircularPhasesBuffer {
  using StoredPair = std::pair<PhaseType, StoredType>;

  /**
   * \brief Construct a CircularPhasesBuffer
   *
   * \param[in] capacity the requested capacity of the buffer
   */
  CircularPhasesBuffer(std::size_t capacity = 0)
    : vector_(capacity, StoredPair{invalid_, StoredType{}}) { }

  /**
   * \brief Construct a CircularPhasesBuffer. The size of buffer will be equal to the size of the list
   *
   * \param[in] list the initializer list with elements to be put into the buffer
   */
  CircularPhasesBuffer(std::initializer_list<StoredPair> list) {
    vector_.resize(list.size());
    for (auto&& pair : list) {
      store(pair.first, std::move(pair.second));
    }
  }

  /**
   * \brief Store data in the buffer
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  void store(const PhaseType phase, StoredType&& obj) {
    moveHeadAndTail();
    vector_[head_] = std::make_pair(phase, std::move(obj));
  }

  /**
   * \brief Store data in the buffer
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  void store(const PhaseType phase, StoredType const& obj) {
    moveHeadAndTail();
    vector_[head_] = std::make_pair(phase, obj);
  }

  /**
   * \brief Check if phase is present in the buffer
   *
   * \param[in] phase the phase to look for
   *
   * \return whether buffer contains the phase or not
   */
  bool contains(const PhaseType phase) const {
    return !empty() && phase <= vector_[head_].first &&
      phase >= vector_[tail_].first;
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
      store(phase, StoredType{});
    }
    return vector_[phaseToPos(phase)].second;
  }

  /**
   * \brief Find data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return pointer to the stored data or null if not present
   */
  const StoredType* find(const PhaseType phase) const {
    if (contains(phase)) {
      return &vector_[phaseToPos(phase)].second;
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
    if (contains(phase)) {
      return &vector_[phaseToPos(phase)].second;
    }
    return nullptr;
  }

  /**
   * \brief Get data related to the phase. Throws and exception if phase is not present.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  const StoredType& at(const PhaseType phase) const {
    vtAssert(contains(phase), "Buffer don't contain the requested phase.");

    return vector_[phaseToPos(phase)].second;
  }

  /**
   * \brief Get data related to the phase. Throws and exception if phase is not present.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& at(const PhaseType phase) {
    vtAssert(contains(phase), "Buffer don't contain the requested phase.");

    return vector_[phaseToPos(phase)].second;
  }

  /**
   * \brief Resize buffer to the requested new_size
   *
   * \param[in] new_size the requested new size of the buffer
   */
  void resize(const std::size_t new_size) {
    if (new_size == vector_.size()) {
      return;
    }

    auto new_vec =
      std::vector<StoredPair>(new_size, StoredPair{invalid_, StoredType{}});

    if (new_size > 0) {
      if (new_size < size()) {
        int tmp = head_ - new_size + 1;
        if (tmp < 0) {
          tmp += size();
        }

        std::size_t tmp_tail = static_cast<std::size_t>(tmp);
        for (int i = 0; tmp_tail != getNextEntry(head_);) {
          new_vec[i++] = std::move(vector_[tmp_tail]);
          tmp_tail = getNextEntry(tmp_tail);
        }

        head_ = new_size - 1;
        tail_ = 0;

      } else if (!empty()) {
        int i = 0;
        for (auto&& pair : *this) {
          new_vec[i++] = std::move(pair);
        }

        head_ = --i;
        tail_ = 0;
      }
    } else {
      resetIndexes();
    }

    vector_.swap(new_vec);
  }

  /**
   * \brief Get current size of the buffer
   *
   * \return the current size
   */
  std::size_t size() const { return capacity() - numFree(); }

  /**
   * \brief Get number of free spaces in the buffer
   *
   * \return the number free spaces in the buffer
   */
  std::size_t numFree() const {
    if (empty()) {
      return capacity();
    } else if (head_ == tail_) {
      return capacity() - 1;
    } else if (head_ < tail_) {
      return tail_ - head_ - 1;
    } else {
      return capacity() + tail_ - head_ - 1;
    }
  }

  /**
   * \brief Check if the buffer is empty
   *
   * \return whether the buffer is empty or not
   */
  bool empty() const {
    return head_ == invalid_index_ && tail_ == invalid_index_;
  }

  /**
   * \brief Get the maximum number of phases which can be stored
   *
   * \return the maximum number of phases
   */
  std::size_t capacity() const { return vector_.size(); }

  /**
   * \brief Check if the buffer is initialized
   *
   * \return whether the buffer is initialized or not
   */
  bool isInitialized() const { return capacity() > 0; }

  /**
   * \brief Clears content of the buffer including phases and related data. Does not change the buffer maximum capacity.
   */
  void clear() {
    auto new_vec = std::vector<StoredPair>(
      vector_.size(), StoredPair{invalid_, StoredType{}});
    vector_.swap(new_vec);
    resetIndexes();
  }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    s | head_;
    s | tail_;
    s | vector_;
  }

private:
  /**
   * \brief Convert the phase number to related index in the buffer
   *
   * \param[in] phase the phase to convert
   *
   * \return the index to the phase data
   */
  inline std::size_t phaseToPos(const PhaseType phase) const {
    auto go_back = vector_[head_].first - phase;
    if (go_back > head_) {
      return vector_.size() - (go_back - head_);
    }
    return head_ - go_back;
  }

  /**
   * \brief Calculates next valid index in the buffer after the passed index
   *
   * \param[in] index the index to be incremented
   *
   * \return the incremented index
   */
  inline std::size_t getNextEntry(const std::size_t index) const {
    auto next_entry = index + 1;
    if (next_entry == capacity()) {
      next_entry = 0;
    }
    return next_entry;
  }

  /**
   * \brief Update internal indexes to point to the correct spots in the buffer after inserting new element
   */
  inline void moveHeadAndTail() {
    head_ = getNextEntry(head_);
    if (head_ == tail_) {
      tail_ = getNextEntry(tail_);
    } else if (tail_ == invalid_index_) {
      tail_ = 0;
    }
  }

  /**
   * \brief Resets internal indexes to their default position
   */
  inline void resetIndexes() {
    head_ = invalid_index_;
    tail_ = invalid_index_;
  }

private:
  std::size_t head_ = invalid_index_;
  std::size_t tail_ = invalid_index_;
  std::vector<StoredPair> vector_;

  static const constexpr PhaseType invalid_ =
    std::numeric_limits<PhaseType>::max();
  static const constexpr std::size_t invalid_index_ =
    std::numeric_limits<std::size_t>::max();

public:
  /**
 * \struct PhaseIterator
 *
 * \brief The iterator for CircularPhasesBuffer.
 */
  template <typename StoredPair>
  class PhaseIterator {
    using ContainerType = std::vector<StoredPair>;

    ContainerType* buffer_;
    std::size_t pos_, head_;

  public:
    PhaseIterator(
      ContainerType* buff, std::size_t start_pos, std::size_t head_pos)
      : buffer_(buff),
        pos_(start_pos),
        head_(head_pos) { }

    PhaseIterator& operator++() {
      if (pos_ == head_) {
        pos_ = buffer_->size();
        return *this;
      }

      ++pos_;
      if (pos_ == buffer_->size()) {
        pos_ = 0;
      }

      return *this;
    }

    StoredPair& operator*() { return (*buffer_)[pos_]; }

    StoredPair* operator->() { return &(operator*()); }

    bool operator==(const PhaseIterator& other) const {
      return pos_ == other.pos_;
    }

    bool operator!=(const PhaseIterator& other) const {
      return pos_ != other.pos_;
    }
  };

  auto begin() {
    if (empty()) {
      return end();
    }
    return PhaseIterator<StoredPair>(&vector_, tail_, head_);
  }

  auto end() { return PhaseIterator<StoredPair>(&vector_, vector_.size(), vector_.size()); }
};

}}} /* end namespace vt::util::container */

#endif /* INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H */
