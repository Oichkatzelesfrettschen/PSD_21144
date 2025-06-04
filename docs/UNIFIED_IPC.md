# Unified IPC Architecture

## Overview

PSD_21144 implements a unified IPC system that combines multiple BSD and
microkernel techniques. Messages carry semantic tags describing their
purpose and can traverse virtualization levels with optional zero-copy
semantics.

### Semantic Domains

- `SEM_CODE`    – executable mappings
- `SEM_DATA`    – mutable data
- `SEM_MESSAGE` – IPC buffers
- `SEM_MATRIX`  – numerical arrays

### Zero-Copy IPC

1. Sender and receiver map a shared region.
2. Data is written directly; no serialization occurs.
3. Tokens synchronise access to prevent races.
4. Generation numbers ensure ordering.

### Cross-Virtualization

Channels may connect processes at different virtualization levels. The
host mediates mapping while preserving the semantic attributes so that
fast paths such as seL4-style register transfers remain possible.
