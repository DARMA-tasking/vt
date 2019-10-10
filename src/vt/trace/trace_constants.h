/*
//@HEADER
// *****************************************************************************
//
//                              trace_constants.h
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

#if !defined INCLUDED_TRACE_TRACE_CONSTANTS_H
#define INCLUDED_TRACE_TRACE_CONSTANTS_H

namespace vt { namespace trace {

enum class eTraceConstants : int32_t {
  InvalidTraceType          = -1,
  Creation                  = 1,
  BeginProcessing           = 2,
  EndProcessing             = 3,
  Enqueue                   = 4,
  Dequeue                   = 5,
  BeginComputation          = 6,
  EndComputation            = 7,
  BeginInterrupt            = 8,
  EndInterrupt              = 9,
  MessageRecv               = 10,
  BeginTrace                = 11,
  EndTrace                  = 12,
  UserEvent                 = 13,
  BeginIdle                 = 14,
  EndIdle                   = 15,
  BeginPack                 = 16,
  EndPack                   = 17,
  BeginUnpack               = 18,
  EndUnpack                 = 19,
  CreationBcast             = 20,
  CreationMulticast         = 21,
  BeginFunc                 = 22,
  EndFunc                   = 23,
  MemoryMalloc              = 24,
  MemoryFree                = 25,
  UserSupplied              = 26,
  MemoryUsageCurrent        = 27,
  UserSuppliedNote          = 28,
  UserSuppliedBracketedNote = 29,
  EndPhase                  = 30,
  SurrogateBlock            = 31,
  UserStat                  = 32,
  BeginUserEventPair        = 98,
  EndUserEventPair          = 99,
  UserEventPair             = 100
};



enum eTraceEnvelopeTypes {
  NewChareMsg     = 1,
  NewVChareMsg    = 2,
  BocInitMsg      = 3,
  ForChareMsg     = 4,
  ForBocMsg       = 5,
  ForVidMsg       = 6,
  FillVidMsg      = 7,
  DeleteVidMsg    = 8,
  RODataMsg       = 9,
  ROMsgMsg        = 10,
  StartExitMsg    = 11,
  ExitMsg         = 12,
  ReqStatMsg      = 13,
  StatMsg         = 14,
  StatDoneMsg     = 15,
  NodeBocInitMsg  = 16,
  ForNodeBocMsg   = 17,
  ArrayEltInitMsg = 18,
  ForArrayEltMsg  = 19,
  ForIDedObjMsg   = 20,
};

}} //end namespace vt::trace

#endif /*INCLUDED_TRACE_TRACE_CONSTANTS_H*/
