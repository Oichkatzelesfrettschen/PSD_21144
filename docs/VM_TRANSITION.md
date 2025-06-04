# VM Transition

This document describes the migration of PSD_21144 from the legacy
2.11BSD memory model to the 4.4BSD virtual memory system.  The goal of
this transition is to modernise the kernel while retaining existing
system-call semantics.

## Overview

1. **Compatibility Layer**
   The header `vm_compat.h` exposes accessor functions that translate
   historic `proc` fields (such as `p_dsize` or `p_daddr`) onto their
   counterparts in the new `vmspace` structure.  Macros remain for
   backwards compatibility but simply call the functions.
2. **Fork Adapter**
The file `kern_fork_compat.c` implements `kern_fork_compat()`, a
wrapper around `vmspace_fork()` and `cpu_fork()` which preserves
overlay handling during process creation.  Overlay manipulation is
serialized by a simple lock to avoid races while the parent's address
space is unmapped.

## Pseudo Segmentation

"Pseudo Segmentation" is the compatibility scheme that emulates
2.11BSD's historic segmentation model on top of the flat 4.4BSD
`vmspace` layout.  Each process owns a `vm_pseudo_segment` structure
containing pointers to text, data and stack descriptors.  Rather than
driving a real segmented MMU, these descriptors simply describe ranges
within the process's linear address space.  Legacy code can therefore
index the traditional segment fields while the VM subsystem maintains a
uniform page mapping interface.

## Memory Model

- Each process owns a `vmspace` structure containing a `vm_map`, a
  hardware `pmap` and a `vm_pseudo_segment` describing traditional text,
  data and stack segments.
- Existing user space semantics are preserved by translating references
  through the compatibility macros.

## Future Work

Further development will expand this shim to cover additional kernel
subsystems and provide a full regression test-suite under `tests/vm`.
