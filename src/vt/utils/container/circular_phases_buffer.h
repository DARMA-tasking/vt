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

namespace vt { namespace util { namespace container {

/**
 * \struct CircularPhasesBuffer
 *
 * \brief The circular buffer which holds data related to a set of phases.
 */
template <typename StoredType>
class CircularPhasesBuffer {
  using StoredPair = std::pair<PhaseType, StoredType>;

  constexpr static auto no_phase = std::numeric_limits<PhaseType>::max();
  constexpr static auto no_index = std::numeric_limits<std::size_t>::max();

  static StoredPair makeEmptyPair(const PhaseType& phase = no_phase) {
    return std::make_pair(phase, StoredType{});
  }

public:

  /**
   * \brief Construct a CircularPhasesBuffer
   *
   * \param[in] size_in the requested size of the buffer
   */
  CircularPhasesBuffer(std::size_t size_in = 1)
    : buffer_(size_in, makeEmptyPair())
  {
    vtAssert(size_in > 0, "Size of circular phases buffer needs to be greather than zero.");
  }

  /**
   * \brief Construct a CircularPhasesBuffer.
   *
   * \param[in] in_list the initializer list with elements to be put into the buffer
   */
  CircularPhasesBuffer(std::initializer_list<StoredPair> in_list) {
    const auto& [min, max] = std::minmax(in_list, [](const StoredPair& lhs, const StoredPair& rhs) {
      return lhs.first < rhs.first;
    });
    // Calcuate the size of the buffer to hold all phases including the missing ones
    buffer_.resize((max.first - min.first) + 1, makeEmptyPair());

    for (auto pair : in_list) {
      buffer_[phaseToIndex(pair.first)] = pair;
      updateHead(pair.first);
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
    return buffer_[phaseToIndex(phase)].first == phase;
  }

  /**
   * \brief Get or set data for the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& operator[](const PhaseType phase) {
    auto& pair = buffer_[phaseToIndex(phase)];
    // Create empty data for new phase
    if (pair.first != phase) {
      store(phase, StoredType{});
    }
    return pair.second;
  }

  /**
   * \brief Store data in the buffer.
   *
   * \param[in] phase the phase for which data will be stored
   * \param[in] obj the data to store
   */
  // probably best to get phase and data and store it when it is within valid range of phases
  void store(const PhaseType& phase, StoredType data) {
    vtAssert(canBeStored(phase), "Phase is out of valid range");

    buffer_[phaseToIndex(phase)] = std::make_pair(phase, data);
    updateHead(phase);
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  StoredType& at(const PhaseType phase) {
    vtAssert(contains(phase), "Phase is not stored in the buffer.");
    return buffer_[phaseToIndex(phase)].second;
  }

  /**
   * \brief Get data related to the phase.
   *
   * \param[in] phase the phase to look for
   *
   * \return reference to the stored data
   */
  const StoredType& at(const PhaseType phase) const {
    vtAssert(contains(phase), "Phase is not stored in the buffer.");
    return buffer_[phaseToIndex(phase)].second;
  }

  /**
   * \brief Resize buffer to the requested size.
   * The minimal size of the buffer is 1.
   *
   * \param[in] new_size_in the requested new size of the buffer
   */
  void resize(const std::size_t new_size_in) {
    vtAssert(new_size_in > 0, "Size of circular phases buffer needs to be greather than zero.");
    if (new_size_in == buffer_.size()) {
      return;
    }

    // temporary vector to copy the elements to retain
    std::vector<StoredPair> tmp(new_size_in, makeEmptyPair());
    // number of elements to copy
    auto num = std::min(new_size_in, buffer_.size() - numFree());

    // copy num phases
    auto to_copy = head_phase_;
    while(num > 0 && to_copy != no_phase) {
      if (contains(to_copy)) {
        tmp[calcIndex(to_copy, tmp)] = buffer_[phaseToIndex(to_copy)];
        num--;
      }
      to_copy--;
    }

    buffer_.swap(tmp);
  }

  /**
   * \brief Check if the buffer is empty.
   *
   * \return whether the buffer is empty or not
   */
  bool empty() const {
    return head_phase_ == no_phase;
  }

  /**
   * \brief Get the number of valid elements in the buffer
   *
   * \return the number of valid elements
   */
  std::size_t size() const {
    return buffer_.size() - numFree();
  }

  /**
   * \brief Returns the current capacity of the buffer
   *
   * \return the buffer capacity
   */
  std::size_t capacity() const {
    return buffer_.size();
  }

  /**
   * \brief Clears content of the buffer.
   */
  void clear() {
    head_phase_ = no_phase;
    buffer_.assign(buffer_.size(), makeEmptyPair());
  }

  /**
   * \brief Get the latest phase
   *
   * \return the latest phase
   */
  PhaseType frontPhase() const {
    return head_phase_;
  }

  /**
   * \brief Get the data for the latest phase
   *
   * \return the reference to the latest data
   */
  StoredType& frontData() {
    return at(head_phase_);
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
      return &buffer_[phaseToIndex(phase)].second;
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
      return &buffer_[phaseToIndex(phase)].second;
    }
    return nullptr;
  }

  /**
   * \brief Get const iterator to the first valid element in the buffer
   *
   * \return the const iterator
   */
  auto begin() const { return ++ForwardIterator(&buffer_, no_index); }

  /**
   * \brief Get iterator to the first valid element in the buffer
   *
   * \return the iterator
   */
  auto begin() { return ++ForwardIterator(&buffer_, no_index); }

  /**
   * \brief Get const iterator to the space after the buffer
   *
   * \return the const end iterator
   */
  auto end() const { return ForwardIterator(&buffer_, buffer_.size()); }

  /**
   * \brief Get iterator to the space after the buffer
   *
   * \return the end iterator
   */
  auto end() { return ForwardIterator(&buffer_, buffer_.size()); }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    s | head_phase_;
    s | buffer_;
  }

private:
  /**
   * \brief Converts the phase to the index in the buffer
   *
   * \param phase - the phase to be converted
   * \return the position of the data container in the buffer
   */
  std::size_t phaseToIndex(const PhaseType& phase) const {
    return phase % buffer_.size();
  }

