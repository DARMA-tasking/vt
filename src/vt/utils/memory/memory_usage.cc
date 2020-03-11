/*
//@HEADER
// *****************************************************************************
//
//                               memory_usage.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/utils/memory/memory_usage.h"

#include <vector>
#include <memory>
#include <string>
#include <unordered_map>
#include <istream>
#include <fstream>
#include <cstdio>

#if defined(vt_has_malloc_h)
# include <malloc.h>
#endif

#if defined(vt_has_malloc_malloc_h)
# include <malloc/malloc.h>
#endif

#if defined(vt_has_mach_mach_h)
# include <mach/mach.h>
#endif

#if defined(vt_has_sys_resource_h)
# include <sys/resource.h>
#endif

#if defined(vt_has_unistd_h)
# include <unistd.h>
#endif

#if defined(vt_has_inttypes_h)
# include <inttypes.h>
#endif

namespace vt { namespace util { namespace memory {

std::size_t Mstats::getUsage() {
# if defined(vt_has_mstats)
    struct mstats ms = mstats();
    return ms.bytes_used;
# else
    return 0;
# endif
}

std::string Mstats::getName() {
  return "mstats";
}

std::size_t Sbrk::getUsage() {
# if defined(vt_has_sbrk)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    if (!inited_) {
      sbrkinit_ = reinterpret_cast<uintptr_t>(sbrk(0));
      inited_ = true;
    }
    uintptr_t sbrknow = reinterpret_cast<uintptr_t>(sbrk(0));
#   pragma GCC diagnostic pop
    return static_cast<std::size_t>(sbrknow - sbrkinit_);
# else
    return 0;
# endif
}

std::string Sbrk::getName() {
  return "sbrk";
}

std::size_t PS::getUsage() {
# if defined(vt_has_popen) && defined(vt_has_pclose) && defined(vt_has_getpid)
    if (arguments::ArgConfig::vt_allow_memory_report_with_ps) {
      auto cmd = fmt::format("/bin/ps -o vsz= -p {}", getpid());
      FILE* p = popen(cmd.c_str(), "r");
      std::size_t vsz = 0;
      if (p) {
        fscanf(p, "%zu", &vsz);
        pclose(p);
      }
      return vsz * 1024;
    } else {
      return 0;
    }
# else
    return 0;
# endif
}

std::string PS::getName() {
  return "ps";
}

std::size_t Mallinfo::getUsage() {
# if defined(vt_has_mallinfo) && defined(vt_has_malloc_h)
    struct mallinfo mi = mallinfo();
    unsigned int blocks = mi.uordblks;
    return static_cast<std::size_t>(blocks);
# else
    return 0;
# endif
}

std::string Mallinfo::getName() {
  return "mallinfo";
}

std::size_t Getrusage::getUsage() {
# if defined(vt_has_getrusage)
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    // On BSD, units are in KiB, on mach it's in bytes
#   if defined(__APPLE__) && defined(__MACH__)
      return usage.ru_maxrss;
#   else
      return usage.ru_maxrss * 1024;
#   endif
# else
    return 0;
# endif
}

std::string Getrusage::getName() {
  return "getrusage";
}

std::size_t MachTaskInfo::getUsage() {
# if defined(vt_has_mach_task_self)
    struct mach_task_basic_info info;
    mach_msg_type_number_t info_count = MACH_TASK_BASIC_INFO_COUNT;
    if (
      task_info(
        mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info,
        &info_count
      ) != KERN_SUCCESS
    ) {
      return 0;
    } else {
      return static_cast<std::size_t>(info.resident_size);
    }
# else
    return 0;
# endif
}

std::string MachTaskInfo::getName() {
  return "machinfo";
}

std::size_t Stat::getUsage() {
  if (failed_) {
    return 0;
  }

  std::size_t vsz = 0; /* should remain 0 on failure */
  FILE *f = fopen("/proc/self/stat", "r");
  if (!f) {
    failed_ = true;
    return 0;
  }
  for (int i = 0; i < 22; i++) {
    fscanf(f, "%*s");
  }
  fscanf(f, "%zu", &vsz);
  fclose(f);

  if (!vsz) {
    failed_ = true;
  }
  return vsz;
}

std::string Stat::getName() {
  return "selfstat";
}

std::size_t StatM::getUsage() {
# if defined(vt_has_sysconf)
  if (failed_) {
    return 0;
  }

  std::ifstream buffer("/proc/self/statm");
  if (buffer.good()) {
    std::size_t to_ignore = 0;
    std::size_t npages = 0;
    buffer >> to_ignore >> npages;
    buffer.close();

    std::size_t huge_pages = 0;
    std::size_t huge_page_bytes = 0;
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.good()) {
      std::string name = "";
      while (not meminfo.eof()) {
        std::string line;
        std::getline(meminfo, line);
        std::stringstream ss(line);
        ss >> name;
        if (name == "HugePages_Total:") {
          ss >> huge_pages;
        }
        if (name == "Hugepagesize:") {
          ss >> huge_page_bytes;
          // Convert huge page size recorded in KiB to bytes
          huge_page_bytes *= 1024;
        }
      }
    }

    return ((npages - huge_pages) * sysconf(_SC_PAGE_SIZE)) +
      (huge_pages * huge_page_bytes);
  } else {
    failed_ = true;
    return 0;
  }
