
#if ! defined __RUNTIME_TRANSPORT_VIRTUAL_CONTEXT__
#define __RUNTIME_TRANSPORT_VIRTUAL_CONTEXT__

#include "config.h"
#include "message.h"
#include "utils/bits/bits_common.h"

namespace vt {

static constexpr BitCountType const num_bool_bits = 1;

enum eVrtLocBits {
  Auto       = 0,
  Collection = eVrtLocBits::Auto       + num_bool_bits,
  Migratable = eVrtLocBits::Collection + num_bool_bits,
  Node       = eVrtLocBits::Migratable + node_num_bits,
  ID         =
};

struct VrtContext {
  using VrtContextType = eVrtLocBits;


};

struct VrtContextManager {
  using VrtContextType = eVrtLocBits;
  using VrtContextContainerType = std::vector<VrtContextType>;


};


struct VrtContextCollection {
  using VrtContextType = eVrtLocBits;
  using VrtContextContainerType = std::vector<VrtContextType>;


};

struct VrtContextCollectionManager {
  using VrtContextType = eVrtLocBits;
  using VrtContextContainerType = std::vector<VrtContextType>;


};


} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_CONTEXT__*/
