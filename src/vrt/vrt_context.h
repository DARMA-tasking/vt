
#if !defined INCLUDED_VRT_CONTEXT
#define INCLUDED_VRT_CONTEXT

#include "config.h"
#include "utils/bits/bits_common.h"
#include "configs/types/types_headers.h"

namespace vt { namespace vrt {

using VrtContext_IdentifierType = uint32_t;
static constexpr BitCountType const vrtCntx_collection_num_bits = 1;
static constexpr BitCountType const vrtCntx_migratable_num_bits = 1;
static constexpr BitCountType const vrtCntx_node_num_bits =
    BitCounterType<NodeType>::value;
static constexpr BitCountType const vrtCntx_identifier_num_bits =
    BitCounterType<VrtContext_IdentifierType>::value;

enum eVrtContextBits {
  Collection = 0,
  Migratable = eVrtContextBits::Collection + vrtCntx_collection_num_bits,
  Node       = eVrtContextBits::Migratable + vrtCntx_migratable_num_bits,
  Identifier = eVrtContextBits::Node       + vrtCntx_node_num_bits
};

struct VrtContext {
  using VrtContext_BitsType = eVrtContextBits;
  using VrtContext_UniversalIdType = VrtContextType;

  VrtContext() = default;
  VrtContext(NodeType const& node, VrtContext_IdentifierType const& iden,
             bool const& is_coll = false, bool const& is_migratable = false);

  void setIsCollection(bool const& is_coll);
  void setIsMigratable(bool const& is_migratable);
  void setVrtContextNode(NodeType const& node);
  void setVrtContextIdentifier(VrtContext_IdentifierType const& iden);

  void printVrtContext() const;
  bool isCollection() const;
  bool isMigratable() const;
  NodeType getVrtContextNode() const;
  VrtContext_IdentifierType getVrtContextIdentifier() const;

  inline VrtContext_UniversalIdType getVrtContextUId() const {
    return vrtC_UID_;
  }

 private:
  VrtContext_UniversalIdType vrtC_UID_;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_VRT_CONTEXT*/
