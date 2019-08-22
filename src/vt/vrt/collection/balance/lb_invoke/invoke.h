/*
//@HEADER
// ************************************************************************
//
//                            invoke.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_INVOKE_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_INVOKE_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_type.h"
#include "vt/vrt/collection/balance/lb_invoke/invoke_msg.h"
#include "vt/vrt/collection/balance/lb_invoke/start_lb_msg.h"
#include "vt/configs/arguments/args.h"

#include <functional>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct InvokeLB {
  using ArgType = vt::arguments::ArgConfig;

  static LBType shouldInvoke(PhaseType phase, bool try_file = true);
  static void startLB(PhaseType phase, LBType lb);
  static void startLBCollective(InvokeMsg* msg);
  static void startLBCollective(InvokeReduceMsg* msg);
  static void startLBCollective(PhaseType phase, LBType lb);
  static void releaseLB(PhaseType phase);
  static void releaseLBCollective(InvokeMsg* msg);
  static void releaseLBCollective(InvokeReduceMsg* msg);
  static void releaseLBCollective(PhaseType phase);

  template <typename LB>
  static objgroup::proxy::Proxy<LB> makeLB(MsgSharedPtr<StartLBMsg> msg);

private:
  static PhaseType cached_phase_;
  static LBType cached_lb_;
  static std::function<void()> destroy_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LB_INVOKE_INVOKE_H*/
