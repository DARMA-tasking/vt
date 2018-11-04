
#if !defined INCLUDED_RDMA_RDMA_REGION_H
#define INCLUDED_RDMA_RDMA_REGION_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"

#include <sstream>
#include <iostream>

namespace vt { namespace rdma {

using RegionElmSizeType = int32_t;
static constexpr RegionElmSizeType const no_elm_size = -1;

struct Region {
  RDMA_ElmType lo = no_rdma_elm;
  RDMA_ElmType hi = no_rdma_elm;
  RDMA_ElmType sd = 1;

  RegionElmSizeType elm_size = no_elm_size;

  Region(Region const&) = default;

  Region(
    RDMA_ElmType const& in_lo, RDMA_ElmType const& in_hi,
    RDMA_ElmType const& in_sd = 1,
    RegionElmSizeType const& in_elm_size = no_elm_size
  ) : lo(in_lo), hi(in_hi), sd(in_sd), elm_size(in_elm_size)
  {
    assert(sd == 1);
  }

  bool hasElmSize() const {
    return elm_size != no_elm_size;
  }

  void setElmSize(RegionElmSizeType const& size) {
    fmt::print("setting region size to {}\n", size);

    elm_size = size;
  }

  RDMA_ElmType getSize() const {
    return (hi-lo)/sd;
  }

  std::string regionToBuf() const {
    std::stringstream buf;
    buf << "[" << lo << ":" << hi << ":" << sd << "]" << std::endl;
    // char const* const print_format = "[{}:{}:{}]";
    // auto const& len = std::strlen(print_format);
    // std::string str(len);
    // sfmt::print(str.c_str(), print_format, lo, hi, sd);
    return buf.str();
  }
};

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_REGION_H*/
