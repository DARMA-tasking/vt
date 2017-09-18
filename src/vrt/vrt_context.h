
#if !defined INCLUDED_VRT_CONTEXT
#define INCLUDED_VRT_CONTEXT

#include "config.h"
#include "utils/bits/bits_common.h"
#include "configs/types/types_headers.h"

namespace vt { namespace vrt {

using VrtContext_IdentifierType = int32_t;
static constexpr BitCountType const vrtCntx_collection_num_bits = 1;
static constexpr BitCountType const vrtCntx_migratable_num_bits = 1;
static constexpr BitCountType const vrtCntx_node_num_bits =
    BitCounterType<NodeType>::value;
static constexpr BitCountType const vrtCntx_identifier_num_bits =
    BitCounterType<VrtContext_IdentifierType>::value;

enum eVrtContextBits {
  Collection = 0,
  Migratable = eVrtContextBits::Collection + vrtCntx_collection_num_bits,
  Node = eVrtContextBits::Migratable + vrtCntx_migratable_num_bits,
  Identifier = eVrtContextBits::Node + vrtCntx_identifier_num_bits
};


struct VrtContext {
  using VrtContext_BitsType = eVrtContextBits;
  using VrtContext_UniversalIdType = VrtContextType;

//  VrtContext() = default;
  VrtContext(
      NodeType const& node, VrtContext_IdentifierType const& iden,
      bool const& is_coll = false, bool const& is_migratable = false
  ) {
    BitPackerType::boolSetField<VrtContext_BitsType::Collection>
        (vrtC_UID_, is_coll);
    BitPackerType::boolSetField<VrtContext_BitsType::Migratable>
        (vrtC_UID_, is_migratable);
    BitPackerType::setField<VrtContext_BitsType::Node,
                            vrtCntx_node_num_bits>(vrtC_UID_, node);
    BitPackerType::setField<VrtContext_BitsType::Identifier,
                            vrtCntx_identifier_num_bits>(vrtC_UID_, iden);
  }

  inline void setIsCollection(bool const& is_coll) {
    BitPackerType::boolSetField<VrtContext_BitsType::Collection>
        (vrtC_UID_, is_coll);
  }

  inline void setIsMigratable(bool const& is_migratable) {
    BitPackerType::boolSetField<VrtContext_BitsType::Migratable>
        (vrtC_UID_, is_migratable);
  }

  inline void setVrtContextNode(NodeType const& node) {
    BitPackerType::setField<VrtContext_BitsType::Node,
                            vrtCntx_node_num_bits>(vrtC_UID_, node);
  }

  inline void setVrtContextIdentifier(VrtContext_IdentifierType const& iden) {
    BitPackerType::setField<VrtContext_BitsType::Identifier,
                            vrtCntx_identifier_num_bits>(vrtC_UID_, iden);
  }

  inline bool isCollection() const {
    return BitPackerType::boolGetField<VrtContext_BitsType::Collection>
        (vrtC_UID_);
  }

  inline bool isMigratable() const {
    return BitPackerType::boolGetField<VrtContext_BitsType::Migratable>
        (vrtC_UID_);
  }

  inline NodeType getVrtContextNode() const {
    return BitPackerType::getField<VrtContext_BitsType::Node,
                                   vrtCntx_node_num_bits,
                                   NodeType>(vrtC_UID_);
  }

  inline VrtContext_IdentifierType getVrtContextIdentifier() const {
    return BitPackerType::getField<VrtContext_BitsType::Identifier,
                                   vrtCntx_identifier_num_bits,
                                   VrtContext_IdentifierType>(vrtC_UID_);
  }

  inline VrtContext_UniversalIdType getVrtContextUId() const {
    return vrtC_UID_;
  }

 private:
  VrtContext_UniversalIdType vrtC_UID_;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_VRT_CONTEXT*/