  /**
   * \brief Converts the phase to the index in the temporary buffer
   *
   * @param phase - the phase to be converted
   * @param vector - the buffer to return index for
   * @return the position of the data container in the buffer
   */
  std::size_t calcIndex(const PhaseType& phase, const std::vector<StoredPair>& vector) const {
    return phase % vector.size();
  }

  /**
   * \brief Counts spots in the buffer without a valid phase assigned.
   *
   * @return the number of empty spots
   */
  std::size_t numFree() const {
    return std::count_if(buffer_.begin(), buffer_.end(),
      [](const StoredPair& pair){ return pair.first == no_phase; }
    );
  }

  /**
   * \brief Update the head phase stored in the buffer if necessary
   *
   * \param phase - the phase to compare to \c head_phase_
   */
  void updateHead(const PhaseType& phase) {
    if (head_phase_ == no_phase || phase > head_phase_) {
      head_phase_ = phase;
    }
  }

  bool canBeStored(const PhaseType& phase) const {
    if (!empty() && head_phase_ > buffer_.size()) {
      return phase > head_phase_ - buffer_.size();
    }
    return true;
  }

  PhaseType head_phase_ = no_phase;
  std::vector<StoredPair> buffer_;

public:
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
    void advance() {
      index_++;
      if (index_ == buffer_->size()) {
        return;
      }
      if ((*buffer_)[index_].first == no_phase) {
        advance();
      }
    }

    std::size_t index_;
    std::vector<value_type>* buffer_;
  };
};

}}} /* end namespace vt::util::container */

#endif /*INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H*/
