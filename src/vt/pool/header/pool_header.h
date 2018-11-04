
#if !defined INCLUDED_POOL_HEADER_POOL_HEADER_H
#define INCLUDED_POOL_HEADER_POOL_HEADER_H

#include "vt/config.h"

namespace vt { namespace pool {

struct Header {
  WorkerIDType alloc_worker;
  size_t alloc_size;
  size_t oversize;
};

struct AllocLayout {
  Header prealloc;
};

union AllocView {
  AllocLayout* layout;
  char* buffer;
};

struct HeaderManager {
  static char* setHeader(
    size_t const& num_bytes, size_t const& oversize, char* buffer
  );
  static size_t getHeaderBytes(char* buffer);
  static size_t getHeaderOversizeBytes(char* buffer);
  static WorkerIDType getHeaderWorker(char* buffer);
  static char* getHeaderPtr(char* buffer);
};

}} /* end namespace vt::pool */

#endif /*INCLUDED_POOL_HEADER_POOL_HEADER_H*/
