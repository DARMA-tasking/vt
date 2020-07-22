\page insertable-collections Insertable Collections
\brief Dynamically inserted collections

The virtual context collection component
`vt::vrt::collection::CollectionManager` supports sparse collections that can be
dynamically inserted at runtime. To declare an insertable collection, inherit
from `vt::InsertableCollection` instead of the dense collection
`vt::Collection`. After the collection is constructed, elements will not be
created automatically for the index range passed to `theCollection()->construct`
or `theCollection()->constructCollective()`. Instead, the user is responsible
for inserting the indices that exist after the collection is constructed.

\section insertable-example-program Insertable Collection Example
\snippet examples/collection/collective_insertable.cc Collective insertable example
