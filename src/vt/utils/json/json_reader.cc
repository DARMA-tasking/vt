/*
//@HEADER
// *****************************************************************************
//
//                                json_reader.cc
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

#if !defined INCLUDED_VT_UTILS_JSON_JSON_READER_CC
#define INCLUDED_VT_UTILS_JSON_JSON_READER_CC

#include "vt/utils/json/json_reader.h"
#include "vt/utils/json/decompression_input_container.h"
#include "vt/utils/json/input_iterator.h"

#include <fstream>

#include <nlohmann/json.hpp>

namespace vt { namespace util { namespace json {

std::unique_ptr<nlohmann::json> Reader::readFile() {
  using json = nlohmann::json;

  bool compressed = true;

  // determine if the file is compressed or not
  {
    std::ifstream is(filename_);
    auto str = fmt::format("Filename is not valid: {}", filename_);
    vtAbortIf(not is.good(), str);
    char f = '\0';
    while (is.good()) {
      f = is.get();
      if (f == ' ' or f == '\t' or f == '\n') {
        continue;
      } else {
        break;
      }
    }
    if (f == '{') {
      compressed = false;
    }
    is.close();
  }

  // @todo: add parser callbacks to allow filtering values while reading
  if (compressed) {
    DecompressionInputContainer c(filename_);
    json j = json::parse(c);
    return std::make_unique<json>(std::move(j));
  } else {
    std::ifstream is(filename_, std::ios::binary);
    vtAssertExpr(is.good());
    json j = json::parse(is);
    is.close();
    return std::make_unique<json>(std::move(j));
  }
}

}}} /* end namespace vt::util::json */

#endif /*INCLUDED_VT_UTILS_JSON_JSON_READER_CC*/
