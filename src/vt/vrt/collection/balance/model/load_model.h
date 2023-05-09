/*
//@HEADER
// *****************************************************************************
//
//                                 load_model.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_LOAD_MODEL_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_MODEL_LOAD_MODEL_H

#include "vt/config.h"
#include "vt/timing/timing_type.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/elm/elm_comm.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

struct ObjectIteratorImpl {
  using value_type = ElementIDStruct;

  ObjectIteratorImpl() = default;
  virtual ~ObjectIteratorImpl() = default;

  virtual void operator++() = 0;
  virtual value_type operator*() const = 0;
  virtual bool isValid() const = 0;
};

struct ObjectIterator {
  ObjectIterator(std::unique_ptr<ObjectIteratorImpl>&& in_impl)
    : impl(std::move(in_impl))
  { }
  ObjectIterator(std::nullptr_t)
    : impl(nullptr)
  { }
  void operator++() {
    vtAssert(isValid(), "Can only increment a valid iterator");
    ++(*impl);
  }
  ElementIDStruct operator*() const {
    vtAssert(isValid(), "Can only increment a valid iterator");
    return **impl;
  }
  bool isValid() const { return impl && impl->isValid(); }
  bool operator!=(const ObjectIterator& rhs) const {
    vtAssert(rhs.impl == nullptr, "Can only compare against an end() iterator");
    return isValid();
  }

private:
  std::unique_ptr<ObjectIteratorImpl> impl;
};

struct LoadMapObjectIterator : public ObjectIteratorImpl {
  using map_iterator_type = LoadMapType::const_iterator;
  using iterator_category = std::iterator_traits<map_iterator_type>::iterator_category;
  map_iterator_type i, end;

  LoadMapObjectIterator(map_iterator_type in, map_iterator_type in_end)
    : i(in), end(in_end)
  { }
  void operator++() override { ++i; }
  value_type operator*() const override { return i->first; }
  bool isValid() const override { return i != end; }
};

struct DualLoadMapObjectIterator : public ObjectIteratorImpl {
  using DualLoadMapType = std::unordered_map<
    ElementIDStruct, std::tuple<LoadSummary, LoadSummary>
  >;
  using map_iterator_type = DualLoadMapType::const_iterator;
  using iterator_category = std::iterator_traits<map_iterator_type>::iterator_category;
  map_iterator_type i, end;

  DualLoadMapObjectIterator(map_iterator_type in, map_iterator_type in_end)
    : i(in), end(in_end)
  { }
  void operator++() override { ++i; }
  value_type operator*() const override { return i->first; }
  bool isValid() const override { return i != end; }
};

struct FilterIterator : public ObjectIteratorImpl {
  FilterIterator(
    ObjectIterator&& in_it, std::function<bool(ElementIDStruct)>&& in_filter
  ) : it(std::move(in_it))
    , filter(std::move(in_filter))
  {
    advanceToNext();
  }

  // Satisfy the invariant at initialization or increment
  void advanceToNext() {
    while (it.isValid() && !filter(*it)) {
      ++it;
    }
  }

  void operator++() override {
    ++it;
    advanceToNext();
  }
  value_type operator*() const override { return *it; }
  bool isValid() const override { return it.isValid(); }

  // Invariant: points either to an element that satisfies filter, or to end
  ObjectIterator it;
  std::function<bool(ElementIDStruct)> filter;
};

struct ConcatenatedIterator : public ObjectIteratorImpl {
  ConcatenatedIterator(ObjectIterator&& in_it1, ObjectIterator&& in_it2)
    : it1(std::move(in_it1))
    , it2(std::move(in_it2))
  { }

  void operator++() override
  {
    if (it1.isValid()) {
      ++it1;
      return;
    }

    if (it2.isValid()) {
      ++it2;
    }
  }

  value_type operator*() const override
  {
    if (it1.isValid())
      return *it1;
    else
      return *it2;
  }

  bool isValid() const override
  {
    return it1.isValid() || it2.isValid();
  }

  ObjectIterator it1, it2;
};

/**
 * \brief Interface for transforming measurements of past object loads
 * into predictions of future object load for load balancing
 * strategies
 */
