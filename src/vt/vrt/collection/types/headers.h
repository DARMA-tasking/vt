/*
//@HEADER
// ************************************************************************
//
//                          headers.h
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

#if !defined INCLUDED_VRT_COLLECTION_TYPES_HEADERS_H
#define INCLUDED_VRT_COLLECTION_TYPES_HEADERS_H

#include "vt/config.h"

/*
 * Include all the collection variants with differing constraints:
 *   - Untyped: a virtual context collection with type erasure
 *   - Base: the parent of all others, ranges from static to dynamic
 *   - Statically sized
 *   - Statically sized after initial insertion phase
 *   - Insertable
 *   - Insertable during epochs
 *   - Deletable
 *   - Deletable during epochs
 *   - Dynamic
 */
#include "vt/vrt/collection/types/untyped.h"
#include "vt/vrt/collection/types/base.h"

#include "vt/vrt/collection/types/static_size.h"
#include "vt/vrt/collection/types/static_insertable.h"

#include "vt/vrt/collection/types/insertable.h"
#include "vt/vrt/collection/types/insertable_epoch.h"

#include "vt/vrt/collection/types/deletable.h"
#include "vt/vrt/collection/types/deletable_epoch.h"

#include "vt/vrt/collection/types/dynamic.h"

/*
 *  Type aliases/wrappers for the above collection base classes for user
 *  convenience.
 */
#include "vt/vrt/collection/types/type_aliases.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_HEADERS_H*/
