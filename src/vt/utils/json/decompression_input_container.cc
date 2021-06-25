/*
//@HEADER
// *****************************************************************************
//
//                       decompression_input_container.cc
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

#include "vt/utils/json/decompression_input_container.h"

#include <fstream>

namespace vt { namespace util { namespace json {

DecompressionInputContainer::DecompressionInputContainer(
  std::string const& filename, std::size_t in_chunk_size
) : chunk_size_(in_chunk_size)
{
  std::ifstream is(filename, std::ios::binary);
  vtAssertExpr(is.good());
  d_ = std::make_unique<DecompressorStreamType<std::ifstream>>(std::move(is));
  output_buf_ = std::make_unique<uint8_t[]>(chunk_size_);
  len_ = d_->read(output_buf_.get(), chunk_size_);
}

bool DecompressionInputContainer::advance() const {
  if (cur_ + 1 == len_) {
    len_ = d_->read(output_buf_.get(), chunk_size_);
    cur_ = 0;
    if (cur_ + 1 < len_) {
      return true;
    } else {
      return false;
    }
  }
  if (cur_ + 1 < len_) {
    cur_++;
    return true;
  } else {
    return false;
  }
}

char const& DecompressionInputContainer::getCurrent() const {
  uint8_t const* ptr = &output_buf_[cur_];
  return *reinterpret_cast<char const*>(ptr);
}

}}} /* end namespace vt::util::json */
