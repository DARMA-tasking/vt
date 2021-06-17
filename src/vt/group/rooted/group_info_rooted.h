/*
//@HEADER
// *****************************************************************************
//
//                             group_info_rooted.h
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

#if !defined INCLUDED_GROUP_GROUP_INFO_ROOTED_H
#define INCLUDED_GROUP_GROUP_INFO_ROOTED_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/base/group_info_base.h"
#include "vt/group/region/group_region.h"
#include "vt/group/region/group_range.h"
#include "vt/group/region/group_list.h"
#include "vt/group/region/group_shallow_list.h"
#include "vt/group/msg/group_msg.h"

#include <memory>
#include <vector>
#include <cstdlib>

namespace vt { namespace group {

struct InfoRooted : virtual InfoBase {
  using RegionType = region::Region;
  using RegionPtrType = std::unique_ptr<RegionType>;
  using ListType = std::vector<RegionType::BoundType>;

  InfoRooted(
    bool const& in_is_remote, RegionPtrType in_region,
    RegionType::SizeType const& in_total_size
  );

protected:
  void setupRooted();

protected:
  bool is_forward_                   = false;
  bool this_node_included_           = false;
  NodeType forward_node_             = uninitialized_destination;
  RegionPtrType region_              = nullptr;
  RegionType::SizeType total_size_   = 0;
  TreePtrType default_spanning_tree_ = nullptr;
  bool is_remote_                    = false;
  ListType region_list_              = {};
  WaitCountType wait_count_          = 0;
  bool is_setup_                     = false;
};

}} /* end namespace vt::group */

#endif /*INCLUDED_GROUP_GROUP_INFO_ROOTED_H*/
