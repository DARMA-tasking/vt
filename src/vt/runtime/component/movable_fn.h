/*
//@HEADER
// *****************************************************************************
//
//                                 movable_fn.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_MOVABLE_FN_H
#define INCLUDED_VT_RUNTIME_COMPONENT_MOVABLE_FN_H

namespace vt { namespace runtime { namespace component {

/**
 * \internal \struct MovableFn
 *
 * \brief A move-only closure around a general function. This type is not
 * user-constructible; serves as a general base class for move-only callables
 */
struct MovableFn {
protected:
  MovableFn() = default;
public:
  MovableFn(MovableFn&&) = default;
  MovableFn(MovableFn const&) = delete;
  virtual ~MovableFn() = default;
  virtual void invoke() = 0;
};

/**
 * \internal \struct MovableFnTyped
 *
 * \brief A typed move-only function closed around a lambda that can't be
 * copied.
 */
template <typename Fn>
struct MovableFnTyped : MovableFn {
  explicit MovableFnTyped(Fn&& in_fn) : fn_(std::move(in_fn)) { }

  /**
   * \internal \brief Invoke the function
   */
  void invoke() override { fn_(); }

private:
  Fn fn_;                       /**< The move-only function to invoke */
};


}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_MOVABLE_FN_H*/
