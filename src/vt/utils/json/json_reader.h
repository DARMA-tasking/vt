/*
//@HEADER
// *****************************************************************************
//
//                                json_reader.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_UTILS_JSON_JSON_READER_H
#define INCLUDED_VT_UTILS_JSON_JSON_READER_H

#include <memory>
#include <string>

#include <nlohmann/json_fwd.hpp>

namespace vt { namespace util { namespace json {

/**
 * \struct Reader
 *
 * \brief Reader for JSON files either compressed or not.
 */
struct Reader {

  /**
   * \brief Construct the reader
   *
   * \param[in] in_filename the file name to read
   * \param[in] in_compressed whether the data is compressed
   */
  explicit Reader(std::string const& in_filename)
    : filename_(in_filename)
  { }

  /**
   * \brief Check if the file is compressed or not
   *
   * \return whether the file is compressed
   */
  bool isCompressed() const;

  /**
   * \brief Read the file and output JSON
   *
   * \return the JSON
   */
  std::unique_ptr<nlohmann::json> readFile();

private:
  std::string filename_ = "";   /**< The file name */
};

}}} /* end namespace vt::util::json */

#endif /*INCLUDED_VT_UTILS_JSON_JSON_READER_H*/
