
#include "vrt_context.h"

namespace vt { namespace vrt {

VrtContext::VrtContext
    (NodeType const& node, VrtContext_IdentifierType const& iden,
     const bool& is_coll, const bool& is_migratable) {
  BitPackerType::boolSetField<VrtContext_BitsType::Collection>
      (vrtC_UID_, is_coll);
  BitPackerType::boolSetField<VrtContext_BitsType::Migratable>
      (vrtC_UID_, is_migratable);
  BitPackerType::setField<VrtContext_BitsType::Node,
                          vrtCntx_node_num_bits>(vrtC_UID_, node);
  BitPackerType::setField<VrtContext_BitsType::Identifier,
                          vrtCntx_identifier_num_bits>(vrtC_UID_, iden);
}

void VrtContext::setIsCollection(bool const& is_coll) {
  BitPackerType::boolSetField<VrtContext_BitsType::Collection>
      (vrtC_UID_, is_coll);
}

void VrtContext::setIsMigratable(bool const& is_migratable) {
  BitPackerType::boolSetField<VrtContext_BitsType::Migratable>
      (vrtC_UID_, is_migratable);
}

void VrtContext::setVrtContextNode(NodeType const& node) {
  BitPackerType::setField<VrtContext_BitsType::Node,
                          vrtCntx_node_num_bits>(vrtC_UID_, node);
}

void VrtContext::setVrtContextIdentifier
    (VrtContext_IdentifierType const& iden) {
  BitPackerType::setField<VrtContext_BitsType::Identifier,
                          vrtCntx_identifier_num_bits>(vrtC_UID_, iden);
}

bool VrtContext::isCollection() const {
  return BitPackerType::boolGetField<VrtContext_BitsType::Collection>
      (vrtC_UID_);
}

bool VrtContext::isMigratable() const {
  return BitPackerType::boolGetField<VrtContext_BitsType::Migratable>
      (vrtC_UID_);
}

NodeType VrtContext::getVrtContextNode() const {
  return BitPackerType::getField<VrtContext_BitsType::Node,
                                 vrtCntx_node_num_bits,
                                 NodeType>(vrtC_UID_);
}

VrtContext_IdentifierType VrtContext::getVrtContextIdentifier() const {
  return BitPackerType::getField<VrtContext_BitsType::Identifier,
                                 vrtCntx_identifier_num_bits,
                                 VrtContext_IdentifierType>(vrtC_UID_);
}

void VrtContext::printVrtContext() const {
  printf("Virtual context id: %lld \n", vrtC_UID_);
  printf("   |---- Node      : %hu \n", getVrtContextNode());
  printf("   |---- Identifier: %d \n", getVrtContextIdentifier());
  printf("   |---- Migratable: %d \n", isMigratable());
  printf("   |---- Collection: %d \n", isCollection());
}

}} // end namespace vt::vrt