
#if !defined INCLUDED_REDUCTION_REDUCTION_MANAGER_IMPL_H
#define INCLUDED_REDUCTION_REDUCTION_MANAGER_IMPL_H

#include "config.h"
#include "reduction/reduction_manager.h"
#include "reduction/reduction_state.h"
#include "registry/registry.h"
#include "registry/auto_registry_interface.h"

namespace vt { namespace reduction {

template <typename MessageT>
/*static*/ void ReductionManager::reduceUp(MessageT* msg) {
  theReduction()->reduceNewMsg(msg);
}

template <typename MessageT>
/*static*/ void ReductionManager::reduceRootRecv(MessageT* msg) {
  auto const& handler = msg->combine_handler_;
  auto active_fun = auto_registry::getAutoHandler(handler);
  msg->next = nullptr;
  msg->count = 1;
  msg->is_root = true;
  active_fun(msg);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void ReductionManager::reduce(
  NodeType const& root, MessageT* const msg, TagType const& tag
) {
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  msg->combine_handler_ = han;
  msg->reduce_tag_ = tag;
  msg->reduce_root_ = root;

  auto iter = next_epoch_for_tag_.find(tag);
  if (iter == next_epoch_for_tag_.end()) {
    next_epoch_for_tag_[tag] = static_cast<EpochType>(1);
    iter = next_epoch_for_tag_.find(tag);
  }
  assert(iter != next_epoch_for_tag_.end() && "Must exist now");
  msg->reduce_epoch_ = iter->second++;

  reduceNewMsg<MessageT>(msg);
}

template <typename MessageT>
void ReductionManager::reduceNewMsg(MessageT* msg) {
  auto lookup = ReduceIdentifierType{msg->reduce_tag_,msg->reduce_epoch_};
  auto live_iter = live_reductions_.find(lookup);
  if (live_iter == live_reductions_.end()) {
    live_reductions_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(lookup),
      std::forward_as_tuple(State{msg->reduce_tag_,msg->reduce_epoch_})
    );
    live_iter = live_reductions_.find(lookup);
    assert(live_iter != live_reductions_.end());
  }

  auto& state = live_iter->second;
  messageRef(msg);
  state.msgs.push_back(msg);

  if (state.msgs.size() == getNumChildren() + 1) {
    // Combine messages
    if (state.msgs.size() > 1) {
      for (int i = 0; i < state.msgs.size(); i++) {
        bool const has_next = i+1 < state.msgs.size();
        state.msgs[i]->next = has_next ? state.msgs[i+1] : nullptr;
        state.msgs[i]->count = state.msgs.size() - i;
        state.msgs[i]->is_root = false;
      }

      // Run user handler
      auto const& handler = msg->combine_handler_;
      auto active_fun = auto_registry::getAutoHandler(handler);
      active_fun(state.msgs[0]);
    }

    // Send to parent
    if (isRoot()) {
      auto const& user_root = msg->reduce_root_;
      if (user_root != theContext()->getNode()) {
        theMsg()->sendMsg<MessageT, reduceRootRecv<MessageT>>(
          user_root, static_cast<MessageT*>(state.msgs[0]), [=]{
            /*delete state.msgs[0];*/
          }
        );
      } else {
        reduceRootRecv(state.msgs[0]);
        //delete state.msgs[0];
      }
    } else {
      theMsg()->sendMsg<MessageT, reduceUp<MessageT>>(
        getParent(), static_cast<MessageT*>(state.msgs[0]), [=]{
          /*delete state.msgs[0];*/
        }
      );
    }
  }
}


}} /* end namespace vt::reduction */

#endif /*INCLUDED_REDUCTION_REDUCTION_MANAGER_IMPL_H*/
