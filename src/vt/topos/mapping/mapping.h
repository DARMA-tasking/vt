/*
//@HEADER
// *****************************************************************************
//
//                                  mapping.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_TOPOS_MAPPING_MAPPING_H
#define INCLUDED_VT_TOPOS_MAPPING_MAPPING_H

#include "vt/config.h"
#include "vt/topos/mapping/mapping_function.h"
#include "vt/topos/mapping/seed/seed.h"
#include "vt/topos/index/index.h"
#include "vt/registry/auto/map/auto_registry_map.h"

#include <functional>

namespace vt { namespace mapping {

// General mapping functions: maps indexed collections to hardware
template <typename IndexType>
using MapType = PhysicalResourceType(*)(IndexType*, PhysicalResourceType);

template <typename IndexType>
using NodeMapType = MapType<IndexType>;
template <typename IndexType>
using CoreMapType = MapType<IndexType>;

// Dense index mapping functions: maps dense index, with dense regions size, to
// hardware
template <typename IndexType>
using DenseMapType = PhysicalResourceType(*)(
  IndexType*, IndexType*, PhysicalResourceType
);

template <typename IndexType>
using DenseNodeMapType = DenseMapType<IndexType>;
template <typename IndexType>
using DenseCoreMapType = DenseMapType<IndexType>;

// Seed mapping functions for singleton mapping to hardware
using SeedMapType = PhysicalResourceType(*)(SeedType, PhysicalResourceType);

using NodeSeedMapType = SeedMapType;
using CoreSeedMapType = SeedMapType;

}}  // end namespace vt::location

#endif /*INCLUDED_VT_TOPOS_MAPPING_MAPPING_H*/