struct LoadModel
{
  LoadModel() = default;
  virtual ~LoadModel() = default;

  /**
   * \brief Initialize the model instance with pointers to the measured load data
   *
   * This would typically be called by LBManager when the user has
   * passed a new model instance for a collection
   */
  virtual void setLoads(
    std::unordered_map<PhaseType, LoadMapType> const* proc_load,
    std::unordered_map<PhaseType, CommMapType> const* proc_comm
  ) = 0;

  /**
   * \brief Signals that load data for a new phase is available
   *
   * For models that want to do pre-computation based on measured
   * loads before being asked to provide predictions from them
   *
   * This would typically be called by LBManager collectively inside
   * an epoch that can be used for global communication in advance of
   * any calls to getModeledLoad()
   *
   * The `setLoads` method must have been called before any call to
   * this.
   */
  virtual void updateLoads(PhaseType last_completed_phase) = 0;

  /**
   * \brief Provide an estimate of the given object's load during a specified interval
   *
   * \param[in] object The object whose load is desired
   * \param[in] when The interval in which the estimated load is desired
   *
   * \return How much computation time the object is estimated to require
   *
   * The `updateLoads` method must have been called before any call to
   * this.
   */
  virtual TimeType getModeledLoad(ElementIDStruct object, PhaseOffset when) const = 0;

  /**
   * \brief Whether or not the model is based on the RawData model
   */
  virtual bool hasRawLoad() const { return false; }

  /**
   * \brief Provide the given object's raw load during a specified interval
   *
   * \param[in] object The object whose raw load is desired
   * \param[in] when The interval in which the raw load is desired
   *
   * \return How much computation time the object required
   */
  virtual TimeType getRawLoad(ElementIDStruct object, PhaseOffset when) const {
    vtAbort(
      "LoadModel::getRawLoad() called on a model that does not implement it"
    );
    return 0.0;
  };

  /**
   * \brief Provide an estimate of the communication cost for a given object
   * during a specified interval
   *
   * \param[in] object The object whose communication is desired
   * \param[in] when The interval in which the communication takes place
   *
   * \return How much communication time the object is estimated to require
   *
   * The `updateLoads` method must have been called before any call to
   * this.
   */
  virtual TimeType getModeledComm(ElementIDStruct object, PhaseOffset when) const {
    return {};
  }

  /**
   * \brief Compute how many phases of past load statistics need to be
   * kept available for the model to use
   *
   * \param[in] look_back How many phases into the past the caller
   * intends to query
   *
   * \return How many phases of past load statistics will be needed to
   * satisfy the requested history
   */
  virtual unsigned int getNumPastPhasesNeeded(unsigned int look_back = 0) const = 0;

  /**
   * Object enumeration, to abstract away access to the underlying structures
   * from NodeLBData
   *
   * The `updateLoads` method must have been called before any call to
   * this.
   */
  virtual ObjectIterator begin() const = 0;

  /**
   * Object enumeration, to abstract away access to the underlying structures
   * from NodeLBData
   *
   * The `updateLoads` method must have been called before any call to
   * this.
   */
  ObjectIterator end() const { return ObjectIterator{nullptr}; }

  /**
   * Object enumeration, to abstract away access to the underlying structures
   * from NodeLBData
   *
   * The `updateLoads` method must have been called before any call to
   * this.
   */
  virtual int getNumObjects() const {
    int count = 0;
    for (auto it = begin(); it != end(); ++it, ++count) {}
    return count;
  }

  /**
   * Returns the number of phases of history available
   *
   * The `updateLoads` method must have been called before any call to
   * this.
   */
  virtual unsigned int getNumCompletedPhases() const = 0;

  /**
   * Returns the number of subphases recorded in the most recent
   * completed phase
   *
   * The `updateLoads` method must have been called before any call to
   * this.
   */
  virtual int getNumSubphases() const = 0;

  template <typename Serializer>
  void serialize(Serializer& s) {}
}; // struct LoadModel

}}}} // namespaces

#endif
