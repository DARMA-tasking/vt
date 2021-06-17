/*
//@HEADER
// *****************************************************************************
//
//                               json_appender.h
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

#if !defined INCLUDED_VT_UTILS_JSON_JSON_APPENDER_H
#define INCLUDED_VT_UTILS_JSON_JSON_APPENDER_H

#include "vt/utils/json/base_appender.h"
#include "vt/utils/json/output_adaptor.h"

#include <nlohmann/json.hpp>

#include <fmt/format.h>

namespace vt { namespace util { namespace json {

/**
 * \struct Appender
 *
 * \brief Appends data to a JSON output stream by adding elements to a JSON
 * array until it is destroyed.
 */
template <typename StreamLike>
struct Appender : BaseAppender {
  using jsonlib        = nlohmann::json;
  using SerializerType = nlohmann::detail::serializer<jsonlib>;
  using AdaptorType    = util::json::OutputAdaptor<StreamLike>;

  /**
   * \brief Construct a JSON appender for a specific array with a filename
   *
   * \param[in] array the JSON array name
   * \param[in] filename the JSON filename
   * \param[in] compress whether to compress the output
   */
  Appender(std::string const& array, std::string const& filename, bool compress)
    : Appender(array, StreamLike{filename}, compress)
  { }

  /**
   * \brief Construct a JSON appender for a specific array with a stream
   *
   * \param[in] array the JSON array name
   * \param[in] in_os the output stream
   * \param[in] compress whether to compress the output
   */
  Appender(std::string const& array, StreamLike in_os, bool compress)
    : os_(std::move(in_os)),
      oa_(std::make_shared<AdaptorType>(os_, compress))
  {
    oa_->write_character('{');
    auto str = fmt::format("\"{}\":[", array);
    oa_->write_characters(str.c_str(), str.length());
  }

  /**
   * \brief Add an element to the JSON array
   *
   * \param[in] in the element to add
   */
  void addElm(jsonlib const& in) {
    if (not first_append) {
      oa_->write_character(',');
    }
    SerializerType s(oa_, ' ', jsonlib::error_handler_t::strict);
    s.dump(in, false, true, 0);
    first_append = false;
  }

  /**
   * \brief Finalize the output, closing the array, and flushing the stream
   */
  StreamLike finish() {
    oa_->write_character(']');
    oa_->write_character('}');
    // causes the final flush to happen
    oa_ = nullptr;
    os_.flush();
    // will close automatically when out of scope
    finished_ = true;
    return std::move(os_);
  }

  virtual ~Appender() {
    if (not finished_) {
      finish();
    }
  }

private:
  StreamLike os_;                              /**< The stream to write to */
  std::shared_ptr<AdaptorType> oa_ = nullptr;  /**< The output adaptor */
  bool first_append = true;                    /**< First append?  */
  bool finished_ = false;                      /**< Is finished? */
};

}}} /* end namespace vt::util::json */

#endif /*INCLUDED_VT_UTILS_JSON_JSON_APPENDER_H*/
