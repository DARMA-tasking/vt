/*
//@HEADER
// *****************************************************************************
//
//                                 compressor.h
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

#if !defined INCLUDED_VT_UTILS_COMPRESS_COMPRESSOR_H
#define INCLUDED_VT_UTILS_COMPRESS_COMPRESSOR_H

#include <cstdlib>
#include <memory>

#include <brotli/encode.h>

namespace vt { namespace util { namespace compress {

/**
 * \struct Compressor
 *
 * \brief A streaming compressor for outputting data with brotli compression
 */
struct Compressor {

  /**
   * \brief Construct a compressor
   *
   * \param[in] in_quality the quality of compression
   * \param[in] in_window_bits the number of bits for the lgwin
   * \param[in] buf_size the size of the temporary buffer
   */
  explicit Compressor(
    int in_quality, int in_window_bits, std::size_t buf_size = 1 << 20
  );

  ~Compressor();

  /**
   * \brief Compress data and output to writable
   *
   * \param[in] s the stream/writable to write compressed data
   * \param[in] buffer the buffer to compress
   * \param[in] size the length of the buffer to compress
   *
   * \return whether it was successful or not
   */
  template <typename StreamLike>
  bool write(StreamLike& s, uint8_t const* buffer, std::size_t const size);

  /**
   * \brief Finish writing data and output to writable
   *
   * \param[in] s the stream/writable to write compressed data
   *
   * \return whether it was successful or not
   */
  template <typename StreamLike>
  bool finish(StreamLike& s);

protected:
  /**
   * \internal \brief Compress data and output to writable, optionally finishing
   *
   * \param[in] s the stream/writable to write compressed data
   * \param[in] buffer the buffer to compress
   * \param[in] size the length of the buffer to compress
   * \param[in] finish_ whether to finish writing
   *
   * \return whether it was successful or not
   */
  template <typename StreamLike>
  bool writeImpl(
    StreamLike& s, uint8_t const* buffer, std::size_t const size,
    bool finish_
  );

private:
  int quality_ = 0;                              /**< The compressor quality */
  int window_bits_ = 0;                          /**< The compressor lgwin bits */
  std::size_t buf_size_ = 0;                     /**< The output buffer size */
  BrotliEncoderState* enc_ = nullptr;            /**< Underlying encoder state */
  std::unique_ptr<uint8_t[]> out_buf_ = nullptr; /**< The temporary output buffer */
};

}}} /* end namespace vt::util::compress */

#include "vt/utils/compress/compressor.impl.h"

#endif /*INCLUDED_VT_UTILS_COMPRESS_COMPRESSOR_H*/
