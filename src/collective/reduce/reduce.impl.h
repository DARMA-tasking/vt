
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H

#include "config.h"
#include "collective/collective_alg.h"
#include "registry/registry.h"
#include "registry/auto/auto_registry_interface.h"
#include "messaging/active.h"

namespace vt { namespace collective { namespace reduce {

template <typename MessageT>
/*static*/ void Reduce::reduceUp(MessageT* msg) {
  messageRef(msg);
  reduceNewMsg<MessageT>(msg);
}

template <typename MessageT>
/*static*/ void Reduce::reduceRootRecv(MessageT* msg) {
  auto const& handler = msg->combine_handler_;
  auto active_fun = auto_registry::getAutoHandler(handler);
  msg->next = nullptr;
  msg->count = 1;
  msg->is_root = true;
  active_fun(msg);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void Reduce::reduce(
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

  messageRef(msg);
  reduceNewMsg<MessageT>(msg);
}

template <typename MessageT>
void Reduce::reduceNewMsg(MessageT* msg) {
  auto lookup = ReduceIdentifierType{msg->reduce_tag_,msg->reduce_epoch_};
  auto live_iter = live_reductions_.find(lookup);
  if (live_iter == live_reductions_.end()) {
    live_reductions_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(lookup),
      std::forward_as_tuple(ReduceState{msg->reduce_tag_,msg->reduce_epoch_})
    );
    live_iter = live_reductions_.find(lookup);
    assert(live_iter != live_reductions_.end());
  }

  auto& state = live_iter->second;
  state.msgs.push_back(msg);

  if (state.msgs.size() == getNumChildren() + 1) {
    // Combine messages
    if (state.msgs.size() > 1) {
      for (int i = 0; i < state.msgs.size(); i++) {
        bool const has_next = i+1 < state.msgs.size();
        state.msgs[i]->next = has_next ? state.msgs[i+1] : nullptr;
        state.msgs[i]->count = state.msgs.size() - i;
        state.msgs[i]->is_root = false;

        debug_print(
          reduce, node,
          "i={} next={} has_next={} count={} msgs.size()={}\n",
          i, print_ptr(state.msgs[i]->next), has_next, state.msgs[i]->count,
          state.msgs.size()
        );
      }

      debug_print(
        reduce, node,
        "msgs.size()={}\n", state.msgs.size()
      );

       /*
        *  Invoke user handler to run the functor that combines messages,
        *  applying the reduction operator
        */
      auto const& handler = msg->combine_handler_;
      auto active_fun = auto_registry::getAutoHandler(handler);
      active_fun(state.msgs[0]);
    }

    // Send to parent
    auto msg = static_cast<MessageT*>(state.msgs[0]);
    ActionType cont = nullptr;
    if (isRoot()) {
      auto const& root = msg->reduce_root_;
      auto const& this_node = theContext()->getNode();
      if (root != this_node) {
        debug_print(
          reduce, node,
          "reduce notify root (send): root={}, node={}\n", root, this_node
        );
        theMsg()->sendMsg<MessageT, reduceRootRecv<MessageT>>(root, msg, cont);
      } else {
        debug_print(
          reduce, node,
          "reduce notify root (deliver directly): root={}, node={}\n",
          root, this_node
        );
        reduceRootRecv(msg);
      }
    } else {
      auto const& parent = getParent();
      debug_print(
        reduce, node,
        "reduce send to parent: parent={}\n", parent
      );
      theMsg()->sendMsg<MessageT, reduceUp<MessageT>>(parent, msg, cont);
    }
  }
}

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H*/
