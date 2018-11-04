
#if !defined INCLUDED_VRT_COLLECTION_CONS_DETECT_H
#define INCLUDED_VRT_COLLECTION_CONS_DETECT_H

#include "vt/config.h"

#if backend_check_enabled(detector)
  #include "detector_headers.h"
#endif /* backend_check_enabled(detector) */

#include <functional>

#if backend_check_enabled(detector)

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename... Args>
struct ConstructorType {
  template <typename U>
  using non_idx_t = decltype(U(std::declval<Args>()...));
  template <typename U>
  using idx_fst_t = decltype(U(std::declval<IndexT>(),std::declval<Args>()...));
  template <typename U>
  using idx_snd_t = decltype(U(std::declval<Args>()...,std::declval<IndexT>()));

  using has_non_index_cons = detection::is_detected<non_idx_t, ColT>;
  using has_index_fst      = detection::is_detected<idx_fst_t, ColT>;
  using has_index_snd      = detection::is_detected<idx_snd_t, ColT>;

  static constexpr auto const non_index_cons = has_non_index_cons::value;
  static constexpr auto const index_fst      = has_index_fst::value;
  static constexpr auto const index_snd      = has_index_snd::value;

  static constexpr auto const use_no_index   =
    non_index_cons && !index_snd && !index_fst;
  static constexpr auto const use_index_fst  =
    index_fst;
  static constexpr auto const use_index_snd  =
    index_snd && !index_fst;
};

}}} /* end namespace vt::vrt::collection */

#endif /*backend_check_enabled(detector)*/

#endif /*INCLUDED_VRT_COLLECTION_CONS_DETECT_H*/
