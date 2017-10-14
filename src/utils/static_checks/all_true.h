
#if !defined __RUNTIME_TRANSPORT_ALL_TRUE__
#define __RUNTIME_TRANSPORT_ALL_TRUE__

#include "config.h"

namespace vt { namespace util {

template <bool...> struct BoolPack;
template <bool... bs>
using all_true = std::is_same<BoolPack<bs..., true>, BoolPack<true, bs...>>;

}} /* end namespace vt::util */

#endif /*__RUNTIME_TRANSPORT_ALL_TRUE__*/
