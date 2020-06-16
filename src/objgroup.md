\page objgroup ObjGroupManager
\brief Create object instances across nodes

The object group manager component `vt::objgroup::ObjGroupManager`, accessed via
`vt::theObjGroup()` allows the creation and management of instances of a group
of objects (one per node) that have a collective proxy for performing operations
like sends, broadcasts, or reductions across the object group.

\section objgroup-example Example creating an object group

\snippet examples/hello_world/objgroup.cc Object group creation
