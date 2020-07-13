/*
//@HEADER
// *****************************************************************************
//
//                              histogram_approx.h
//                           DARMA Toolkit v. 1.0.0
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

#if !defined INCLUDED_VT_UTILS_ADT_HISTOGRAM_APPROX_H
#define INCLUDED_VT_UTILS_ADT_HISTOGRAM_APPROX_H

#include "vt/config.h"

#include <vector>
#include <tuple>
#include <limits>
#include <cmath>
#include <algorithm>

#include <fmt/format.h>

namespace vt { namespace util { namespace adt {

namespace detail {

/**
 * \internal \struct Centroid
 *
 * \brief A centroid in the histogram with a value and count
 */
template <typename T, typename CountT>
struct Centroid {
  using CountType = CountT;

  Centroid() = default;

  /**
   * \brief Construct a new centroid
   *
   * \param[in] in_value the value (p)
   * \param[in] in_count the count (m)
   */
  Centroid(T in_value, CountType in_count)
    : value_(in_value),
      count_(in_count)
  { }

  /**
   * \brief Get the value
   *
   * \return the value (p)
   */
  T getValue() const { return value_; }

  /**
   * \brief Get the count
   *
   * \return the value (m)
   */
  CountType getCount() const { return count_; }

  /**
   * \brief Set the value
   *
   * \param[in] in_value the value
   */
  void setValue(T in_value) { value_ = in_value; }

  /**
   * \brief Set the count
   *
   * \param[in] in_count the count
   */
  void setCount(CountType in_count) { count_ = in_count; }

  /**
   * \brief Add to the count
   *
   * \param[in] in_add amount to add
   */
  void addCount(CountType in_add) { count_ += in_add; }

  /**
   * \brief Merge in another centroid
   *
   * \param[in] in the other centroid to merge
   */
  void merge(Centroid<T, CountT> const& in) {
    auto const new_count = count_ + in.count_;
    auto const new_value = ((value_*count_) + (in.value_*in.count_))/new_count;
    count_ = new_count;
    value_ = new_value;
    exact_ = false;
  }

  /**
   * \brief Return if this centroid is exact or approximated
   *
   * \return whether it is exact
   */
  bool isExact() const { return exact_; }

  /**
   * \brief Compare a centroid to a double using the value
   *
   * \param[in] c1 the centroid
   * \param[in] val the value (a double) to compare
   *
   * \return whether it is less than the value
   */
  friend bool operator<(Centroid<T, CountT> const& c1, double val) {
    return c1.value_ < val;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | value_;
    s | count_;
    s | exact_;
  }

private:
  T value_ = {};                /**< The value (p) */
  CountType count_ = 0;         /**< The count (m) */
  bool exact_ = true;           /**< Whether this centroid is approximate */
};

} /* end namespace detail */

/**
 * \struct HistogramApprox
 *
 * \brief A bounded-size, online/streaming approximate histogram.
 *
 * This is an implementation of the paper "A Streaming Parallel Decision Tree
 * Algorithm" by Ben-Haim and Tom-Tov:
 *
 *   http://www.jmlr.org/papers/volume11/ben-haim10a/ben-haim10a.pdf
 *
 * The "histogram sketch" (as the authors call it) approximates a histogram with
 * a fixed/maximum number of of centroids (value, count) sorted in ascending
 * order. Each time a new value, V, is added, it is either added to an existing
 * centroid (if the value matches exactly) by increasing the count; or, a new
 * centroid (V, 1) is created. After creation of a new centroid, if at the
 * centroid count limit, the best pair of centroids (ones with the minimum
 * distance) are merged, weighted by their values and associated
 * counts. Quantiles and counts are estimated by finding bordering centroids and
 * using them to estimate the area of a trapezoid.
 */
template <typename T, typename CountT>
struct HistogramApprox {
  using CountType = CountT;
  using CentroidType = detail::Centroid<T, CountT>;

