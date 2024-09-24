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
#include <vector>
#include <algorithm>

namespace vt { namespace util { namespace container {

/**
 * \struct CircularPhasesBuffer
 *
 * \brief A circular buffer for storing and accessing data with phase-based indexing.
 *
 * The buffer supports a valid range of phases that spans from `frontPhase() - capacity()`
 * to `frontPhase()`, allowing both random access and insertion of new phases within this window.
 * Phases outside this range, i.e., those before the window, are not guaranteed to remain accessible.
 * Inserting a phase greater than the current `frontPhase()` will automatically advance the
 * `frontPhase()`, shifting the valid window forward to accommodate the new phase.
 */
template <typename StoredType>
class CircularPhasesBuffer {
  using StoredPair = std::pair<PhaseType, StoredType>;

  constexpr static auto no_phase = std::numeric_limits<PhaseType>::max();
  constexpr static auto no_index = std::numeric_limits<std::size_t>::max();

  /**
   * \brief Create an empty phase-data pair for initializing the buffer.
   *
   * \param[in] phase The phase to assign to the pair (defaults to \c no_phase if not provided)
   * \return A new pair containing the phase and an empty \c StoredType object.
   */
  static StoredPair makeEmptyPair(const PhaseType& phase = no_phase) {
    return std::make_pair(phase, StoredType{});
  }

public:

  /**
   * \brief Constructor to initialize the circular buffer with a specified size.
   *
   * \param[in] size_in The size of the buffer (must be greater than zero).
   */
  CircularPhasesBuffer(std::size_t size_in = 1)
    : buffer_(size_in, makeEmptyPair())
  {
    vtAssert(size_in > 0, "Size of CircularPhasesBuffer needs to be greather than zero.");
  }

  /**
   * \brief Check if a given phase exists in the buffer.
   *
   * \param[in] phase The phase to check for existence.
   * \return \c true if the phase is present, otherwise \c false.
   */
  bool contains(const PhaseType& phase) const {
    return buffer_[phaseToIndex(phase)].first == phase;
  }

  /**
   * \brief Access data for a given phase. If the phase is not present, it inserts a new element.
   * This method can abort the process if the phase is out of range (check \c canBeStored()).
   *
   * \param[in] phase The phase to retrieve data for.
   * \return A reference to the stored data for the phase.
   */
  StoredType& operator[](const PhaseType phase) {
    auto& pair = buffer_[phaseToIndex(phase)];
    // Inserts empty data for new phase
    if (pair.first != phase) {
      store(phase, StoredType{});
    }
    return pair.second;
  }

  /**
   * \brief Store data for a given phase in the buffer.
   * Can abort the process if the phase is out of the valid range (check \c canBeStored()).
   *
   * \param[in] phase The phase for which data will be stored.
   * \param[in] data The data to store.
   */
  void store(const PhaseType& phase, StoredType data) {
    vtAssert(canBeStored(phase), "Phase is out of valid range");

    buffer_[phaseToIndex(phase)] = std::make_pair(phase, data);
    updateHead(phase);
  }

  /**
   * \brief Retrieve data for a specific phase.
   *
   * \param[in] phase The phase to retrieve.
   * \return A reference to the stored data.
   * \throw Asserts if the phase is not present in the buffer.
   */
  StoredType& at(const PhaseType phase) {
    vtAssert(contains(phase), "Phase is not stored in the buffer.");
    return buffer_[phaseToIndex(phase)].second;
  }

  /**
   * \brief Retrieve data for a specific phase.
   *
   * \param[in] phase The phase to retrieve.
   * \return A const reference to the stored data.
   * \throw Asserts if the phase is not present in the buffer.
   */
  const StoredType& at(const PhaseType phase) const {
    vtAssert(contains(phase), "Phase is not stored in the buffer.");
    return buffer_[phaseToIndex(phase)].second;
  }

  /**
   * \brief Resize the buffer to a new size.
   * The buffer's minimal size is 1.
   *
   * \param[in] new_size_in The requested new size of the buffer.
   */
  void resize(const std::size_t new_size_in) {
    auto new_size = std::max(std::size_t{1}, new_size_in);
    if (new_size == buffer_.size()) {
      return;
    }

    // temporary vector to copy the elements to retain
    std::vector<StoredPair> tmp(new_size, makeEmptyPair());
    // number of elements to copy
    auto num = std::min(new_size, buffer_.size() - numFree());

    // copy data which should be retained
    auto to_copy = head_phase_;
    while(num > 0 && to_copy != no_phase) {
      if (contains(to_copy)) {
        tmp[to_copy % tmp.size()] = buffer_[phaseToIndex(to_copy)];
        num--;
      }
      to_copy--;
    }

    buffer_.swap(tmp);
  }

  /**
   * \brief Check if the buffer is empty (i.e., contains no valid phases).
   *
   * \return \c true if the buffer is empty, otherwise \c false.
   */
  bool empty() const {
    return head_phase_ == no_phase;
  }

  /**
   * \brief Get the number of valid elements currently stored in the buffer.
   *
   * \return The number of valid elements in the buffer.
   */
  std::size_t size() const {
    return buffer_.size() - numFree();
  }

  /**
   * \brief Get the total capacity of the buffer.
   *
   * \return The capacity of the buffer.
   */
  std::size_t capacity() const {
    return buffer_.size();
  }

