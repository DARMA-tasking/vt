
#if ! defined __RUNTIME_TRANSPORT_RDMA_REGION__
#define __RUNTIME_TRANSPORT_RDMA_REGION__

#include "common.h"
#include "rdma_common.h"

#include <sstream>
#include <iostream>

namespace runtime { namespace rdma {

using region_elm_size_t = int32_t;
static constexpr region_elm_size_t const no_elm_size = -1;

struct Region {
  RDMA_ElmType lo = no_rdma_elm;
  RDMA_ElmType hi = no_rdma_elm;
  RDMA_ElmType sd = 1;

  region_elm_size_t elm_size = no_elm_size;

  Region(Region const&) = default;

  Region(
    RDMA_ElmType const& in_lo, RDMA_ElmType const& in_hi,
    RDMA_ElmType const& in_sd = 1,
    region_elm_size_t const& in_elm_size = no_elm_size
  ) : lo(in_lo), hi(in_hi), sd(in_sd), elm_size(in_elm_size)
  {
    assert(sd == 1);
  }

  bool
  has_elm_size() const {
    return elm_size != no_elm_size;
  }

  void
  set_elm_size(region_elm_size_t const& size) {
    printf("setting region size to %d\n", size);

    elm_size = size;
  }

  RDMA_ElmType
  get_size() const {
    return (hi-lo)/sd;
  }

  std::string
  region_to_buf() const {
    std::stringstream buf;
    buf << "[" << lo << ":" << hi << ":" << sd << "]" << std::endl;
    // char const* const print_format = "[%lld:%lld:%lld]";
    // auto const& len = std::strlen(print_format);
    // std::string str(len);
    // sprintf(str.c_str(), print_format, lo, hi, sd);
    return buf.str();
  }
};

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_COMMON__*/