  HistogramApprox() = default; // for serialization

  /**
   * \brief Construct a new approximate histogram with a bound on storage
   *
   * \param[in] in_max_centroids max number of centroids (bounds space)
   */
  explicit HistogramApprox(CountType in_max_centroids)
    : max_centroids_(in_max_centroids)
  { }

  /**
   * \brief Get the maximum value ever inserted in the histogram
   *
   * \return the max value inserted
   */
  T getMax() const { return max_; }

  /**
   * \brief Get the minimum value ever inserted in the histogram
   *
   * \return the min value inserted
   */
  T getMin() const { return min_; }

  /**
   * \brief Get the total number of values inserted
   *
   * \note This is not the size of the histogram as it is limited to
   * \c max_centroids_
   *
   * \return the number of values inserted
   */
  CountType getCount() const { return total_count_; }

  /**
   * \brief Add a value to the histogram
   *
   * \param[in] value the value
   * \param[in] num_times how many times to add it (default 1)
   */
  void add(T value, CountType num_times = 1) {
    if (value < min_) { min_ = value; }
    if (value > max_) { max_ = value; }

    // add \c num_times to total count
    total_count_ += num_times;

    // Find an index k such that value_k <= value < value_{k+1} using a binary
    // search on the underlying centroids
    auto it = std::lower_bound(cs_.begin(), cs_.end(), value);
    std::size_t k = it - cs_.begin();

    // If we found a valid value and it's exactly equal, we are done!
    if (k < cs_.size() and cs_[k].getValue() == value) {
      cs_[k].addCount(num_times);
      return;
    }

    // insert an element right before current position
    auto iter = cs_.begin() + k;
    cs_.insert(iter, CentroidType{value, num_times});

    if (cs_.size() <= static_cast<std::size_t>(max_centroids_)) {
      // not at capacity, nothing to merge
      return;
    }

    // find the best candidate for merging two centroids since we are over the
    // space limit
    auto min_idx = findMinimumCentroidPair();
    // merge them
    cs_[min_idx].merge(cs_[min_idx+1]);
    // shift centroids in place
    for (auto j = min_idx + 1; j < cs_.size() - 1; j++) {
      cs_[j] = cs_[j+1];
    }
    // remove the last element, now dead
    cs_.pop_back();
  }

  /**
   * \brief Estimate the p'th quantile defined as the smallest data point, d,
   * such that p*total_count_ <= d
   *
   * \param[in] p the quantile between [0.0, 1.0]
   *
   * \return data point, d
   */
  double quantile(double const p) {
    vtAssert(!(p < 0.0 || p > 1.0), "percentile must be between 0.0 and 1.0");
    vtAssert(total_count_ >= 1, "Must have at least one value to calculate");

    double const t = p * total_count_;
    double s = 0.;
    double pv = 0.;

    // Find the last two consecutive centroids (ci, cj) such that the sum of all
    // centroid weights up to ci plus half of cj is at most t. This implies the
    // quantile is between ci and cj
    std::size_t i = 0;
    for ( ; i < cs_.size(); i++) {
      double v = cs_[i].getCount() / 2.;
      if (s + pv + v > t) {
        break;
      }
      s += v + pv;
      pv = v;
    }

    // Border centroids give a more precise approximation of the quantiles
    auto centroids = getBorderCentroids(i);
    auto ci = std::get<0>(centroids);
    auto cj = std::get<1>(centroids);

    // If we found exact centroids, return the value
    if (ci.isExact() and cj.isExact()) {
      return cj.getValue();
    }

    // See paper; we are solving a quadratic.
    // Let cx.p = cx.getValue(), cx.m = cx.getCount()
    // solve for u such that t-s = (ci.m + mu)/2 * (u-ci.p)/(cj.p - ci.p),
    // where mu = ci.m + (u-ci.p)*(cj.m - ci.m)/(cj.p - ci.p).
    auto const mi = ci.getCount();
    auto const pi = ci.getValue();
    auto const mj = cj.getCount();
    auto const pj = cj.getValue();
    double const d = t - s;
    double const a = mj - mi;
    if (a == 0) {
      return pi + (pj - pi) * (d / mi);
    }
    double const b = 2.0 * ci.getCount();
    double const c = -2.0 * d;
    double const z = (-b + std::sqrt(b * b - 4 * a * c)) / (2 * a);
    return pi + (pj - pi) * z;
  }

