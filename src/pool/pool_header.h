
#if !defined INCLUDED_POOL_POOL_HEADER_H
#define INCLUDED_POOL_POOL_HEADER_H

#include "config.h"

namespace vt { namespace pool {

struct Header {
  WorkerIDType alloc_worker;
  size_t alloc_size;
};

struct AllocLayout {
  Header prealloc;
};

union AllocView {
  AllocLayout* layout;
  char* buffer;
};

struct HeaderManager {
  static char* setHeader(size_t const& num_bytes, char* buffer);
  static size_t getHeaderBytes(char* buffer);
  static WorkerIDType getHeaderWorker(char* buffer);
  static char* getHeaderPtr(char* buffer);
};

}} /* end namespace vt::pool */

#endif /*INCLUDED_POOL_POOL_HEADER_H*/
