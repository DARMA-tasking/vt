\page epoch Epoch Manager
\brief Manage TD epochs

The epoch manager component `vt::epoch::EpochManip`, accessed via
`vt::theEpoch()`, manages termination epochs that are allocated and deallocated
as a program executes to encapsulate and order distributed work. The epoch
manager controls the bits allocated for these along with scopes (distinct,
collective stands of epoch bit allocation) for generating the bits. The epoch
manager also allows inspection of information embedded in the epoch bit field
`vt::EpochType` that encodes the epoch type, category, scope, rank, etc.
