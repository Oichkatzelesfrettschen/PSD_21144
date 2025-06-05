#ifndef _SYS_VM_SEMANTIC_H_
#define _SYS_VM_SEMANTIC_H_

// vm_coherence.h is now responsible for including stdint/stddef in standalone modes
// and providing base types like vm_offset_t, vm_prot_t, uint32_t, uint64_t (from stdint)
#include <vm/include/vm_coherence.h> // This will provide necessary types

// Ensure this matches the definition in vm_semantic.c or the actual kernel header
enum semantic_domain {
    SEM_NONE = 0, // Important for sentinel in transitions array
    SEM_DATA,
    SEM_TEXT,     // Changed from SEM_CODE to match vm_semantic.c
    SEM_STACK,
    SEM_MESSAGE,
    SEM_MATRIX,
    SEM_HEAP,     // Added for JIT example
    SEM_MAX       // Number of domains
};

// Attributes for semantic regions
#define SEM_ATTR_WRITABLE       0x0001
#define SEM_ATTR_EXECUTABLE     0x0002
#define SEM_ATTR_EXECUTING      0x0004 // Currently executing (e.g., a JITted function)
#define SEM_ATTR_COMPUTING      0x0008 // Matrix computation in progress
#define SEM_ATTR_BUFFER_VALID   0x0010 // IPC buffer has valid data
#define SEM_ATTR_DELIVERED      0x0020 // IPC message has been delivered
#define SEM_ATTR_IN_TRANSIT     0x0040 // IPC message is on its way
#define SEM_ATTR_COMPILED       0x0080 // Heap region has been JIT compiled
#define SEM_ATTR_IMMUTABLE      0x0100 // Region is immutable (e.g. for zero-copy)

struct semantic_descriptor {
    enum semantic_domain domain;
    uint32_t attributes;
    uint64_t version; // For content versioning
    uint32_t transition_count; // How many times this region transitioned
    // Potentially other metadata like owner, permissions etc.
};

// Forward declaration for standalone test
struct semantic_entry; // Defined in vm_semantic_mgt.c (not part of this subtask's files)

// Declarations for functions presumably in vm_semantic_mgt.c or similar
// These are needed if other modules (like IPC) call them.
// The test in vm_semantic_fsm.c does not directly call these.
#ifndef VM_SEMANTIC_FSM_STANDALONE // Avoid duplicate declarations if FSM provides stubs for these
extern struct semantic_entry *vm_semantic_lookup(vm_offset_t start, vm_offset_t end);
extern int vm_semantic_register(vm_offset_t start, vm_offset_t end, enum semantic_domain dom, uint32_t attr);
extern void vm_semantic_init(void);
#endif
extern int semantic_check_protection(struct semantic_descriptor *desc, vm_prot_t prot);


// For IPC message validation example
#define MESSAGE_MAGIC 0xABADCAFE
struct message_header {
    uint32_t mh_magic; // From stdint.h via vm_coherence.h in standalone
    uint32_t mh_size; // Payload size
    // other fields
};


#endif /* _SYS_VM_SEMANTIC_H_ */
