/*
//@HEADER
// *****************************************************************************
//
//                       decompression_input_container.h
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

#if !defined INCLUDED_VT_UTILS_JSON_DECOMPRESSION_INPUT_CONTAINER_H
#define INCLUDED_VT_UTILS_JSON_DECOMPRESSION_INPUT_CONTAINER_H

#include "vt/utils/compress/decompressor_base.h"
#include "vt/utils/compress/decompressor.h"

#include <memory>
#include <string>

namespace vt { namespace util { namespace json {

/**
 * \struct DecompressionInputContainer
 *
 * \brief Decompression container for traversing compressed data and passing it
 * to the JSON reader through the \c InputIterator
 */
struct DecompressionInputContainer {
  template <typename StreamT>
  using DecompressorStreamType = compress::Decompressor<StreamT>;
  using DecompressorType = compress::DecompressorBase;

  /**
   * \brief Construct with filename to read
   *
   * \param[in] filename the filename
   * \param[in] in_chunk_size the chunk size to read in increments
   */
  explicit DecompressionInputContainer(
    std::string const& filename, std::size_t in_chunk_size = 1 << 16
  );

  /// Tag type for non-file constructor
  struct AnyStreamTag {};

  /**
   * \brief Construct with anything that resembles a stream
   *
   * \param[in] stream the stream
   * \param[in] in_chunk_size the chunk size to read in increments
   */
  template <typename StreamLike>
  explicit DecompressionInputContainer(
    AnyStreamTag, StreamLike stream, std::size_t in_chunk_size = 1 << 16
  );

  /**
   * \brief Advance by one
   *
   * \return whether it was successful (otherwise, EOF)
   */
  bool advance() const;

  /**
   * \brief Get the current char
   *
   * \return the current char
   */
  char const& getCurrent() const;

private:
  std::size_t chunk_size_ = 0;                     /**< The chunk size */
  std::unique_ptr<DecompressorType> d_ = nullptr;  /**< The decompressor */
  std::unique_ptr<char[]> output_buf_ = nullptr;   /**< The temp output buffer */
  std::size_t mutable cur_ = 0;                    /**< Current position */
  std::size_t mutable len_ = 0;                    /**< Max position in buf */
};

}}} /* end namespace vt::util::json */

#include "vt/utils/json/decompression_input_container.impl.h"

#endif /*INCLUDED_VT_UTILS_JSON_DECOMPRESSION_INPUT_CONTAINER_H*/
