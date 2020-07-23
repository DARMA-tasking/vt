\page lb-manager LB Manager
\brief Manage load balancers

The LB manager component `vt::vrt::collection::balance::LBManager`, accessed via
`vt::theLBManager()` manages and coordinates instances of load balancers. It
counts collections as they call `nextPhase` to ensure they are all ready before
load balancing begins. It reads the command-line arguments or LB specification
file to determine which load balancer to run.

To enable load balancing, the cmake flag \code{.cmake} -Dvt_lb_enabled=1
\endcode should be passed during building. This also enables automatic
instrumentation of work and communication performed by collection elements.

To run a load balancer at runtime:
  - Pass `--vt_lb --vt_lb_name=<LB>` as a command line argument
  - Write a LB specification file `--vt_lb --vt_lb_file --vt_lb_file_name=<FILE>`

\section lb-specification-file LB Specification File

The LB specification file allows users to specify which load balancer along with
which LB-specific configuration parameters are passed to the load balancer
instance for any given phase. The order of the LB phase specification lines in
the file disambiguates lines---higher precedence for earlier lines.

The format of the LB specification file is:

\code
[%] <$phase> <$lbname> [$LB-specific-arg-1] ... [$LB-specific-arg-N]
\endcode

If a `%` is present, the line matches phases where:
`current phase % $phase == 0`. Phase-specific lines (ones that specify a load
balancer without a `%`) always always have precedence over `%` lines. The next
token after the optional `%` and `$phase` is the name of the load balancer to
invoke on that phase. After the load balancer name, `N` arguments to the load
balancer are allowed to customize how the load balancer is run with the format
of `key=value`. These arguments are the equivalent of passing
`--vt_lb_args="A=test B=test2"` on the command line.

The following is an example LB specification:

\code
%10 GossipLB c=1 k=5 f=2 i=10
0 HierarchicalLB min=0.9 max=1.1 auto=false
% 5 GreedyLB min=1.0
120 GreedyLB c=0 k=2 f=3 i=3
\endcode

\section load-balancers Load balancers

| Load Balancer  | Type                    | Description                                    | Reference |
| -------------- | ----------------------- | ---------------------------------------------- | --------- |
| RotateLB       | Testing                 | Rotate objects in a ring                       | `vt::vrt::collection::lb::RotateLB` |
| RandomLB       | Testing                 | Randomly migrate object with seed              | `vt::vrt::collection::lb::RandomLB` |
| GreedyLB       | Centralized             | Gather to central node apply min/max heap      | `vt::vrt::collection::lb::GreedyLB` |
| GossipLB       | Distributed             | Gossip-based protocol for fully distributed LB | `vt::vrt::collection::lb::GossipLB` |
| HierarchicalLB | Hierarchical            | Build tree to move objects nodes               | `vt::vrt::collection::lb::HierarchicalLB` |
| ZotltanLB      | Hyper-graph Partitioner | Run Zoltan in hyper-graph mode to LB           | `vt::vrt::collection::lb::ZoltanLB` |
| StatsMapLB     | User-specified          | Read file to determine mapping                 | `vt::vrt::collection::lb::StatsMapLB` |

\section load-models Object Load Models

The performance-oriented load balancers described in the preceding
section require a prediction of the loads each object will represent
during the phases between one load balancing invocation and the
next. These predictions are provided by load models, which are
implementations of the `vt::vrt:collection::balance::LoadModel`
interface. There are a number of general-purpose load model
implementations provided by \vt.

By default, \vt uses a load model that predicts each object's work
load for all future phases will match its workload in the most recent
past phase. The system also provides an interface for applications and
users to arrange use of a non-default load model where that may be
desirable for reasons such as performance experimentation,
specialization to application details, or execution environment
considerations. To install a custom load model, application code
should call `vt::theLBManager()->setLoadModel(user_model)`. To
simplify implementation of custom load models, and allow them to
benefit from future system-level improvements, we recommend that
custom load models be composed atop the default model, which can be
obtained by calling `vt::theLBManager()->getBaseLoadModel()`.

Most provided load models are designed as composable filters inherited
from the `vt::vrt:collection::balance::ComposedModel` class. This
allows them to form a 'model stack' in which each class makes a
particular adjustment to the predictions generated, and relies on
others above and below to refine them further. One exception is the
`vt::vrt:collection::balance::RawData` model, which directly returns
past values recorded in the instrumented statistics structures that
`LBManager` provides.

To illustrate the design concept, the default model is implemented as
a stack of two other components. At the base of the stack is a
`RawData` model that will refer to the instrumented object load
timings recorded by the system during preceding execution. Layered on
that is a `vt::vrt:collection::balance::NaivePersistence` model that
queries the underlying `RawData` model for the times taken in the most
recent phase, and returns those same times as its prediction of the
times those objects will take in all future phases.

The full set of load model classes provided with \vt is as follows

| Load Model         | Description                                         | Reference |
| -------------------|-----------------------------------------------------|---------- |
| **Utilities**      |
| LoadModel          | Pure virtual interface class, which the following implement | `vt::vrt:collection::balance::LoadModel` |
| ComposedModel      | A convenience class for most implementations to inherit from, that passes unmodified calls through to an underlying model instance | `vt::vrt:collection::balance::ComposedModel` |
| RawData            | Returns historical data only, from the measured times | `vt::vrt:collection::balance::RawData` |
| **Transformers**   | Transforms the values computed by the composed model(s), agnostic to whether a query refers to a past or future phase |
| Norm               | When asked for a `WHOLE_PHASE` value, computes a specified l-norm over all subphases | `vt::vrt:collection::balance::Norm` |
| SelectSubphases    | Filters and remaps the subphases with data present in the underlying model | `vt::vrt:collection::balance::SelectSubphases` |
| CommOverhead       | Adds a specified amount of imputed 'system overhead' time to each object's work based on the number of messages received | `vt::vrt:collection::balance::CommOverhead` |
| PerCollection      | Maintains a set of load models associated with different collection instances, and passes queries for an object through to the model corresponding to its collection | `vt::vrt:collection::balance::PerCollection` |
| **Predictors**     | Computes values for future phase queries, and passes through past phase queries |
| NaivePersistence   | Passes through historical queries, and maps all future queries to the most recent past phase | `vt::vrt:collection::balance::NaivePersistence` |
| PersistenceMedianLastN | Similar to NaivePersistence, except that it predicts based on a median over the N most recent phases | `vt::vrt:collection::balance::PersistenceMedianLastN` |
| LinearModel        | Computes a linear regression over on object's loads from a number of recent phases | `vt::vrt:collection::balance::LinearModel` |

All of the provided load balancers described in the previous section
require that the installed load model provide responses to future
phase queries for at least `PhaseOffset::NEXT_PHASE` (i.e. `0`), as
the **Predictors** described above do.
