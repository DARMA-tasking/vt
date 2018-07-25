
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/id/pipe_id.h"
#include "utils/bits/bits_common.h"

namespace vt { namespace pipe {

/*static*/ PipeType PipeIDBuilder::createPipeID(
  PipeIDType const& id, NodeType const& node, bool const& is_send_back,
  bool const& is_persist
) {
  PipeType new_pipe = 0;

  setIsSendback(new_pipe, is_send_back);
  setIsPersist(new_pipe, is_persist);
  setNode(new_pipe, node);
  setID(new_pipe, id);

  return new_pipe;
}

/*static*/ void PipeIDBuilder::setIsSendback(
  PipeType& pipe, bool const& is_send_back
) {
  BitPackerType::boolSetField<ePipeIDBits::Sendback>(pipe, is_send_back);
}

/*static*/ void PipeIDBuilder::setIsPersist(
  PipeType& pipe, bool const& is_persist
) {
  BitPackerType::boolSetField<ePipeIDBits::Persist>(pipe, is_persist);
}

/*static*/ void PipeIDBuilder::setNode(
  PipeType& pipe, NodeType const& node
) {
  BitPackerType::setField<ePipeIDBits::Node, pipe_node_num_bits>(pipe, node);
}

/*static*/ void PipeIDBuilder::setID(
  PipeType& pipe, PipeIDType const& id
) {
  BitPackerType::setField<ePipeIDBits::ID, pipe_id_num_bits>(pipe, id);
}

/*static*/ bool PipeIDBuilder::isSendback(PipeType const& pipe) {
  return BitPackerType::boolGetField<ePipeIDBits::Sendback>(pipe);
}

/*static*/ bool PipeIDBuilder::isPersist(PipeType const& pipe) {
  return BitPackerType::boolGetField<ePipeIDBits::Persist>(pipe);
}

/*static*/ NodeType PipeIDBuilder::getNode(PipeType const& pipe) {
  return BitPackerType::getField<
    ePipeIDBits::Node, pipe_node_num_bits, NodeType
  >(pipe);
}

/*static*/ PipeIDType PipeIDBuilder::getPipeID(PipeType const& pipe) {
  return BitPackerType::getField<
    ePipeIDBits::ID, pipe_id_num_bits, PipeIDType
  >(pipe);
}

}} /* end namespace vt::pipe */
