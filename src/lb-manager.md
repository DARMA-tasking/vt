\page lb-manager LB Manager
\brief Manage load balancers

The LB manager component `vt::vrt::collection::balance::LBManager`, accessed via
`vt::theLBManager()` manages and coordinates instances of load balancers. It
counts collections as they call `nextPhase` to ensure they are all ready before
load balancing begins. It reads the command-line arguments or LB specification
file to determine which load balancer to run.

To enable load balancing, the cmake flag \code{.cmake} -Dvt_lb_enabled=1
\endcode should be passed during building. This also enables automatic
instrumentation of collections.

The run a load balancer at runtime:
  - Pass `--vt_lb --vt_lb_name=<LB>` as a command line argument
  - Write a LB specification file `--vt_lb --vt_lb_file --vt_lb_file_name=<FILE>`

The LB specification file allows users to specify which load balance along with
which LB-specific configuration parameters are passed for any given phase. Here
is an example LB specification. Starting with a mod controls which phases will
match each specification line. The order in the file disambiguates lines with
precedence lines that might conflict.

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
