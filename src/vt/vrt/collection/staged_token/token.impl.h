
#if !defined INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_IMPL_H


#include "vt/config.h"
#include "vt/vrt/collection/staged_token/token.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
template <typename... Args>
void InsertTokenRval<ColT,IndexT>::insert(Args&&... args) {
  return theCollection()->staticInsert<ColT>(proxy_,idx_,args...);
}

// /*virtual*/ InsertToken::~InsertToken() {
//   theCollection()->finishedStaticInsert<ColT>(proxy_);
// }


}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_STAGED_TOKEN_TOKEN_IMPL_H*/
