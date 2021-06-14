/*
//@HEADER
// *****************************************************************************
//
//                               output_adaptor.h
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

#if !defined INCLUDED_VT_UTILS_JSON_OUTPUT_ADAPTOR_H
#define INCLUDED_VT_UTILS_JSON_OUTPUT_ADAPTOR_H

#include "vt/utils/compress/compressor.h"

#include <nlohmann/json.hpp>

namespace vt { namespace util { namespace json {

/**
 * \struct OutputAdaptor
 *
 * \brief A custom output adaptor for optionally compressing the data before
 * outputting to a stream.
 */
template <typename StreamLike, typename CharType = char>
struct OutputAdaptor : nlohmann::detail::output_adapter_protocol<CharType> {

  /**
   * \brief Construct with a stream
   *
   * \param[in] in_os the stream
   * \param[in] compress whether to compress the output
   */
  OutputAdaptor(StreamLike& in_os, bool compress)
    : c_((compress ? std::make_unique<compress::Compressor>(8, 20) : nullptr)),
      os_(in_os)
  { }

  /**
   * \brief Write a single character to the output
   *
   * \param[in] c the character
   */
  void write_character(CharType c) override {
    write_characters(&c, 1);
  }

  /**
   * \brief Write an array of characters to the output
   *
   * \param[in] s the array
   * \param[in] length length of the array
   */
  void write_characters(CharType const* s, std::size_t length) override {
    if (c_) {
      c_->write(os_, reinterpret_cast<uint8_t const*>(s), length);
    } else {
      os_.write(s, length);
    }
  }

  /**
   * \brief Finalize output, flush, and finish compression if need be
   */
  virtual ~OutputAdaptor() {
    if (c_) {
      c_->finish(os_);
    }
  }

private:
  std::unique_ptr<compress::Compressor> c_ = nullptr; /**< The compressor */
  StreamLike& os_;                                    /**< The output stream */
};

}}} /* end namespace vt::util::json */

#endif /*INCLUDED_VT_UTILS_JSON_OUTPUT_ADAPTOR_H*/
