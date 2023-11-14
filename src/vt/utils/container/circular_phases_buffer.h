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

constexpr PhaseType invalid_ = -1;

// Circular Buffer which can store continuous phases data
template<typename StoredType>
struct CircularPhasesBuffer {
    using StoredPair = std::pair<PhaseType, StoredType>;

    CircularPhasesBuffer(std::size_t capacity = 0)
    : head_phase_(-1), vector_(capacity, StoredPair{invalid_, StoredType{}})
    { }

    CircularPhasesBuffer(std::initializer_list<StoredPair> list) {
        vector_.resize(list.size());
        for (auto&& pair : list) {
            store(pair.first, std::move(pair.second));
        }
    }

    // store, override StoredType if present on the same index
    void store(const PhaseType phase, StoredType&& obj) {
        vector_[phase % vector_.size()] = std::make_pair(phase, std::move(obj));
        head_phase_ = phase;
    }

    // store, override StoredType if present on the same index
    void store(const PhaseType phase, StoredType const& obj) {
        vector_[phase % vector_.size()] = std::make_pair(phase, obj);
        head_phase_ = phase;
    }

    StoredType& emplace(const PhaseType phase) {
        store(phase, StoredType{});
        return vector_[phase % vector_.size()].second;
    }

    const StoredType* find(const PhaseType phase) const {
        if (contains(phase)) {
            return &vector_[phase % vector_.size()].second;
        }
        return nullptr;
    }

    StoredType* find(const PhaseType phase) {
        if (contains(phase)) {
            return &vector_[phase % vector_.size()].second;
        }
        return nullptr;
    }

    // map style operator - get reference to exsiting or newly inserted element
    StoredType& operator[](const PhaseType phase) {
        if (!contains(phase)) {
            store(phase, StoredType{});
        }
        return vector_[phase % vector_.size()].second;
    }

    bool contains(const PhaseType phase) const {
        return vector_[phase % vector_.size()].first == phase;
    }

    const StoredType& at(const PhaseType phase) const {
        vtAssert(contains(phase), "Buffer don't contain requested phase.");

        return vector_[phase % vector_.size()].second;
    }

    StoredType& at(const PhaseType phase) {
        vtAssert(contains(phase), "Buffer don't contain requested phase.");

        return vector_[phase % vector_.size()].second;
    }

    std::size_t size() const {
        return std::count_if(vector_.begin(), vector_.end(), [](const StoredPair& pair){
            return pair.first != invalid_;
        });
    }

    void resize(std::size_t new_size) {
        vtAssert(new_size != 0, "Logic error: Trying to resize buffer to size 0.");
        if (new_size == vector_.size()) { return; }

        auto new_vec = std::vector<StoredPair>(new_size, StoredPair{invalid_, StoredType{}});

        if (new_size < vector_.size()) {
            std::size_t count = 0;
            std::size_t index = head_phase_ % vector_.size();

            for(; count < new_size; index--, count++) {
                auto pair = vector_[index];
                if (pair.first != invalid_) {
                    new_vec[pair.first % new_size] = std::move(pair);
                }

                if (index == 0) {
                    index = vector_.size();
                }
            }
        } else {
            for(auto pair : vector_) {
                if (pair.first != invalid_) {
                    new_vec[pair.first % new_size] = std::move(pair);
                }
            }
        }

        vector_.swap(new_vec);
    }

    void clear() {
        auto new_vec = std::vector<StoredPair>(vector_.size(), StoredPair{invalid_, StoredType{}});
        vector_.swap(new_vec);
        head_phase_ = 0;
    }

    template<typename SerializeT>
    void serialize(SerializeT& s) {
        s | head_phase_;
        s | vector_;
    }

private:
    PhaseType head_phase_;
    std::vector<StoredPair> vector_;
};

}}} /* end namespace vt::util::container */

#endif /* INCLUDED_VT_UTILS_CONTAINER_CIRCULAR_PHASES_BUFFER_H */
