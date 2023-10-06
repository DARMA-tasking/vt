/*
//@HEADER
// *****************************************************************************
//
//                                    args.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H

#include "vt/configs/arguments/app_config.h"
#include "vt/runtime/component/component.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

namespace vt {

namespace CLI {
class App;
} /* end namespace CLI */

namespace arguments {

/**
 * \struct ArgConfig
 *
 * \brief Component that manages the configuration for a VT instance, parsed
 * through the command-line arguments.
 */
struct ArgConfig : runtime::component::Component<ArgConfig> {
  /// Parse the arguments into ArgConfig.
  /// Re-assigns argc/argv to remove consumed arguments.
  /// On success the tuple will be {-1, ""}. Otherwise the exit code
  /// (which may be 0 if help was requested) will be returned along
  /// with an appropriate display message.
  std::tuple<int, std::string> parse(
    int& argc, char**& argv, AppConfig const* appConfig
  );

  static std::unique_ptr<ArgConfig> construct(std::unique_ptr<ArgConfig> arg);

  std::string name() override { return "ArgConfig"; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | config_
      | parsed_;
  }

  AppConfig config_;

private:
  std::tuple<int, std::string> parseToConfig(
    int& argc, char**& argv, AppConfig& appConfig
  );

  bool parsed_ = false;
};

} // namespace arguments
} // namespace vt

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_H*/
