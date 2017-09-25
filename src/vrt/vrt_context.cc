
#include "vrt_context.h"

namespace vt { namespace vrt {

VrtContext::VrtContext
    (NodeType const& node, const bool& is_coll, const bool& is_migratable) {
  BitPackerType::boolSetField<VrtContext_BitsType::Collection>
      (vrtC_, is_coll);
  BitPackerType::boolSetField<VrtContext_BitsType::Migratable>
      (vrtC_, is_migratable);
  BitPackerType::setField<VrtContext_BitsType::Node,
                          vrtCntx_node_num_bits>(vrtC_, node);

}

void VrtContext::setIsCollection(bool const& is_coll) {
  BitPackerType::boolSetField<VrtContext_BitsType::Collection>
      (vrtC_, is_coll);
}

void VrtContext::setIsMigratable(bool const& is_migratable) {
  BitPackerType::boolSetField<VrtContext_BitsType::Migratable>
      (vrtC_, is_migratable);
}

void VrtContext::setVrtContextNode(NodeType const& node) {
  BitPackerType::setField<VrtContext_BitsType::Node,
                          vrtCntx_node_num_bits>(vrtC_, node);
}

bool VrtContext::isCollection() const {
  return BitPackerType::boolGetField<VrtContext_BitsType::Collection>
      (vrtC_);
}

bool VrtContext::isMigratable() const {
  return BitPackerType::boolGetField<VrtContext_BitsType::Migratable>
      (vrtC_);
}

NodeType VrtContext::getVrtContextNode() const {
  return BitPackerType::getField<VrtContext_BitsType::Node,
                                 vrtCntx_node_num_bits,
                                 NodeType>(vrtC_);
}

void VrtContext::printVrtContext() const {
  printf("Virtual context id: %lld \n", vrtC_);
  printf("   |---- Node      : %hu \n", getVrtContextNode());
  printf("   |---- Migratable: %d \n", isMigratable());
  printf("   |---- Collection: %d \n", isCollection());
}

}} // end namespace vt::vrt