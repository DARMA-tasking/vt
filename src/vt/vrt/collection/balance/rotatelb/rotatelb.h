/*
//@HEADER
// ************************************************************************
//
//                          rotatelb.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_ROTATELB_ROTATELB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_ROTATELB_ROTATELB_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/vrt/collection/balance/proc_stats.h"
#include "vt/timing/timing.h"

#include <memory>
#include <list>
#include <map>
#include <cstdlib>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct RotateLBTypes {
  using ObjIDType = balance::ProcStats::ElementIDType;
  using ObjBinType = int32_t;
  using ObjBinListType = std::list<ObjIDType>;
  using ObjSampleType = std::map<ObjBinType, ObjBinListType>;
  using LoadType = double;
  using LoadProfileType = std::unordered_map<NodeType,LoadType>;
};

struct RotateObjMsg : ::vt::Message {};

struct RotateLB : RotateLBTypes {
  using ElementLoadType = std::unordered_map<ObjIDType,TimeType>;
  using ProcStatsMsgType = balance::ProcStatsMsg;
  using TransferType = std::map<NodeType, std::vector<ObjIDType>>;
  using LoadType = double;

  RotateLB() = default;

private:
  void finishedMigrate();
  void procDataIn(ElementLoadType const& data_in);
  static std::unique_ptr<RotateLB> rotate_lb_inst;

public:
  int64_t transfer_count = 0;
  static void rotateLBHandler(balance::RotateLBMsg* msg);
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_ROTATELB_ROTATELB_H*/
