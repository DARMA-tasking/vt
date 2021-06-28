/*
//@HEADER
// *****************************************************************************
//
//                             decompressor.impl.h
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

#if !defined INCLUDED_VT_UTILS_COMPRESS_DECOMPRESSOR_IMPL_H
#define INCLUDED_VT_UTILS_COMPRESS_DECOMPRESSOR_IMPL_H

#include "vt/config.h"
#include "vt/utils/compress/decompressor.h"

namespace vt { namespace util { namespace compress {

template <typename Readable>
Decompressor<Readable>::Decompressor(Readable in_r, std::size_t buf_len_)
  : r_(std::move(in_r)),
    in_buf_len_(buf_len_)
{
  // for now, use the default allocator
  dec_ = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
  if (!dec_) {
    vtAbort("Could not allocate decompressor!\n");
  }
  // allocate the temporary buffer for input that streams into the decompressor
  // @note: if we use something like mmap in the future, we can avoid this step
  // potentially
  buf_in_ = std::make_unique<uint8_t[]>(in_buf_len_);

  // r_.seekg(0, r_.end);
  // int length = r_.tellg();
  // r_.seekg(0, r_.beg);

  // vt_debug_print(
  //   terse, gen,
  //   "decompressor: length={}\n", length
  // );
}

template <typename Readable>
bool Decompressor<Readable>::done() const {
  return BrotliDecoderIsUsed(dec_) ? BrotliDecoderIsFinished(dec_) : false;
}

template <typename Readable>
Decompressor<Readable>::~Decompressor() {
  vtAssert(done(), "Should be finished decompressing before destroying");
  if (dec_) {
    BrotliDecoderDestroyInstance(dec_);
  }
}

template <typename Readable>
std::size_t Decompressor<Readable>::read(
  uint8_t* output_buffer, std::size_t bytes_to_output
) {
  uint8_t* next_out = output_buffer;
  std::size_t avail_out = bytes_to_output;

  if (avail_in_ == 0) {
    getMoreInput();
  }

  bool success = false;
  // While we have input data and room to output decompressed data...
  while (avail_out > 0 and not success) {
    BrotliDecoderResult res = BrotliDecoderDecompressStream(
      dec_, &avail_in_, &next_in_, &avail_out, &next_out, nullptr
    );
    switch (res) {
    case BROTLI_DECODER_RESULT_NEEDS_MORE_INPUT:
    {
      // We need to read more to continue decoding
      vtAssert(avail_in_ == 0, "Brotli asked for more input even though we still had some.");
      bool const has_more_input = getMoreInput();
      if (not has_more_input) {
        vtAbort("Brotli asked for more input but the file terminated early.");
      }
      break;
    }
    case BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT:
    {
      // this is an expected condition
      vtAssert(
        avail_out == 0, "Brotli asked for more output even though we still had some."
      );
      break;
    }
    case BROTLI_DECODER_RESULT_SUCCESS:
    {
      if (avail_in_ != 0 or getMoreInput()) {
        vtAbort("Brotli terminated early before reading the whole file!");
      }
      success = true;
      break;
    }
    case BROTLI_DECODER_RESULT_ERROR:
    {
      // we have hit an unknown error, print the code and corresponding message!
      auto error_code = BrotliDecoderGetErrorCode(dec_);
      auto error_str = fmt::format(
        "code={}, msg={}\n", error_code, BrotliDecoderErrorString(error_code)
      );
      vtAbort(error_str);
      break;
    }
    default:
      vtAbort("Unknown error condition during decompression");
    }
  }

  // return how many bytes we ended up decoding
  return bytes_to_output - avail_out;
}

template <typename Readable>
bool Decompressor<Readable>::getMoreInput() {
  if (avail_in_ == 0) {
    // read some data, up to our internal temporary buffer length
    r_.read(reinterpret_cast<char*>(buf_in_.get()), in_buf_len_);

    // check how much we actually read in
    avail_in_ = r_.gcount();
    next_in_ = buf_in_.get();

    return avail_in_ != 0;
  } else {
    return false;
  }
}

}}} /* end namespace vt::util::compress */

#endif /*INCLUDED_VT_UTILS_COMPRESS_DECOMPRESSOR_IMPL_H*/
