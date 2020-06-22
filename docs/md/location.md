\page location Location Manager
\brief Virtual entity location management

The location manager component `vt::location::LocationManager`, accessed via
`vt::theLocMan()` manages the location of arbitrary virtual entities in the
system. It holds a set of live `vt::location::EntityLocationCoord` across the
distributed system which allow users to register/unregister entities and inform
the system when migrations occur. With the entities registered, the location
coordinator can route messages to them even in the presence of migrations that
may occur at any time. The location coordinator maintains a cache of locations
for entities registered off-node and forwards messages using a communication
protocol that depends on the size of the message.

Every entity in the system has a "home node", which is the node that is
ultimately responsible for knowing the location of the entity. When an entity
migrates, it informs the home node of its new location in the system. Nodes that
try to route messages to that entity will inquire the home node unless the
location is already in cache.

\section comm-protocol Eager vs. Rendezvous Routing Protocol

The variable `vt::location::small_msg_max_size` controls whether a message is
routed with an eager or rendezvous protocol. If the message is under that size
limit, the message is routed eagerly---forwarded to the "home node" for
resolution if the location is not in the cache.

If the size of the message is greater than `vt::location::small_msg_max_size`,
the location coordinator will inquire with a control message to resolve the
location before the large message is actually sent. This reduces the number of
hops required to send large messages.

\section location-migrations Entity Migrations

When migrations occur at any time, it's always possible for the message to
arrive on a node where the entity *used to be*. In this case, the location
coordinator knows to follow the breadcrumb to get the message delivered properly
where the entity exists now. If the entity continues to move, the message will
"chase" it until it catches up.
