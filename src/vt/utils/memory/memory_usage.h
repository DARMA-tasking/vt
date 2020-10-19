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
#include "vt/runtime/component/component_pack.h"
#include "vt/utils/memory/memory_units.h"
#include "vt/utils/memory/memory_reporter.h"

#include <string>
#include <memory>

namespace vt { namespace util { namespace memory {

struct Mstats final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) { }
};

struct Sbrk final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | inited_
      | sbrkinit_;
   }

private:
  bool inited_ = false;
  uintptr_t sbrkinit_ = 0;
};

struct PS final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) { }
};

struct Mallinfo final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) { }
};

struct Getrusage final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) { }
};

struct MachTaskInfo final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) { }
};

struct Stat final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | failed_;
   }

private:
  bool failed_ = false;
};

struct StatM final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | failed_;
  }

private:
  bool failed_ = false;
};

struct Mimalloc final : Reporter {
  checkpoint_virtual_serialize_derived_from(Reporter)

  std::size_t getUsage() override;
  std::string getName() override;

  template <typename Serializer>
  void serialize(Serializer& s) { }
};

/**
 * \struct MemoryUsage
 *
 * \brief An optional VT component that records memory usage for tracing and
 * general profiling.
 *
 * The memory usage component is backed by a list of memory reporters that can
 * be selected from depending on the platform and accuracy needed.
 */
struct MemoryUsage : runtime::component::Component<MemoryUsage> {
  checkpoint_virtual_serialize_derived_from(Component)

  /**
   * \internal \brief Construct a memory usage component
   */
  MemoryUsage();

  void initialize() override;

  std::string name() override { return "MemoryUsage"; }

  /**
   * \brief Get mean current usage in bytes over all working reporters
   *
   * \return average usage in bytes
   */
  std::size_t getAverageUsage();

  /**
   * \brief Get current usage from first working reporter
   *
   * \return usage in bytes
   */
  std::size_t getFirstUsage();

  /**
   * \brief Get the first working reporter name
   *
   * \return the name
   */
  std::string getFirstReporter();

  /**
   * \brief Get string of (optionally pretty-printed) usage from all reporters
   *
   * \param[in] pretty whether to pretty-print
   * \param[in] unit unit requested for usage
   *
   * \return string output of usage
   */
  std::string getUsageAll(
    bool pretty = true, MemoryUnitEnum unit = MemoryUnitEnum::Megabytes
  );

  /**
   * \brief Check if there exists a working reporter
   *
   * \return whether there is a working reporter
   */
  bool hasWorkingReporter() const;

  /**
   * \brief Get list of working reporters
   *
   * \return working reporters
   */
  std::vector<std::string> getWorkingReporters();

  /**
   * \brief Convert bytes to string
   *
   * \param[in] in string with bytes
   *
   * \return number of bytes
   */
  std::size_t convertBytesFromString(std::string const& in);

  template <
    typename SerializerT,
    typename = std::enable_if_t<
      std::is_same<SerializerT, checkpoint::Footprinter>::value
    >
  >
  void serialize(SerializerT& s) {
    s | reporters_
      | first_valid_reporter_;
  }

private:
  std::vector<std::unique_ptr<Reporter>> reporters_;
  int first_valid_reporter_ = -1;
};

}}} /* end namespace vt::util::memory */

namespace vt {

util::memory::MemoryUsage* theMemUsage();

} /* end namespace vt */

#endif /*INCLUDED_VT_UTILS_MEMORY_MEMORY_USAGE_H*/
