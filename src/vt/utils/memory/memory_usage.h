/*
//@HEADER
// *****************************************************************************
//
//                                memory_usage.h
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

#if !defined INCLUDED_VT_UTILS_MEMORY_MEMORY_USAGE_H
#define INCLUDED_VT_UTILS_MEMORY_MEMORY_USAGE_H

#include "vt/config.h"
#include "vt/utils/memory/memory_units.h"
#include "vt/utils/memory/memory_reporter.h"

#include <string>
#include <memory>

namespace vt { namespace util { namespace memory {

struct Mstats : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;
};

struct Sbrk : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;

private:
  bool inited_ = false;
  uintptr_t sbrkinit_ = 0;
};

struct PS : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;
};

struct Mallinfo : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;
};

struct Getrusage : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;
};

struct MachTaskInfo : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;
};

struct Stat : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;

private:
  bool failed_ = false;
};

struct StatM : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;

private:
  bool failed_ = false;
};

struct Mimalloc : Reporter {
  std::size_t getUsage() override;
  std::string getName() override;
};

struct MemoryUsage {
  MemoryUsage();

  std::size_t getAverageUsage();

  std::size_t getFirstUsage();

  std::string getFirstReporter();

  std::string getUsageAll(
    bool pretty = true, MemoryUnitEnum unit = MemoryUnitEnum::Megabytes
  );

  bool hasWorkingReporter() const;

  std::vector<std::string> getWorkingReporters();

  std::size_t convertBytesFromString(std::string const& in);

  static MemoryUsage* get();

  static void initialize();

  static void finalize();

private:
  static std::unique_ptr<MemoryUsage> impl_;

  std::vector<std::unique_ptr<Reporter>> reporters_;
  int first_valid_reporter_ = -1;
};

}}} /* end namespace vt::util::memory */

#endif /*INCLUDED_VT_UTILS_MEMORY_MEMORY_USAGE_H*/
