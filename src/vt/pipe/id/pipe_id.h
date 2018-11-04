
#if !defined INCLUDED_PIPE_ID_PIPE_ID_H
#define INCLUDED_PIPE_ID_PIPE_ID_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"

namespace vt { namespace pipe {

static constexpr BitCountType const pipe_send_back_num_bits = 1;
static constexpr BitCountType const pipe_persist_num_bits = 1;
static constexpr BitCountType const pipe_node_num_bits =
    BitCounterType<NodeType>::value;
static constexpr BitCountType const pipe_id_num_bits =
    BitCounterType<PipeIDType>::value;

enum ePipeIDBits {
  Sendback   = 0,
  Persist    = ePipeIDBits::Sendback + pipe_send_back_num_bits,
  Node       = ePipeIDBits::Persist  + pipe_persist_num_bits,
  ID         = ePipeIDBits::Node     + pipe_node_num_bits
};

struct PipeIDBuilder {
  static PipeType createPipeID(
    PipeIDType const& id, NodeType const& node,
    bool const& is_send_back = false, bool const& is_persist = true
  );

  static void setIsSendback(PipeType& pipe, bool const& is_send_back);
  static void setIsPersist(PipeType& pipe, bool const& is_persist);
  static void setNode(PipeType& pipe, NodeType const& node);
  static void setID(PipeType& pipe, PipeIDType const& id);
  static bool isSendback(PipeType const& pipe);
  static bool isPersist(PipeType const& pipe);
  static NodeType getNode(PipeType const& pipe);
  static PipeIDType getPipeID(PipeType const& pipe);
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_ID_PIPE_ID_H*/
