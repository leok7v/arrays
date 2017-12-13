# arrays

Simple partial replacement for std::vector that attempts to minimize or avoid "heap" usage.

All array classes only supports pointers, atomic types and constructor-less plain data objects "t".

By design: no copy constructor, not assignable, no shrinkig, double capacity realloc for heap.

Arrays are "heavy" - absence of copy constructor and assign operator ensure that arrays are not passed by value as function parameters by mistake.

