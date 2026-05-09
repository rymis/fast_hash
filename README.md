Collision-free Swiss Hash Tables
================================

Introduction
------------

This hash table implementation uses technique I didn't see in use before. It is a Swiss hash table, so it does not create indirection, but it has invariant that can't be found in other hash tables: if element with position `idx` exists in the hash table then at position `idx` will be element with corresponding index. Also all elements in one bucket are connected using `next` field in item structure.

I name it Collision-free because we always start in corresponding bucket or say that element is not present in the table in exactly one read of the item. I plan to test modification that allows to insert element in constant time using list of free items.

I came to this structure when during Microsoft Hackathon implemented similar idea for C# as a part of Energy Efficient Data Structures project. This implementation is independent and does not use any code from the original implementation. I don't have access to original code and as far as I know MS wasn't interested in it.

Data structure
--------------

TODO: describe data structure

Test results
------------

TODO: describe results
