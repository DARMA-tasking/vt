/*
//@HEADER
// *****************************************************************************
//
//                               worker_headers.h
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

#if !defined INCLUDED_VT_WORKER_WORKER_HEADERS_H
#define INCLUDED_VT_WORKER_WORKER_HEADERS_H

#if vt_threading_enabled

#include "vt/worker/worker.h"
#include "vt/worker/worker_group.h"
#include "vt/worker/worker_types.h"

#if vt_check_enabled(openmp)
  #include "vt/worker/worker_group_omp.h"
#else
  #include "vt/worker/worker_group.h"
#endif

namespace vt { namespace worker {

#if vt_check_enabled(openmp)
  using WorkerGroupType = WorkerGroupOMP;
#elif vt_check_enabled(stdthread)
  using WorkerGroupType = WorkerGroupSTD;
#elif vt_check_enabled(fcontext)
  using WorkerGroupType = WorkerGroupSeq;
#endif

#if vt_check_enabled(openmp)
  using WorkerType = OMPWorker;
#elif vt_check_enabled(stdthread)
  using WorkerType = StdThreadWorker;
#elif vt_check_enabled(fcontext)
  using WorkerType = WorkerSeq;
#endif

}} /* end namespace vt::worker */

namespace vt {

extern worker::WorkerGroupType* theWorkerGrp();

} /* end namespace vt */

#endif /* vt_threading_enabled */

#endif /*INCLUDED_VT_WORKER_WORKER_HEADERS_H*/
