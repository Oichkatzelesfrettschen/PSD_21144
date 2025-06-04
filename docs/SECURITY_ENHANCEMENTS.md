# Security Enhancements

This document outlines planned improvements to harden PSD_21144 against
modern threats while preserving backward compatibility with 2.11BSD.

## Address Space Layout Randomization

Address Space Layout Randomization (ASLR) increases security by placing
memory regions at unpredictable locations.  The 4.4BSD virtual memory
subsystem already supports flexible mappings; randomization hooks can be
added when creating user address spaces.

The compatibility layer will provide default layouts identical to
2.11BSD when ASLR is disabled.  When enabled, text, data and stack
segments will receive randomized base addresses while maintaining the
traditional interfaces exposed through `vm_compat.h`.

## Memory-Mapped Files

`mmap_compat()` extends the traditional file API by allowing files to be
mapped into a process address space.  Legacy applications continue to
use `open`, `read` and `write`, while newer code may opt in to memory
mapping.  The implementation delegates to `vm_mmap()` so existing VM
security checks apply.
