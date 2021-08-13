/*
//@HEADER
// *****************************************************************************
//
//                             construct_po.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PO_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PO_IMPL_H

#include "vt/vrt/collection/param/construct_po.h"

namespace vt { namespace vrt { namespace collection { namespace param {

template <typename ColT>
EpochType ConstructParams<ColT>::deferWithEpoch(ProxyFnType cb) {
  // validate all the inputs for the configuration
  validateInputs();
  // Set constructed to true
  constructed_ = true;

  if (not has_bounds_ and bulk_inserts_.size() > 0) {
    bounds_ = bulk_inserts_[0];
    has_bounds_ = true;
  }

  auto tup = theCollection()->makeCollection<ColT>(*this);
  auto epoch = std::get<0>(tup);
  auto proxy = std::get<1>(tup);

  theTerm()->addAction(epoch, [=]{ cb(CollectionProxy<ColT>{proxy}); });

  return std::get<0>(tup);
}

/**
 * \brief Wait for construction to complete and then return the proxy
 *
 * \return the proxy after the construction is complete
 */
template <typename ColT>
typename ConstructParams<ColT>::ProxyType ConstructParams<ColT>::wait() {
  // wait for the collection to be constructed
  ProxyType out_proxy;
  bool action_run = false;
  auto ep = deferWithEpoch([&](ProxyType proxy) {
    out_proxy = proxy;
    action_run = true;
  });
  runSchedulerThrough(ep);
  theSched()->runSchedulerWhile([&]{ return not action_run; });
  return out_proxy;
}


}}}} /* end namespace vt::vrt::collection::param */

#endif /*INCLUDED_VT_VRT_COLLECTION_PARAM_CONSTRUCT_PO_IMPL_H*/