  /**
   * \brief Clear all elements in the buffer, resetting it to an empty state.
   */
  void clear() {
    head_phase_ = no_phase;
    buffer_.assign(buffer_.size(), makeEmptyPair());
  }

  /**
   * \brief Restart the buffer from a specific phase, shifting the head.
   * This does not remove any existing data.
   *
   * \param start_point The phase to start from.
   */
  void restartFrom(const PhaseType& start_point) {
    if (empty()) {
      return;
    }

    // copy data from head_phase_
    if (phaseToIndex(head_phase_) != phaseToIndex(start_point)) {
      buffer_[phaseToIndex(start_point)].second = buffer_[phaseToIndex(head_phase_)].second;
    }

    buffer_[phaseToIndex(start_point)].first = start_point;
    head_phase_ = start_point;
  }

  /**
   * \brief Retrieve the most recent phase in the buffer.
   *
   * \return The phase at the front of the buffer.
   */
  PhaseType frontPhase() const {
    return head_phase_;
  }

  /**
   * \brief Retrieve the data associated with the most recent phase.
   *
   * \return A reference to the data for the latest phase.
   * \throw Asserts if the buffer is empty.
   */
  StoredType& frontData() {
    return at(head_phase_);
  }

  /**
   * \brief Find data associated with a phase.
   *
   * \param[in] phase The phase to search for.
   * \return A pointer to the data if the phase is present, otherwise \c nullptr.
   */
  StoredType* find(const PhaseType& phase) {
    if (contains(phase)) {
      return &buffer_[phaseToIndex(phase)].second;
    }
    return nullptr;
  }

  /**
   * \brief Find data associated with a phase.
   *
   * \param[in] phase The phase to search for.
   * \return A const pointer to the data if the phase is present, otherwise \c nullptr.
   */
  const StoredType* find(const PhaseType& phase) const {
    if (contains(phase)) {
      return &buffer_[phaseToIndex(phase)].second;
    }
    return nullptr;
  }

  /**
   * \brief Get a const iterator to the first valid element in the buffer.
   *
   * \return A const iterator to the first valid element.
   */
  auto begin() const { return ++ForwardIterator(&buffer_, no_index); }

  /**
   * \brief Get an iterator to the first valid element in the buffer.
   *
   * \return An iterator to the first valid element.
   */
  auto begin() { return ++ForwardIterator(&buffer_, no_index); }

  /**
   * \brief Get a const iterator to the end of the buffer (one past the last element).
   *
   * \return A const iterator to the end of the buffer.
   */
  auto end() const { return ForwardIterator(&buffer_, buffer_.size()); }

  /**
   * \brief Get an iterator to the end of the buffer (one past the last element).
   *
   * \return An iterator to the end of the buffer.
   */
  auto end() { return ForwardIterator(&buffer_, buffer_.size()); }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    s | head_phase_;
    s | buffer_;
  }

private:
  /**
   * \brief Convert a phase number to its corresponding index in the buffer.
   *
   * \param[in] phase The phase to convert.
   * \return The buffer index for the phase.
   */
  std::size_t phaseToIndex(const PhaseType& phase) const {
    return phase % buffer_.size();
  }

  /**
   * \brief Count the number of empty (without a valid phase) slots in the buffer.
   *
   * \return The number of empty slots.
   */
  std::size_t numFree() const {
    return std::count_if(buffer_.begin(), buffer_.end(),
      [](const StoredPair& pair){ return pair.first == no_phase; }
    );
  }

  /**
   * \brief Update the head phase if a newer phase is stored.
   *
   * \param[in] phase The phase being stored.
   */
  void updateHead(const PhaseType& phase) {
    if (head_phase_ == no_phase || phase > head_phase_) {
      head_phase_ = phase;
    }
  }

  /**
   * \brief Check whether a given phase can be stored in the buffer.
   * A phase is valid if it is greater than \c (head_phase_ - capacity()).
   *
   * \param[in] phase The phase to validate.
   * \return \c true if the phase can be stored, otherwise \c false.
   */
  bool canBeStored(const PhaseType& phase) const {
    if (!empty() && head_phase_ > capacity()) {
      return phase > head_phase_ - capacity();
    }
    return true;
  }

  PhaseType head_phase_ = no_phase; ///< The most recent phase in the buffer.
  std::vector<StoredPair> buffer_; ///< The underlying data buffer.

public:
  /**
   * \struct ForwardIterator
   *
   * \brief An iterator for traversing the buffer in a forward direction, skipping empty slots.
   */
  struct ForwardIterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type   = std::ptrdiff_t;
    using value_type        = StoredPair;
    using reference         = StoredPair&;

    ForwardIterator(std::vector<value_type>* buffer, std::size_t index)
      : index_(index), buffer_(buffer) { }

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

    bool operator== (const ForwardIterator& it) const { return index_ == it.index_; };
    bool operator!= (const ForwardIterator& it) const { return index_ != it.index_; };

  private:
    /**
     * \brief Move the iterator to the next element with a valid phase.
     */
    void advance() {
      index_++;
      if (index_ == buffer_->size()) {
        return;
      }
      if ((*buffer_)[index_].first == no_phase) {
        advance();
      }
    }

    std::size_t index_; ///< The current index in the buffer.
    std::vector<value_type>* buffer_; ///< Pointer to the buffer being iterated over.
  };
};

}}} /* end namespace vt::util::container */

#endif /*INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H*/
