
#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H

#include "vt/config.h"
#include "vt/collective/collective_alg.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/serialization/auto_dispatch/dispatch.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"
#include "vt/group/group_headers.h"

namespace vt { namespace collective { namespace reduce {

template <typename MessageT>
/*static*/ void Reduce::reduceUp(MessageT* msg) {
  debug_print(
    reduce, node,
    "reduceUp: tag={}, epoch={}, vrt={}, msg={}\n",
    msg->reduce_tag_, msg->reduce_epoch_, msg->reduce_proxy_, print_ptr(msg)
  );
  auto const& grp = envelopeGetGroup(msg->env);
  if (grp == default_group) {
    theCollective()->reduceAddMsg<MessageT>(msg,false);
    theCollective()->reduceNewMsg<MessageT>(msg);
  } else {
    theGroup()->groupReduce(grp)->template reduceAddMsg<MessageT>(msg,false);
    theGroup()->groupReduce(grp)->template reduceNewMsg<MessageT>(msg);
  }
}

template <typename MessageT>
/*static*/ void Reduce::reduceRootRecv(MessageT* msg) {
  auto const& handler = msg->combine_handler_;
  auto active_fun = auto_registry::getAutoHandler(handler);
  msg->next_ = nullptr;
  msg->count_ = 1;
  msg->is_root_ = true;
  auto const& from_node = theMsg()->getFromNodeCurrentHandler();
  runnable::Runnable<MessageT>::run(handler, nullptr, msg, from_node);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EpochType Reduce::reduce(
  NodeType const& root, MessageT* const msg, TagType const& tag,
  EpochType const& epoch, ReduceNumType const& num_contrib,
  VirtualProxyType const& proxy
) {
  if (group_ != default_group) {
    envelopeSetGroup(msg->env, group_);
  }
  HandlerType const& han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  msg->combine_handler_ = han;
  msg->reduce_tag_ = tag;
  msg->reduce_root_ = root;
  msg->reduce_proxy_ = proxy;
  debug_print(
    reduce, node,
    "reduce: tag={}, epoch={}, vrt={}, num_contrib={}, msg={}, ref={}\n",
    msg->reduce_tag_, msg->reduce_epoch_,
    msg->reduce_proxy_, num_contrib, print_ptr(msg), envelopeGetRef(msg->env)
  );
  if (epoch == no_epoch) {
    auto reduce_epoch_lookup = std::make_tuple(proxy,tag);
    auto iter = next_epoch_for_tag_.find(reduce_epoch_lookup);
    if (iter == next_epoch_for_tag_.end()) {
      next_epoch_for_tag_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(reduce_epoch_lookup),
        std::forward_as_tuple(EpochType{1})
      );
      iter = next_epoch_for_tag_.find(reduce_epoch_lookup);
    }
    assert(iter != next_epoch_for_tag_.end() && "Must exist now");
    msg->reduce_epoch_ = iter->second++;
  } else {
    msg->reduce_epoch_ = epoch;
  }
  reduceAddMsg<MessageT>(msg,true,num_contrib);
  reduceNewMsg<MessageT>(msg);
  return msg->reduce_epoch_;
}

template <typename MessageT>
void Reduce::reduceAddMsg(
  MessageT* msg, bool const local, ReduceNumType const& num_contrib
) {
  auto lookup = ReduceIdentifierType{
    msg->reduce_tag_,msg->reduce_epoch_,msg->reduce_proxy_
  };
  auto live_iter = live_reductions_.find(lookup);
  if (live_iter == live_reductions_.end()) {
    auto num_contrib_state = num_contrib == -1 ? 1 : num_contrib;
    live_reductions_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(lookup),
      std::forward_as_tuple(ReduceState{
        msg->reduce_tag_,msg->reduce_epoch_,num_contrib_state
      })
    );
    live_iter = live_reductions_.find(lookup);
    assert(live_iter != live_reductions_.end());
  }
  auto& state = live_iter->second;
  messageRef(msg);
  state.msgs.push_back(msg);
  if (num_contrib != -1) {
    state.num_contrib_ = num_contrib;
  }
  if (local) {
    state.num_local_contrib_++;
  }
  state.combine_handler_ = msg->combine_handler_;
  state.reduce_root_ = msg->reduce_root_;
  debug_print(
    reduce, node,
    "reduceAddMsg: msg={}, contrib={}, msgs.size()={}, ref={}\n",
    print_ptr(msg), state.num_contrib_,
    state.msgs.size(), envelopeGetRef(msg->env)
  );
}

template <typename MessageT>
void Reduce::startReduce(
  TagType const& tag, EpochType const& epoch, VirtualProxyType const& proxy,
  bool use_num_contrib
) {
  auto lookup = ReduceIdentifierType{tag,epoch,proxy};
  auto live_iter = live_reductions_.find(lookup);
  auto& state = live_iter->second;

  auto const& nmsgs = state.msgs.size();
  bool ready = false;

  debug_print(
    reduce, node,
    "startReduce: tag={}, epoch={}, vrt={}, msg={}, children={}, contrib_={}\n",
    tag, epoch, proxy, state.msgs.size(), getNumChildren(), state.num_contrib_
  );

  if (use_num_contrib) {
    ready = nmsgs == getNumChildren() + state.num_contrib_;
  } else {
    ready = nmsgs == getNumChildren() + state.num_local_contrib_;
  }

  if (ready) {
    // Combine messages
    if (state.msgs.size() > 1) {
      for (int i = 0; i < state.msgs.size(); i++) {
        bool const has_next = i+1 < state.msgs.size();
        state.msgs[i]->next_ = has_next ? state.msgs[i+1] : nullptr;
        state.msgs[i]->count_ = state.msgs.size() - i;
        state.msgs[i]->is_root_ = false;

        debug_print(
          reduce, node,
          "i={} next={} has_next={} count={} msgs.size()={}, ref={}\n",
          i, print_ptr(state.msgs[i]->next_), has_next, state.msgs[i]->count_,
          state.msgs.size(), envelopeGetRef(state.msgs[i]->env)
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
      auto const& handler = state.combine_handler_;
      auto active_fun = auto_registry::getAutoHandler(handler);
      auto const& from_node = theMsg()->getFromNodeCurrentHandler();
      runnable::Runnable<MessageT>::run(
        handler,nullptr,reinterpret_cast<MessageT*>(state.msgs[0]),from_node
      );
    }

    for (int i = 1; i < state.msgs.size(); i++) {
      messageDeref(state.msgs[i]);
    }

    // Send to parent
    auto msg = static_cast<MessageT*>(state.msgs[0]);
    ActionType cont = nullptr;

    state.msgs.clear();
    state.num_contrib_ = 1;

    if (isRoot()) {
      auto const& root = state.reduce_root_;
      auto const& this_node = theContext()->getNode();
      if (root != this_node) {
        debug_print(
          reduce, node,
          "reduce notify root (send): root={}, node={}\n", root, this_node
        );

        using SendDispatch =
          serialization::auto_dispatch::RequiredSerialization<
            MessageT, reduceRootRecv<MessageT>
          >;
        SendDispatch::sendMsg(root, msg);
        //theMsg()->sendMsg<MessageT, reduceRootRecv<MessageT>>(root, msg, cont);
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
      using SendDispatch =
        serialization::auto_dispatch::RequiredSerialization<
          MessageT, reduceUp<MessageT>
        >;
      SendDispatch::sendMsg(parent, msg);
      //theMsg()->sendMsg<MessageT, reduceUp<MessageT>>(parent, msg, cont);
    }
  }
}

template <typename MessageT>
void Reduce::reduceNewMsg(MessageT* msg) {
  return startReduce<MessageT>(
    msg->reduce_tag_, msg->reduce_epoch_, msg->reduce_proxy_
  );
}

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H*/
