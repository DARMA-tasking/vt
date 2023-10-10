/*
//@HEADER
// *****************************************************************************
//
//                               argv_container.h
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

#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H

#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <tuple>

namespace vt {

namespace arguments {

struct ArgvContainer {
  ArgvContainer(int& argc, char**& argv)
  {
    std::vector<char*> non_vt_args;
    for(int i = 0; i < argc; i++) {
      // cache original argv parameter
      argv_.push_back(strdup(argv[i]));
      // collect non vt params
      if (!((0 == strncmp(argv[i], "--vt_", 5)) ||
          (0 == strncmp(argv[i], "!--vt_", 6)))) {
        non_vt_args.push_back(argv[i]);
      }
    }

    // Reconstruct argv without vt related params
    int new_argc = non_vt_args.size();
    static std::unique_ptr<char*[]> new_argv = nullptr;

    new_argv = std::make_unique<char*[]>(new_argc + 1);

    int i = 0;
    for (auto&& arg : non_vt_args) {
      new_argv[i++] = arg;
    }
    new_argv[i++] = nullptr;

    argc = new_argc;
    argv = new_argv.get();
  }

  ~ArgvContainer() {
    for(char* param: argv_) {
      delete param;
    }
  }

  ArgvContainer(const ArgvContainer&) = delete;
  ArgvContainer& operator=(const ArgvContainer&) = delete;


  std::vector<char*> argv_;
};

} // namespace arguments
} // namespace vt

#endif /*INCLUDED_VT_CONFIGS_ARGUMENTS_ARGV_CONTAINER_H*/