  /**
   * \brief Estimate the number of values <= p in the histogram
   *
   * This is implemented based on the math in the referenced paper in algorithm
   * 3 "Sum Procedure".
   *
   * \param[in] p the value
   *
   * \return number of values <= p
   */
  double estimateNumValues(double p) {
    if (p >= max_) { return total_count_; }
    if (p < min_) { return 0; }

    // Find an index k such that p_k <= p < p_{k+1}
    auto it = std::lower_bound(cs_.begin(), cs_.end(), p);
    std::size_t k = it - cs_.begin();

    // \c std::lower_bound doesn't quite do what we need (when p == p_k)
    if (cs_[k].getValue() == p) {
      k++;
    }

    //
    // The following is equivalent to what the above code with
    // \c std::lower_bound does:
    //
    // std::size_t k = 0;
    // for (; k < cs_.size(); k++) {
    //   if (cs_[k].getValue() > p) {
    //     break;
    //   }
    // }

    double s = 0.;
    if (k > 0) {
      for (std::size_t i = 0; i < k - 1; i++) {
        s += cs_[i].getCount();
      }
    }

    auto centroids = getBorderCentroids(k);
    auto ci = std::get<0>(centroids);
    auto cj = std::get<1>(centroids);
    auto const mi = ci.getCount();
    auto const pi = ci.getValue();
    auto const mj = cj.getCount();
    auto const pj = cj.getValue();

    // This is an improvement on the paper when we know the estimation is exact
    // because it was never merged
    if (ci.isExact() && (cj.isExact() || pi == p)) {
      return s + mi;
    }

    // Compute the formula for approximating across the centroids
    double x = (p - pi) / (pj - pi);
    double b = mi + (mj - mi) * x;
    double est = s + mi / 2.0 + (mi + b) * x / 2.0;
    return est;
  }

  /**
   * \brief Merge in another histogram
   *
   * \param[in] in_hist the histogram to merge in
   */
  void mergeIn(HistogramApprox<T, CountT> const& in_hist) {
    auto const& in_cs = in_hist.getCentroids();

    // These are sorted, so just walk through and add them. There is probably
    // some optimization we can apply here since we know all of them, but for
    // now just add them incrementally.
    for (auto&& c : in_cs) {
      add(c.getValue(), c.getCount());
    }
  }

  /**
   * \brief Build a histogram with fixed buckets, each bucket containing
   * the number of values in that range.
   *
   * If \c n_buckets == 4, the buckets segments will be:
   *   { [0, 0.25); [0.25, 0.5); [0.5, 0.75); [0.75, 1.0) }
   *
   * Then, for each one of these bucket segments, the number of values that fall
   * in that proportion of the range of values.
   *
   * \note This can be used to output a mini-histogram of where the values fall.
   *
   * \param[in] n_buckets the number of buckets
   */
  std::vector<CountT> computeFixedBuckets(int n_buckets) {
    auto const r = getMax() - getMin();
    auto const bucket_size = r / n_buckets;

    std::vector<double> buckets;
    for (auto i = 0; i < n_buckets; i++) {
      auto q = getMin() + bucket_size*(i+1);
      // get the number of values that fall less than this value
      auto n_vals = estimateNumValues(q);
      buckets.push_back(n_vals);
    }

    std::vector<CountT> segments;
    segments.resize(n_buckets);
    for (auto i = n_buckets - 1; i >= 0; i--) {
      if (i == 0) {
        segments[i] = std::round(buckets[i]);
      } else {
        segments[i] = std::round(buckets[i] - buckets[i-1]);
      }
    }

    return segments;
  }

