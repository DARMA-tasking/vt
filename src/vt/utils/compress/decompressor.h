/*
//@HEADER
// *****************************************************************************
//
//                                decompressor.h
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

#if !defined INCLUDED_VT_UTILS_COMPRESS_DECOMPRESSOR_H
#define INCLUDED_VT_UTILS_COMPRESS_DECOMPRESSOR_H

#include "vt/utils/compress/decompressor_base.h"

#include <cstdlib>
#include <memory>

#include <brotli/decode.h>

namespace vt { namespace util { namespace compress {

/**
 * \struct Decompressor
 *
 * \brief A streaming decompressor for reading an input buffer with brotli
 * compression.
 */
template <typename Readable>
struct Decompressor : DecompressorBase {

  /**
   * \brief Construct the decompressor
   *
   * \param[in] in_r the stream-like readable to read from
   * \param[in] buf_len_ the temporary buffer length
   */
  explicit Decompressor(Readable in_r, std::size_t buf_len_ = 1 << 16);

  virtual ~Decompressor();

  /**
   * \brief Read bytes into output buffer
   *
   * \param[in] output_buffer the output buffer
   * \param[in] bytes_to_output the number of bytes in the output buffer
   *
   * \return how many bytes it actually read into the buffer
   */
  std::size_t read(uint8_t* output_buffer, std::size_t bytes_to_output) override;

  /**
   * \brief Whether we are done with decompressing the file
   *
   * \return whether we are done
   */
  bool done() const override;

protected:
  /**
   * \internal \brief Get more input, we need it to keep streaming output
   *
   * \return whether more input is available
   */
  bool getMoreInput();

private:
  Readable r_;
  BrotliDecoderState* dec_ = nullptr;  /**< Underlying decoder state */
  std::size_t in_buf_len_ = 0;         /**< Input buffer max length (chunk) */
  std::unique_ptr<uint8_t[]> buf_in_;  /**< Temporary input buffer to read */
  uint8_t const* next_in_ = nullptr;   /**< Next input pointer */
  std::size_t avail_in_ = 0;           /**< Available length of input data */
};

}}} /* end namespace vt::util::compress */

#include "vt/utils/compress/decompressor.impl.h"

#endif /*INCLUDED_VT_UTILS_COMPRESS_DECOMPRESSOR_H*/
