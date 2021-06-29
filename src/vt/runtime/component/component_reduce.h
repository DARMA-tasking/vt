/*
//@HEADER
// *****************************************************************************
//
//                              component_reduce.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_REDUCE_H
#define INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_REDUCE_H

#include "vt/configs/types/types_type.h"

namespace vt { namespace collective { namespace reduce {

struct Reduce;

}}} /* end namespace vt::collective::reduce */

namespace vt { namespace runtime { namespace component {

/**
 * \struct ComponentReducer
 *
 * \brief Trait class for the component that composes in the unique reducer for
 * each component
 */
struct ComponentReducer {

  /**
   * \internal \brief Get the reducer associated with the component that has a
   * unique scope for this component
   *
   * \return the reducer with unique scope
   */
  collective::reduce::Reduce* reducer();

  /**
   * \internal \brief Get the unique identifier for the component
   *
   * This component ID is assigned uniquely by the component pack and then used
   * to generate a unique reducer scope for the component.
   *
   * \return the component ID
   */
  ComponentIDType getComponentID() const { return component_id_; }

protected:
  ComponentIDType component_id_ = 0; /**< The component's unique ID */
};

}}} /* end namespace vt::runtime::component */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_COMPONENT_REDUCE_H*/
