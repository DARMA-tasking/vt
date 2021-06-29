/*
//@HEADER
// *****************************************************************************
//
//                             linear_regression.h
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

#if !defined INCLUDED_VT_UTILS_STATS_LINEAR_REGRESSION_H
#define INCLUDED_VT_UTILS_STATS_LINEAR_REGRESSION_H

#include "vt/config.h"

#include <vector>
#include <numeric>

namespace vt { namespace util { namespace stats {

/**
 * \struct LinearRegression
 *
 * \brief Perform a simple linear regression to predict values with a linear
 * model
 */
struct LinearRegression {

  /**
   * \brief Construct a linear regression
   *
   * \param[in] in_x the x-values
   * \param[in] in_y the y-values
   */
  LinearRegression(std::vector<double> const& in_x, std::vector<double> const& in_y)
    : x_(in_x),
      y_(in_y)
  { }

  /**
   * \brief Perform the regression
   */
  void compute() {
    vtAssert(x_.size() == y_.size(), "Sizes must be the same");
    vtAssert(x_.size() != 0,         "Sizes must not be zero");
    auto const n = x_.size();
    auto const sum_x = std::accumulate(x_.begin(), x_.end(), 0.0);
    auto const sum_y = std::accumulate(y_.begin(), y_.end(), 0.0);
    auto const p_xx = std::inner_product(x_.begin(), x_.end(), x_.begin(), 0.0);
    auto const p_xy = std::inner_product(x_.begin(), x_.end(), y_.begin(), 0.0);
    // numerator:   sum over i of (x[i] - X_mean) * (y[i] - Y_mean)
    // denominator: sum over i of (x[i] - X_mean) * (x[i] - X_mean)
    auto const numerator   = p_xy * n - sum_x * sum_y;
    auto const denominator = p_xx * n - sum_x * sum_x;

    vtAssert(denominator != 0, "Denominator must not be zero");

    slope_ = numerator / denominator;
    intercept_ = (sum_y - slope_ * sum_x) / n;
    computed_ = true;
  }

  /**
   * \brief Get the slope
   *
   * \return the slope of the line
   */
  double getSlope() const { return slope_; }

  /**
   * \brief Get the y-intercept
   *
   * \return the y-intercept
   */
  double getIntercept() const { return intercept_; }

  /**
   * \brief Predict a value using the linear model
   *
   * \param[in] in_x the x value to predict
   *
   * \return the y value
   */
  double predict(double in_x) {
    if (not computed_) {
      compute();
    }
    return intercept_ + slope_ * in_x;
  }

private:
  std::vector<double> const& x_;
  std::vector<double> const& y_;
  double slope_ = 0.;
  double intercept_ = 0.;
  bool computed_ = false;
};

}}} /* end namespace vt::util::stats */

#endif /*INCLUDED_VT_UTILS_STATS_LINEAR_REGRESSION_H*/
