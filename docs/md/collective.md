\page collective Collectives
\brief Collective operations

The collective component `vt::collective::CollectiveAlg`, accessed via
`vt::theCollective()` implements active-message-based distributed collectives
over the \vt runtime. It performs asynchronous reductions, scatters, barriers,
and allows one to safely use MPI interspersed through \vt code, while running in
a handler.

\section collective-reductions Reductions

\vt comes with several reduction operators built in that can operate over types
where the operator already applies (like integers, doubles, etc.) or
user-defined overloaded operators. These operators also have specializations for
`std::vector<T>` and `std::array<T>`.

| Operator      | Reference                  |
| ------------- | -------------------------- |
| `operator&&`  | `vt::collective::AndOp`    |
| `operator||`  | `vt::collective::OrOp`     |
| `operator+`   | `vt::collective::PlusOp`   |
| `operator+`   | `vt::collective::PlusOp`   |
| `std::min`    | `vt::collective::MinOp`    |
| `std::max`    | `vt::collective::MaxOp`    |
| `std::max`    | `vt::collective::MaxOp`    |
| `operator&`   | `vt::collective::BitAndOp` |
| `operator|`   | `vt::collective::BitOrOp`  |
| `operator^`   | `vt::collective::BitXorOp` |
| <no-operator> | `vt::collective::NoneOp`   |

\subsection collective-reduce-example A Simple Reduction

\code{.cpp}
#include <vt/transport.h>

// Reduce ints
struct ReduceDataMsg : vt::collective::ReduceTMsg<int> {
  explicit ReduceDataMsg(int val)
    : vt::collective::ReduceTMsg<int>(val)
  { }
};

// Handler to target for reduction
struct ReduceResult {
  void operator()(ReduceDataMsg* msg) {
    auto num_nodes = vt::theContext()->getNumNodes();
    auto output = msg->getConstVal();
    fmt::print("reduction value={}\n", output);
    vtAssert(num_nodes * 50 == output, "Must be equal);
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  auto reduce_msg = vt::makeMessage<ReduceDataMsg>(50);

  NodeType const root_reduce_node = 0;
  vt::theCollective()->global()->reduce<vt::collective::PlusOp<int>,ReduceResult>(
    root_reduce_node, reduce_msg.get()
  );

  vt::finalize(); // spins in scheduler until termination
  return 0;
}
\endcode
