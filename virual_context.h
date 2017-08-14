
#if ! defined __RUNTIME_TRANSPORT_VIRTUAL_CONTEXT__
#define __RUNTIME_TRANSPORT_VIRTUAL_CONTEXT__

#include "common.h"
#include "message.h"

namespace runtime {

using collection_size_t = uint32_t;
using collection_id_t = uint32_t;
using collection_proxy_t = uint64_t;

struct VirtualLocMessage : Message {
  collection_id_t col_id = 0;
};

struct VirutalLoc {
  VirtualContext() = default;
};

struct VirtualLocCollection {
  collection_size_t col_size;
};

struct VirtualLocCollectionManager {
  static
  collection_proxy_t
  create_virtual_loc(collection_size_t col_size) {
    auto const& node = the_context->get_node();
    return (collection_proxy_t)node << (64 - (sizeof() * 8)) | cur_col_id;
  }

private:
  collection_id_t cur_col_id;
};


} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_CONTEXT__*/
