
#if !defined INCLUDED_VRT_COLLECTION_TRAITS_COLL_MSG_H
#define INCLUDED_VRT_COLLECTION_TRAITS_COLL_MSG_H

#include "config.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /* backend_check_enabled(detector) */

namespace vt { namespace vrt { namespace collection {

template <typename T>
struct ColMsgTraits {
  template <typename U>
  using IsCollectionMsgType = typename U::IsCollectionMessage;
  using hasCollectionMsgType = detection::is_detected<IsCollectionMsgType, T>;
  static constexpr auto const is_coll_msg = hasCollectionMsgType::value;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TRAITS_COLL_MSG_H*/
