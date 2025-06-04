# Distributed Semantic Memory Protocol

This document describes the protocol used to share semantically tagged
memory across network boundaries. A memory region retains its semantic
domain and coherence information even when transferred between nodes.

## Vector Clocks

Distributed regions maintain a vector clock per node. Each write or
export increments the local node's entry. When nodes exchange regions
they merge clocks by taking the component-wise maximum. One vector clock
`A` happens-before another `B` if every component of `A` is less than or
or equal to the corresponding component of `B`, and at least one is
strictly less.

## Consistency Models

Semantic domains select different consistency models:

- `SEM_CODE` – strong consistency, single writer
- `SEM_MESSAGE` – causal consistency using vector clocks
- `SEM_MATRIX` – eventual consistency

Exported packets include the node ID, domain and current vector clock
value. Receivers update their clocks and adjust their local coherence
state accordingly.
