/*
//@HEADER
// *****************************************************************************
//
//                                    tree.h
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

#if !defined INCLUDED_VT_TERMINATION_TREE_TREE_H
#define INCLUDED_VT_TERMINATION_TREE_TREE_H

#include "vt/config.h"

#include <vector>
#include <memory>
#include <functional>
#include <string>

namespace vt { namespace termination { namespace tree {

struct EpochTree {

  explicit EpochTree(EpochType in_epoch)
    : epoch_(in_epoch)
  { }

public:
  bool hasChildren() const { return children_.size() != 0; }
  void addChild(EpochTree&& t) {
    children_.push_back(std::make_shared<EpochTree>(std::move(t)));
  }
  void addChild(std::shared_ptr<EpochTree> t) {
    children_.push_back(t);
  }

  void traverse(std::function<void(EpochTree&)> fn) const {
    for (auto& elm : children_) {
      fn(*elm);
    }
  }

private:
  void pushContext(std::string& ctx, char c) const {
    ctx.push_back(' ');
    ctx.push_back(c);
    ctx.push_back(' ');
    ctx.push_back(' ');
  }

  void popContext(std::string& ctx) const {
    for (int i = 0; i < 4; i++) {
      ctx.pop_back();
    }
  }

  void printImpl(std::string& builder, std::string& ctx) const {
    builder += fmt::format("({:x})\n", epoch_);
    std::size_t rem = children_.size();
    for (auto const& child : children_) {
      rem--;
      bool const has_next = rem > 0;
      builder += fmt::format("{} `--", ctx);
      pushContext(ctx, has_next ? '|' : ' ');
      child->printImpl(builder, ctx);
      popContext(ctx);
    }
  }

public:
  std::string print() const {
    std::string builder = "";
    std::string ctx     = "";
    printImpl(builder, ctx);
    return builder;
  }

private:
  EpochType epoch_ = no_epoch;
  std::vector<std::shared_ptr<EpochTree>> children_ = {};
};

}}} /* end namespace vt::termination::tree */

#endif /*INCLUDED_VT_TERMINATION_TREE_TREE_H*/
