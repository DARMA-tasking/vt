/*
//@HEADER
// *****************************************************************************
//
//                                 memory_lb.cc
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

#include <vt/transport.h>
#include <vt/vrt/collection/balance/node_lb_data.h>
#include <vt/vrt/collection/balance/lb_data_holder.h>
#include <vt/utils/json/json_reader.h>

#include <string>
#include <vector>
#include <numeric>

using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;
using ElementIDStruct = vt::vrt::collection::balance::ElementIDStruct;

namespace vt {

std::unique_ptr<LBDataHolder> readInData(std::string const& file_name) {
  using vt::util::json::Reader;
  Reader r{file_name};
  auto j = r.readFile();
  auto d = std::make_unique<LBDataHolder>(*j);
  // for (auto&& elm : d->user_defined_[0]) {
  //   for (auto&& user : elm.second) {
  //     fmt::print("elm={}: f={}, s={}\n", elm.first, user.first, user.second);
  //   }
  // }
  return d;
}

} /* end namespace vt */

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  std::vector<LBDataHolder> lb_data;

  for (int i = 1; i < argc; i++) {
    std::string const filename = std::string{argv[i]};
    fmt::print("Reading in filename={}\n", filename);
    auto p = vt::readInData(filename);
    lb_data.push_back(*p);
  }

  vt::finalize();
  return 0;
}
