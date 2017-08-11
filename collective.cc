
#include "collective.h"
#include "transport.h"

namespace runtime {

handler_t event_finished_han = 0;
handler_t check_event_finished_han = 0;

/*static*/ void
CollectiveOps::initialize_runtime() {
  event_finished_han =
    CollectiveOps::register_handler([](runtime::Message* in_msg){
      EventFinishedMsg& msg = *static_cast<EventFinishedMsg*>(in_msg);
      auto const& this_node = the_context->get_node();
      auto const& owning_node = the_event->get_owning_node(msg.event_back);

      // printf(
      //   "event_finished_han:: event=%lld, owning_node=%d, "
      //   "this_node=%d\n",
      //   msg.event_back, owning_node, this_node
      // );

      auto const& complete = the_event->test_event_complete(msg.event_back);
      if (complete == AsyncEvent::event_state_t::EventWaiting) {
        auto& holder = the_event->get_event_holder(msg.event_back);
        // printf(
        //   "event_finished_han:: CALLING TRIGGER\n"
        // );
        holder.make_ready_trigger();
      }
    });

  check_event_finished_han =
    CollectiveOps::register_handler([](runtime::Message* in_msg){
      EventCheckFinishedMsg& msg = *static_cast<EventCheckFinishedMsg*>(in_msg);
      auto const& event = msg.event;
      auto const& node = the_event->get_owning_node(event);
      assert(
        node == the_context->get_node() and "Node must be identical"
      );

      auto const& this_node = the_context->get_node();

      // printf(
      //   "check_event_finished_han:: event=%lld, event_back=%lld, "
      //   "this_node=%d, sent_from_node=%d\n",
      //   event, msg.event_back, this_node, msg.sent_from_node
      // );

      auto send_back_fun = [=]{
        auto msg_send = new EventFinishedMsg(event, msg.event_back);
        auto send_back = the_event->get_owning_node(msg.event_back);
        assert(send_back == msg.sent_from_node);
        the_msg->send_msg(
          send_back, event_finished_han, msg_send, [=]{
            delete msg_send;
          }
        );
      };
      auto const& is_complete = the_event->test_event_complete(event);

      // printf(
      //   "check_event_finished_han:: event=%lld, node=%lld, "
      //   "this_node=%d, complete=%d, sent_from_node=%d\n",
      //   event, node, this_node, is_complete, msg.sent_from_node
      // );

      if (is_complete == AsyncEvent::event_state_t::EventReady) {
        send_back_fun();
      } else {
        assert(
          is_complete == AsyncEvent::event_state_t::EventWaiting and
          "Must be waiting if not ready"
        );
        /*ignore return event*/ the_event->attach_action(event, send_back_fun);
      }
    });
}

/*static*/ void
CollectiveOps::finalize_runtime() {

}

} //end namespace runtime