  /**
   * \internal \brief Get the actual centroid array that makes up the histogram
   *
   * \return the centroids
   */
  std::vector<CentroidType> const& getCentroids() const { return cs_; }

  /**
   * \internal \brief Dump the histogram for debugging
   */
  void dump() {
    vt_print(
      gen,
      "dump: count={}, min={}, max={}, cs_.size={}\n",
      total_count_, min_, max_, cs_.size()
    );
    for (std::size_t i = 0; i < cs_.size(); i++) {
      vt_print(
        gen,
        "\t i={} value={}, count={}\n", i, cs_[i].getValue(), cs_[i].getCount()
      );
    }
  }

  /**
   * \brief Build a readable string that contains all internal data for testing
   */
  std::string buildContainerString() {
    auto container_info = fmt::format("{} {} {} [", total_count_, min_, max_);
    std::size_t i = 0;
    for (auto&& c : cs_) {
      container_info += fmt::format("{{{} {}}}", c.getValue(), c.getCount());
      container_info += i != cs_.size() - 1 ? " " : "]";
      i++;
    }
    return container_info;
  }

  /**
   * \brief Serialize the centroid
   *
   * \param[in] s the serializer
   */
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | max_centroids_;
    s | total_count_;
    s | min_;
    s | max_;
    s | cs_;
  }

private:
  /**
   * \internal \brief Find the best candidate for merging two neighboring
   * centroids in the histogram
   *
   * \return the index, \c i, to merge with the neighbor \c i+1
   */
  std::size_t findMinimumCentroidPair() const {
    T min_dist = std::numeric_limits<T>::max();
    std::size_t min_index = cs_.size();
    for (std::size_t i = 0; i < cs_.size() - 1; i++) {
      // Walk through sorted centroids calculating the distance between: cs_[i]
      // and cs_[+1]; return the best match for merging (min distance)
      auto dist = std::fabs(cs_[i+1].getValue() - cs_[i].getValue());
      if (dist < min_dist) {
        min_dist = dist;
        min_index = i;
      }
    }
    vtAssert(min_index != cs_.size(), "Must be valid index");
    return min_index;
  }

  /**
   * \internal \brief Get the border centroids of a given i'th centroid
   * returning the (i-1)'th and i'th centroids
   *
   * Because we know the exact min/max values in the histogram (no deletions),
   * even after the centroids are averaged, we can give a more precise border
   * centroid when we are on the boundaries.
   *
   * \param[in] i the centroid
   *
   * \return a tuple that gives a good neighboring centroid
   */
  std::tuple<CentroidType, CentroidType> getBorderCentroids(std::size_t i) const {
    if (i == 0) {
      return std::make_tuple(CentroidType{min_, 0}, cs_[0]);
    } else if (i == cs_.size()) {
      return std::make_tuple(cs_[cs_.size()-1], CentroidType{max_, 0});
    } else {
      return std::make_tuple(cs_[i-1], cs_[i]);
    }
  }

private:
  CountType max_centroids_ = 0;              /**< Max centroids allowed */
  CountT total_count_ = 0;                   /**< Total number of values added */
  T min_ = std::numeric_limits<T>::max();    /**< Smallest value added */
  T max_ = std::numeric_limits<T>::lowest(); /**< Largest value added */
  std::vector<CentroidType> cs_;             /**< Array of centroids */
};

}}} /* end namespace vt::util::adt */

namespace vt { namespace adt {

template <typename T, typename CountT>
using HistogramApprox = util::adt::HistogramApprox<T, CountT>;

}} /* end namespace vt::adt */

#endif /*INCLUDED_VT_UTILS_ADT_HISTOGRAM_APPROX_H*/
