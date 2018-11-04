
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_READ_LB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_READ_LB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_type.h"

#include <string>
#include <fstream>
#include <unordered_map>
#include <cstdlib>

namespace vt { namespace vrt { namespace collection { namespace balance {

using SpecIndex = int64_t;

struct SpecEntry {
  SpecEntry(
    SpecIndex const in_idx, std::string const in_name,
    double const in_lb_min, double const in_lb_max
  ) : idx_(in_idx), lb_name_(in_name), lb_min_(in_lb_min), lb_max_(in_lb_max)
  {}

  SpecIndex getIdx() const { return idx_; }
  std::string getName() const { return lb_name_; }
  bool hasMin() const { return lb_min_ != 0.0f; }
  bool hasMax() const { return lb_max_ != 0.0f; }
  double min() const { return lb_min_; }
  double max() const { return lb_max_; }
  LBType getLB() const {
    for (auto&& elm : lb_names_<>) {
      if (lb_name_ == elm.second) {
        return elm.first;
      }
    }
    return LBType::NoLB;
  }

private:
  SpecIndex idx_;
  std::string lb_name_;
  double lb_min_ = 0.0f, lb_max_ = 0.0f;
};

struct ReadLBSpec {
  using SpecMapType = std::unordered_map<SpecIndex,SpecEntry>;

  static bool openFile(std::string const name = "balance.in");
  static void readFile();

  static bool hasSpec() { return has_spec_; }
  static SpecMapType const& entries() { return spec_; }
  static SpecIndex numEntries() { return num_entries_; }
  static SpecEntry const* entry(SpecIndex const& idx);
  static LBType getLB(SpecIndex const& idx);

private:
  static bool has_spec_;
  static bool read_complete_;
  static std::string filename;
  static SpecIndex num_entries_;
  static SpecMapType spec_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_READ_LB_H*/
