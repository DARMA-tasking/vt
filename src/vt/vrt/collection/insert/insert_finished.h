/*
//@HEADER
// *****************************************************************************
//
//                              insert_finished.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_INSERT_INSERT_FINISHED_H
#define INCLUDED_VT_VRT_COLLECTION_INSERT_INSERT_FINISHED_H

#include "vt/config.h"
#include "vt/vrt/collection/insert/insert_token.h"
#include "vt/vrt/collection/reducable/reducable.h"
#include "vt/vrt/proxy/base_collection_proxy.h"
#include "vt/activefn/activefn.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct InsertStartEnd : BaseProxyT {
  InsertStartEnd() = default;
  InsertStartEnd(InsertStartEnd const&) = default;
  InsertStartEnd(InsertStartEnd&&) = default;
  explicit InsertStartEnd(VirtualProxyType const in_proxy);
  InsertStartEnd& operator=(InsertStartEnd const&) = default;

public:
  /**
   * \brief A collective call to start an insertion epoch
   *
   * \param[in] label an optional label for the insertion epoch
   *
   * \return the insert token
   */
  InserterToken beginInserting(std::string const& label = "insertion") const;

  /**
   * \brief A collective call to finish an insertion epoch
   *
   * \param[in] token the insert token
   */
  void finishInserting(InserterToken&& token) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_INSERT_INSERT_FINISHED_H*/
