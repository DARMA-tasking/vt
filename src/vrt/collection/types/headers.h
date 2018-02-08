
#if !defined INCLUDED_VRT_COLLECTION_TYPES_HEADERS_H
#define INCLUDED_VRT_COLLECTION_TYPES_HEADERS_H

#include "config.h"

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
#include "vrt/collection/types/untyped.h"
#include "vrt/collection/types/base.h"

#include "vrt/collection/types/static_size.h"
#include "vrt/collection/types/static_insertable.h"

#include "vrt/collection/types/insertable.h"
#include "vrt/collection/types/insertable_epoch.h"

#include "vrt/collection/types/deletable.h"
#include "vrt/collection/types/deletable_epoch.h"

#include "vrt/collection/types/dynamic.h"

/*
 *  Type aliases/wrappers for the above collection base classes for user
 *  convenience.
 */
#include "vrt/collection/types/type_aliases.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_HEADERS_H*/
