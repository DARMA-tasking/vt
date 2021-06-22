/*
//@HEADER
// *****************************************************************************
//
//                                compressor.cc
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
#include "vt/utils/compress/compressor.h"

namespace vt { namespace util { namespace compress {

Compressor::Compressor(int in_quality, int in_window_bits, std::size_t buf_size)
  : quality_(in_quality),
    window_bits_(in_window_bits),
    buf_size_(buf_size)
{
  // for now, use the default allocator
  enc_ = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
  if (!enc_) {
    vtAbort("Could not allocate compressor!\n");
  }
  // A little error checking on the quality and window bits
  if (quality_ < BROTLI_MIN_QUALITY or quality_ > BROTLI_MAX_QUALITY) {
    quality_ = BROTLI_MAX_QUALITY;
  }
  if (
    window_bits_ < BROTLI_MIN_WINDOW_BITS or
    window_bits_ > BROTLI_MAX_WINDOW_BITS
  ) {
    window_bits_ = BROTLI_MAX_WINDOW_BITS;
  }
  // Set the quality parameter for the encoder
  BrotliEncoderSetParameter(enc_, BROTLI_PARAM_QUALITY, quality_);
  // Set the lgwin parameter for the encoder
  BrotliEncoderSetParameter(enc_, BROTLI_PARAM_LGWIN, window_bits_);

  out_buf_ = std::make_unique<uint8_t[]>(buf_size);
}

Compressor::~Compressor() {
  if (enc_) {
    BrotliEncoderDestroyInstance(enc_);
  }
}

}}} /* end namespace vt::util::compress */
