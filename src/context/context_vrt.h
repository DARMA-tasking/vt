
#if !defined INCLUDED_CONTEXT_VRT
#define INCLUDED_CONTEXT_VRT

#include "config.h"
#include "utils/bits/bits_common.h"

namespace vt { namespace vrt {

static constexpr BitCountType const vrtCntx_collection_num_bits = 1;
static constexpr BitCountType const vrtCntx_migratable_num_bits = 1;
static constexpr BitCountType const vrtCntx_node_num_bits =
    BitCounterType<NodeType>::value;

enum eVrtContextBits {
  Collection = 0,
  Migratable = eVrtContextBits::Collection + vrtCntx_collection_num_bits,
  Node       = eVrtContextBits::Migratable + vrtCntx_migratable_num_bits
};

struct VrtContext {
  using VrtContext_BitsType = eVrtContextBits;
  using VrtContext_Type = uint32_t;

  VrtContext() = default;
  explicit VrtContext(NodeType const& node, bool const& is_coll = false,
                      bool const& is_migratable = false);

  void setIsCollection(bool const& is_coll);
  void setIsMigratable(bool const& is_migratable);
  void setVrtContextNode(NodeType const& node);

  void printVrtContext() const;
  bool isCollection() const;
  bool isMigratable() const;
  NodeType getVrtContextNode() const;

 private:
  VrtContext_Type vrtC_;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT*/
