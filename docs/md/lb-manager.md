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

\section lb-specification-file LB Specification File

The LB specification file allows users to specify which load balance along with
which LB-specific configuration parameters are passed to the load balancer
instance for any given phase. The order of the LB phase specficiation lines in
the file disambiguates lines---higher precedence for earlier lines.

The format of the LB specification file is:

\code
[%] <$phase> <$lbname> [$LB-specific-arg-1] ... [$LB-specfic-arg-N]
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
