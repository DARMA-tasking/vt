/*
//@HEADER
// *****************************************************************************
//
//                               migrate_hooks.h
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

#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/types/untyped.h"

namespace vt { namespace vrt { namespace collection {

struct MigrateHookInterface : UntypedCollection {
  MigrateHookInterface() = default;

public:
  virtual void preMigrateOut() = 0;
  virtual void epiMigrateOut() = 0;
  virtual void preMigrateIn()  = 0;
  virtual void epiMigrateIn()  = 0;

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    UntypedCollection::serialize(s);
  }
};

struct MigrateHookBase : MigrateHookInterface {
  MigrateHookBase() = default;

public:
  virtual void preMigrateOut() override {
    vt_debug_print(vrt_coll, node, "preMigrateOut(): this={}\n", print_ptr(this));
  }
  virtual void epiMigrateOut() override {
    vt_debug_print(vrt_coll, node, "epiMigrateOut(): this={}\n", print_ptr(this));
  }
  virtual void preMigrateIn() override {
    vt_debug_print(vrt_coll, node, "preMigrateIn(): this={}\n", print_ptr(this));
  }
  virtual void epiMigrateIn() override {
    vt_debug_print(vrt_coll, node, "epiMigrateIn(): this={}\n", print_ptr(this));
  }

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    MigrateHookInterface::serialize(s);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H*/
