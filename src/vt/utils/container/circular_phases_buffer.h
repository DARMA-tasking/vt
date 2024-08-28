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
#include <iterator>

namespace vt { namespace util { namespace container {

/**
 * \struct CircularPhasesBuffer
 *
 * \brief The circular buffer which holds data related to a set of phases.
 */
template <typename StoredType>
class CircularPhasesBuffer {
  using StoredPair = std::pair<PhaseType, StoredType>;

  static const auto no_pos = std::numeric_limits<std::size_t>::max();

public:

  /**
   * \brief Construct a CircularPhasesBuffer
   *
   * \param[in] in_capacity the requested size of the buffer
   */
  CircularPhasesBuffer(std::size_t size_in = 1)
    : buffer_(size_in)
  { }

  /**
   * \brief Construct a CircularPhasesBuffer.
   * The max size of buffer will be equal to the size of the list.
   *
   * \param[in] in_list the initializer list with elements to be put into the buffer
   */
  CircularPhasesBuffer(std::initializer_list<StoredPair> in_list)
    : buffer_(in_list.size()) {

    for (auto pair : in_list) {
      store(pair.first, pair.second);
    }
  }

  /**
   * \brief Check if phase is present in the buffer.
   *
   * \param[in] phase the phase to look for
   *
   * \return whether buffer contains the phase or not
   */
  bool contains(const PhaseType& phase) const {
    return phase < head_phase_ && phase >= (head_phase_ - size());
  }

  /**
   * \brief Get data related to the phase.
   * Insert new data if phase is not present in the buffer.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& operator[](const PhaseType phase) {
    if (!contains(phase)) {
      store(phase, StoredType{});
    }
    return at(phase);
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& at(const PhaseType phase) {
    return buffer_.at(phaseToIndex(phase));
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  const StoredType& at(const PhaseType phase) const {
    return buffer_.at(phaseToIndex(phase));
  }

  /**
   * \brief Find data for a phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return pointer to phase data, nullptr otherwise.
   */
  StoredType* find(const PhaseType& phase) {
    if (contains(phase)) {
      return &buffer_[phaseToIndex(phase)];
    }
    return nullptr;
  }

  /**
   * \brief Find data for a phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return pointer to phase data, nullptr otherwise.
   */
  const StoredType* find(const PhaseType& phase) const {
    if (contains(phase)) {
      return &buffer_[phaseToIndex(phase)];
    }
    return nullptr;
  }

  /**
   * \brief Store data in the buffer.
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  void store(const PhaseType& phase, StoredType data) {
    buffer_[head_] = std::move(data);
    head_ = getNextIndex();
    head_phase_ = phase + 1;
    inserted_ = inserted_ + 1;
  }

  /**
   * \brief Resize buffer to the requested size.
   *
   * \param[in] new_size the requested new size of the buffer
   */
  void resize(const std::size_t new_size) {
    if (new_size == buffer_.size() || new_size == 0) {
      return;
    }
    // temporary vector to copy the elements to retain
    std::vector<StoredType> tmp(new_size);
    // number of elements to copy
    auto num = std::min(new_size, size());

    // copy
    auto to_copy = head_;
    for(std::size_t it = num; it > 0; it--) {
      to_copy = getPrevIndex(to_copy);
      tmp[it - 1] = buffer_[to_copy];
    }

    buffer_.swap(tmp);
    inserted_ = num;
    head_ = getNextIndex(num - 1);
  }

  /**
   * \brief Get number of elements in the buffer.
   *
   * \return the number of elements
   */
  std::size_t size() const {
    return std::min(inserted_, buffer_.size());
  }

  /**
   * \brief Check if the buffer is empty.
   *
   * \return whether the buffer is empty or not
   */
  bool empty() const {
    return inserted_ == 0;
  }

  /**
   * \brief Clears content of the buffer.
   */
  void clear() {
    head_ = 0;
    inserted_ = 0;
    head_phase_ = 0;
    buffer_.assign(buffer_.size(), StoredType{});
  }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    s | head_;
    s | inserted_;
    s | head_phase_;
    s | buffer_;
  }

  /**
   * \brief Get const iterator to the beginning of the buffer
   *
   * \return the const begin iterator
   */
  auto begin() const { return ForwardIterator(head_, findTail(), &buffer_); }

  /**
   * \brief Get const iterator to the space after buffer
   *
   * \return the const end iterator
   */
  auto end() const { return ForwardIterator(); }

  /**
   * \brief Finds the newest phase in the buffer.
   *
   * \return the newest phase in the buffer.
   */
  PhaseType frontPhase() const {
    return inserted_ == 0 ? 0 : head_phase_ - 1;
  }

  /**
   * \brief Get the newest element in buffer.
   *
   * \return the reference to the newest element.
   */
  StoredType& front() {
    return buffer_.at(getPrevIndex(head_));
  }

  /**
   * \brief Finds the oldest phase in the buffer.
   *
   * \return the oldest phase in the buffer.
   */
  PhaseType backPhase() const {
    if (head_ == inserted_) {
      return 0;
    }
    return head_phase_ - buffer_.size();
  }

  /**
   * \brief Get the oldest element in buffer.
   *
   * \return the reference to the oldest element.
   */
  StoredType& back() {
    return buffer_.at(findTail());
  }

private:
  std::size_t findTail() const {
    if (inserted_ == 0) {
      return no_pos;
    } else if (head_ == inserted_) {
      return 0;
    }
    return head_;
  }

  std::size_t phaseToIndex(const PhaseType phase) const {
    std::size_t go_back = head_phase_ - phase;
    if (go_back > head_) {
      return size() - (go_back - head_);
    } else {
      return head_ - go_back;
    }
  }

  std::size_t getPrevIndex(std::size_t index) const {
    std::size_t prev = index - 1;
    if (prev > buffer_.size()) {
      prev = buffer_.size() - 1;
    }
    return prev;
  }

  std::size_t getNextIndex() const {
    std::size_t next = head_ + 1;
    if (next >= buffer_.size()) {
      next = 0;
    }
    return next;
  }

  std::size_t getNextIndex(std::size_t index) const {
    std::size_t next = index + 1;
    if (next >= buffer_.size()) {
      next = 0;
    }
    return next;
  }

  // Number of the next index in buffer
  std::size_t head_ = 0;
  // Counter for inserted phases
  std::size_t inserted_ = 0;
  // Number of the next phase
  PhaseType head_phase_ = 0;

  std::vector<StoredType> buffer_;

public:
  struct ForwardIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = StoredType;
    using reference         = const StoredType&;

    ForwardIterator(std::size_t head, std::size_t index, const std::vector<value_type>* buffer)
      : head_(head), index_(index), buffer_(buffer) {}

    ForwardIterator()
      : head_(no_pos), index_(no_pos), buffer_(nullptr) {}

    reference operator*() { return (*buffer_)[index_]; }

    ForwardIterator& operator++() {
      advance();
      return *this;
    }
    ForwardIterator operator++(int) {
      ForwardIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    bool operator== (const ForwardIterator& it) { return index_ == it.index_; };
    bool operator!= (const ForwardIterator& it) { return index_ != it.index_; };

  private:
    void advance() {
      index_++;
      if (index_ >= buffer_->size()) {
        index_ = 0;
      }
      if (index_ == head_) {
        index_ = no_pos;
      }
    }

    std::size_t head_;
    std::size_t index_;
    const std::vector<value_type>* buffer_;
  };
};

}}} /* end namespace vt::util::container */

#endif /*INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H*/