# else
  return 0;
# endif
}

std::string StatM::getName() {
  return "selfstatm";
}

struct CommaDelimit : std::string {};

std::istream& operator>>(std::istream& is, CommaDelimit& output) {
  std::getline(is, output, ',');
  return is;
}

MemoryUsage::MemoryUsage() {
  std::vector<std::unique_ptr<Reporter>> all_reporters;

  // Register all the memory reporters
  all_reporters.emplace_back(std::make_unique<Mstats>());
  all_reporters.emplace_back(std::make_unique<MachTaskInfo>());
  all_reporters.emplace_back(std::make_unique<Stat>());
  all_reporters.emplace_back(std::make_unique<StatM>());
  all_reporters.emplace_back(std::make_unique<Sbrk>());
  all_reporters.emplace_back(std::make_unique<Mallinfo>());
  all_reporters.emplace_back(std::make_unique<Getrusage>());
  all_reporters.emplace_back(std::make_unique<PS>());

  std::string pred = arguments::ArgConfig::vt_memory_reporters;
  std::istringstream iss(pred);
  std::vector<std::string> results(
    std::istream_iterator<CommaDelimit>{iss},
    std::istream_iterator<CommaDelimit>{}
  );

  std::unordered_map<std::string, int> name_map;
  int cur = 0;
  for (auto&& r : all_reporters) {
    name_map[r->getName()] = cur;
    cur++;
  }

  for (auto&& r : results) {
    auto iter = name_map.find(r);
    if (iter != name_map.end()) {
      reporters_.emplace_back(std::move(all_reporters[iter->second]));
    } else {
      if (theContext()->getNode() == 0) {
        auto warning = fmt::format(
          "Invalid memory reporter specified: {}", r
        );
        vtWarn(warning);
      }
    }
  }

  // Prime each reporter, determine if we have a working reporter
  for (auto&& r : reporters_) {
    r->getUsage();
  }
  getFirstUsage();
}

std::size_t MemoryUsage::getAverageUsage() {
  std::size_t usage = 0;
  std::size_t num_valid = 0;
  for (auto&& r : reporters_) {
    auto rusage = r->getUsage();
    if (rusage != 0) {
      usage += rusage;
      num_valid++;
    }
  }
  if (num_valid != 0) {
    return usage / num_valid;
  } else {
    return 0;
  }
}

std::size_t MemoryUsage::getFirstUsage() {
  std::size_t usage = 0;
  if (first_valid_reporter_ == -1) {
    int cur_elm = 0;
    for (auto&& r : reporters_) {
      usage = r->getUsage();
      //fmt::print("reporter: {} usage: {}\n", r->getName(), usage);
      if (usage) {
        first_valid_reporter_ = cur_elm;
        break;
      }
      cur_elm++;
    }
  } else {
    usage = reporters_[first_valid_reporter_]->getUsage();
  }
  return usage;
}

std::string MemoryUsage::getFirstReporter() {
  getFirstUsage();
  if (first_valid_reporter_ != -1) {
    return reporters_[first_valid_reporter_]->getName();
  } else {
    return "<no-valid-reporter>";
  }
}

std::string MemoryUsage::getUsageAll(bool pretty, MemoryUnitEnum unit) {
  std::string builder = "";
  std::size_t num_valid = 0;
  for (auto&& r : reporters_) {
    auto rusage = r->getUsage();
    if (rusage != 0) {
      double dusage = static_cast<double>(rusage);
      for (int8_t i = 0; i < static_cast<int8_t>(unit); i++) {
        dusage /= 1024.0;
      }
      builder += fmt::format(
        "{}{}{}={}{:.6g}{} ",
        pretty ? debug::green() : "",
        r->getName(),
        pretty ? debug::reset() : "",
        pretty ? debug::magenta() : "",
        dusage,
        pretty ? debug::reset() : ""
      );
      num_valid++;
    }
  }
  if (num_valid > 0) {
    builder += fmt::format("({})", getMemoryUnitName(unit));
  }
  return builder;
}

std::vector<std::string> MemoryUsage::getWorkingReporters() {
  std::vector<std::string> working;
  for (auto&& r : reporters_) {
    if (r->getUsage() != 0) {
      working.push_back(r->getName());
    }
  }
  return working;
}

std::size_t MemoryUsage::convertBytesFromString(std::string const& in) {
  double val = 0.0;
  std::string units = "";
  std::istringstream iss(in);
  iss >> val >> units;
  auto unit = getUnitFromString(units);
  for (int8_t i = 0; i < static_cast<int8_t>(unit); i++) {
    val *= 1024.0;
  }
  return static_cast<std::size_t>(val);
}

bool MemoryUsage::hasWorkingReporter() const {
  return first_valid_reporter_ != -1;
}

/*static*/ void MemoryUsage::initialize() {
  impl_ = std::make_unique<MemoryUsage>();
}

/*static*/ void MemoryUsage::finalize() {
  impl_ = nullptr;
}

/*static*/ MemoryUsage* MemoryUsage::get() {
  return impl_.get();
}

/*static*/ std::unique_ptr<MemoryUsage> MemoryUsage::impl_ = nullptr;

}}} /* end namespace vt::util::memory */
