/*
//@HEADER
// *****************************************************************************
//
//                                  manager.cc
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

#include "vt/config.h"
#include "vt/topos/location/location_common.h"
#include "vt/topos/location/manager.h"

#include <cassert>

namespace vt { namespace location {

/*static*/ LocationManager::LocInstContainerType LocationManager::loc_insts;

/*static*/ LocationManager::LocCoordPtrType LocationManager::getInstance(
  LocInstType const inst
) {
  auto inst_iter = loc_insts.find(inst);

  vtAssert(
    inst_iter != loc_insts.end(),
    "LocationManager instance must exist in container"
  );

  return inst_iter->second;
}

/*virtual*/ LocationManager::~LocationManager() {
  virtual_loc = nullptr;
  vrtContextLoc = nullptr;
  collectionLoc.clear();
  loc_insts.clear();
  //pending_inst_.clear();
  cur_loc_inst = 0xFF00000000000000;
}

/*static*/ LocInstType LocationManager::cur_loc_inst = 0xFF00000000000000;

}}  // end namespace vt::location
