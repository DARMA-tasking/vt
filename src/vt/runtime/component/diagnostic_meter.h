/*
//@HEADER
// *****************************************************************************
//
//                              diagnostic_meter.h
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

#if !defined INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_METER_H
#define INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_METER_H

#include "vt/runtime/component/meter/counter.h"
#include "vt/runtime/component/meter/gauge.h"
#include "vt/runtime/component/meter/timer.h"
#include "vt/runtime/component/meter/counter_gauge.h"
#include "vt/timing/timing_type.h"

namespace vt { namespace diagnostic {

///////////////////////////////////////////////////////////////////////////////

/**
 * \brief Typed diagnostic counter for counting a value over time
 */
template <typename T>
using CounterT = runtime::component::meter::Counter<T>;

/**
 * \brief Typed diagnostic gauge for measuring a value over time
 */
template <typename T>
using GaugeT = runtime::component::meter::Gauge<T>;

/**
 * \brief Typed diagnostic gauge for timing an event over time
 */
template <typename T>
using TimerT = runtime::component::meter::Timer<T>;

/**
 * \brief Typed diagnostic counter/gauge combo
 */
template <typename T, typename U>
using CounterGaugeT = runtime::component::meter::CounterGauge<T, U>;

///////////////////////////////////////////////////////////////////////////////

/// Default type for counters
using CounterDefaultType = int64_t;

/// Default type for gauges
using GaugeDefaultType = int64_t;

/// Default type for timers
using TimerDefaultType = TimeType;

/**
 * \brief Default diagnostic counter for counting a value over time
 */
using Counter = runtime::component::meter::Counter<CounterDefaultType>;

/**
 * \brief Default diagnostic gauge for measuring a value over time
 */
using Gauge = runtime::component::meter::Gauge<GaugeDefaultType>;

/**
 * \brief Default diagnostic gauge for timing an event over time
 */
using Timer = runtime::component::meter::Timer<TimerDefaultType>;

/**
 * \brief Default diagnostic counter/gauge combo
 */
using CounterGauge = runtime::component::meter::CounterGauge<
  CounterDefaultType, GaugeDefaultType
>;

///////////////////////////////////////////////////////////////////////////////

}} /* end namespace vt;:diagnostic */

#endif /*INCLUDED_VT_RUNTIME_COMPONENT_DIAGNOSTIC_METER_H*/
