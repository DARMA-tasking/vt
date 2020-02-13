/*
//@HEADER
// *****************************************************************************
//
//                                    spec.h
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

#if !defined INCLUDED_VT_TRACE_FILE_SPEC_SPEC_H
#define INCLUDED_VT_TRACE_FILE_SPEC_SPEC_H

#include "vt/config.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/collective/reduce/operators/default_msg.h"

#include <unordered_map>

namespace vt { namespace trace { namespace file_spec {

/**
 * \struct Spec spec.h vt/trace/file_spec/spec.h
 *
 * \brief Parses trace spec file when available and tests when its enabled. One
 * node can do the parsing, the other receive the spec from a broadcast.
 *
 * Parses the following format:
 * """
 * 0 0 10
 * %100 -3 3
 * 200 -5 5
 * """
 *
 * This specifies that tracing will be enabled on the following phases:
 * { [0,10], [97,103], [195,205], [297,303], ... }
 *
 * The sets of mod-phase and phase-specific entries must be unique. There may be
 * overlap across the two sets, but not within them. Having two entries that
 * start with "%100" or two entries that start with "100" would be invalid and
 * trigger a parsing error. But having a "%100" and "100" entry is valid.
 *
 * Whether tracing is enabled is calculated as an OR across all specification
 * entries. Thus, if a given phase is contained in any spec line, it is
 * enabled. Please note that 0 % 100 = 0. Therefore, if the above example did
 * not contain the first line, tracing would be enabled as:
 * { [0,3], [97,103], * [195,205], [297,303], ... }
 *
 */
struct Spec {
  using ProxyType    = vt::objgroup::proxy::Proxy<Spec>;
  using ArgType      = vt::arguments::ArgConfig;
  using SpecIndex    = int64_t;
  using DoneMsg      = collective::ReduceNoneMsg;

private:
  /**
   * \struct SpecEntry
   *
   * \brief Holds a single entry in a trace spec file; checks if a given phase
   * is enabled within that spec
   */
  struct SpecEntry {
    SpecEntry() = default;
    SpecEntry(SpecIndex in_idx, SpecIndex in_neg, SpecIndex in_pos, bool in_mod)
      : idx_(in_idx),
        neg_(in_neg),
        pos_(in_pos),
        is_mod_(in_mod)
    { }

    /**
     * \brief Get the spec index for this entry
     *
     * \return the spec index
     */
    SpecIndex getIdx() const { return idx_; }

    /**
     * \brief Check if a given phase is enabled within this spec entry
     *
     * \param[in] in_idx the index to check
     *
     * \return whether tracing is enabled based on this entry
     */
    bool testTraceEnabledFor(SpecIndex in_idx) const {
      if (is_mod_) {
        // Check if mod idx_ is within range, by dividing out idx_ we get the
        // pos/neg offset from that nod value
        return in_idx % idx_ >= idx_ + neg_ or in_idx % idx_ <= pos_;
      } else {
        // Check intersection with range: [idx_-neg_, idx_-pos_]
        return in_idx >= idx_ - neg_ and in_idx <= idx_ + pos_;
      }
    }

    /**
     * \brief Serializer for \c SpecEntry
     *
     * \param[in] s serializer
     */
    template <typename SerializerT>
    void serialize(SerializerT& s) {
      s | idx_ | neg_ | pos_ | is_mod_;
    }

  private:
    SpecIndex idx_ = 0;         /**< Index for spec entry (mod or phase) */
    SpecIndex neg_ = 0;         /**< Negative offset for spec entry */
    SpecIndex pos_ = 0;         /**< Positive offset for spec entry */
    bool is_mod_ =  false;      /**< Whether this entry is a mod-entry */
  };

  using SpecMapType  = std::unordered_map<SpecIndex,SpecEntry>;

private:
  /**
   * \brief Initialize the Spec objgroup
   *
   * \param[in] in_proxy the objgroup proxy
   */
  void init(ProxyType in_proxy);

public:
  /**
   * \brief Construct a new Spec objgroup
   *
   * \return the proxy
   */
  static ProxyType construct();

  /**
   * \brief Check entire spec to see if tracing is enabled on any of the entries
   *
   * \param[in] in_phase the phase to check
   *
   * \return whether tracing is enabled
   */
  bool checkTraceEnabled(SpecIndex in_phase);

  /**
   * \brief Check if a specification is enabled, file specified, and file
   * exists. Aborts if file specified but the file is not accessible
   *
   * \return whether a spec exists
   */
  bool hasSpec();

  /**
   * \brief Parse the specification file
   */
  void parse();

  /**
   * \brief Broadcast parsed specification to all nodes
   */
  void broadcastSpec();

  /**
   * \brief Check if spec has been received
   */
  bool specReceived() const { return has_spec_; };

  /**
   * \brief Clear the current specification (used for testing)
   */
  void clear() {
    spec_mod_.clear();
    spec_exact_.clear();
    has_spec_ = false;
  }

  /**
   * \brief Get the proxy for the objgroup (used for testing)
   */
  ProxyType getProxy() const { return proxy_; }

private:
  /**
   * \struct SpecMsg
   *
   * \brief Holds the spec for broadcasting to all nodes
   */
  struct SpecMsg : vt::Message {
    SpecMsg() = default;
    SpecMsg(SpecMapType in_mod, SpecMapType in_exact, NodeType in_root)
      : spec_mod_(in_mod),
        spec_exact_(in_exact),
        root_(in_root)
    { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      s | spec_mod_;
      s | spec_exact_;
      s | root_;
    }

    SpecMapType spec_mod_;
    SpecMapType spec_exact_;
    NodeType root_ = uninitialized_destination;
  };

  /**
   * \brief Handler to receive parsed specification
   *
   * \param[in] msg the incoming spec msg
   */
  void transferSpec(SpecMsg* msg);

  /**
   * \brief Insert an entry into the specification holders
   *
   * \param[in] phase the phase mod or specific
   * \param[in] neg negative offset
   * \param[in] pos positive offset
   * \param[in] is_mod whether it's a mod-phase
   * \param[in] map the map to add it to
   */
  void insertSpec(
    SpecIndex phase, SpecIndex neg, SpecIndex pos, bool is_mod, SpecMapType& map
  );

  /**
   * \brief Eat whitespace during parsing except for newlines
   *
   * \param[in] file the file to read
   *
   * \return the current character after whitespace is eaten
   */
  int eatWhitespace(std::ifstream& file);

private:
  ProxyType proxy_;
  SpecMapType spec_mod_;
  SpecMapType spec_exact_;
  bool has_spec_ = false;
};

}}} /* end namespace vt::trace::file_spec */

#endif /*INCLUDED_VT_TRACE_FILE_SPEC_SPEC_H*/
