/*
//@HEADER
// *****************************************************************************
//
//                               startup_config.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_COLLECTIVE_STARTUP_CONFIG_H
#define INCLUDED_VT_COLLECTIVE_STARTUP_CONFIG_H

#include <memory>

namespace vt::arguments {

struct ParseInputHolder;
struct ArgConfig;

} /* end namespace vt::arguments */

namespace vt::runtime {

struct Runtime;

} /* end namespace vt::runtime */

namespace vt {

struct StartupConfig;

// fwd-decl preconfigure function for friend decl
std::unique_ptr<StartupConfig> preconfigure(
  int& argc, char**& argv
);

/**
 * \brief StartupConfig for preconfiguring VT before starting up the runtime.
 */
struct StartupConfig {
  friend std::unique_ptr<StartupConfig> preconfigure(
    int& argc, char**& argv
  );

  friend struct vt::runtime::Runtime;

  using ParseInputHolder = arguments::ParseInputHolder;

  /**
   * \brief \internal Construct a \c StartupConfig
   */
  StartupConfig(
    std::unique_ptr<arguments::ArgConfig> in_arg_config,
    std::unique_ptr<ParseInputHolder> in_parse_input_holder
  );

  ~StartupConfig();

private:
  std::unique_ptr<arguments::ArgConfig> arg_config_;
  std::unique_ptr<ParseInputHolder> parse_input_holder_;
};

} /* end namespace vt */

#endif /*INCLUDED_VT_COLLECTIVE_STARTUP_CONFIG_H*/
