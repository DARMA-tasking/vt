
#if ! defined __RUNTIME_TRANSPORT_SEQ_LIST__
#define __RUNTIME_TRANSPORT_SEQ_LIST__

#include "common.h"
#include "seq_common.h"

namespace runtime { namespace seq {

struct SeqList {
  using seq_fun_t = std::function<bool()>;

  SeqList(seq_t const& this_seq_in)
    : this_seq(this_seq_in)
  { }

  void add_action(seq_fun_t const& fn) {
    lst.push_back(fn);
  }

  void progress() {
    bool performed_action = true;
    while (ready and performed_action) {
      if (lst.size() > 0) {
        auto x = lst.front();
        lst.pop_front();
        bool const should_block = x();
        if (should_block) {
          ready = false;
        }
        performed_action = true;
      } else {
        performed_action = false;
      }
    }
  }

  void make_ready() {
    ready = true;
  }

private:
  seq_t this_seq = no_seq;

  bool ready = true;

  std::list<seq_fun_t> lst;
};

}} //end namespace runtime::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_LIST__*/
