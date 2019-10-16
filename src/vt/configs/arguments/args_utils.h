/*
//@HEADER
// *****************************************************************************
//
//                               args_utils.h
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
#if !defined INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_UTILS_H
#define INCLUDED_VT_CONFIGS_ARGUMENTS_ARGS_UTILS_H

#include <string>

namespace vt { namespace arguments {

template <typename T>
std::string getDisplayValue(const T& val) {
  std::string res;
  return res;
}

template <>
std::string getDisplayValue<bool>(const bool& val) {
  return val ? std::string("ON") : std::string("OFF");
}

template <>
std::string getDisplayValue<int>(const int& val) {
  return std::to_string(val);
}

template <>
std::string getDisplayValue<std::string>(const std::string& val) {
  std::string res = std::string("\"") + val + std::string("\"");
  return res;
}


struct Printer {
  public:
  virtual void output() = 0;
};


template <typename T>
struct Anchor;


template <typename T>
struct PrintOn : public Printer {
  public:
  PrintOn(Anchor<T>* opt, const std::string& msg_str);

  PrintOn(
    Anchor<T>* opt, const std::string& msg_str, std::function<bool()> fun);

  void output() override;

  protected:
  Anchor<T>* option_ = nullptr;
  std::string msg_str_;
  std::function<bool()> condition_ = nullptr;
};


template <typename T>
struct PrintOnOff : public Printer {
  public:
  PrintOnOff(
    Anchor<T>* opt, const std::string& msg_on, const std::string& msg_off);
  PrintOnOff(
    Anchor<T>* opt, const std::string& msg_on, const std::string& msg_off,
    std::function<bool()> fun);
  void output() override;

  protected:
  Anchor<T>* option_;
  std::string msg_on_;
  std::string msg_off_;
  std::function<bool()> condition_ = nullptr;
};


struct Warning : public Printer {
  public:
  Warning(Anchor<bool>* opt, const std::string& compile);
  Warning(
    Anchor<bool>* opt, const std::string& compile, std::function<bool()> fun);
  void output() override;

  protected:
  Anchor<bool>* option_;
  std::string compile_;
  std::function<bool()> condition_ = nullptr;
};


}} // end namespace vt::arguments

#endif
