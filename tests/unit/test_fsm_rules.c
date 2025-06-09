#include <assert.h>
#include <stddef.h> // For size_t
#include <stdint.h> // For uint32_t, uint64_t
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Simplified Local Definitions (No Kernel Headers) ---

enum SimpleSemanticDomain {
	DOMAIN_NONE = 0,
	DOMAIN_DATA,
	DOMAIN_TEXT,
	DOMAIN_MESSAGE,
	DOMAIN_HEAP,
	DOMAIN_MATRIX,
	// Add other domains as needed by rules
};

// Simplified Attributes
#define ATTR_WRITABLE 0x0001
#define ATTR_EXECUTABLE 0x0002
#define ATTR_COMPUTING 0x0008
#define ATTR_IN_TRANSIT 0x0040
#define ATTR_COMPILED 0x0080
#define ATTR_DELIVERED 0x0020
#define ATTR_EXECUTING 0x0004	 // From FSM rules
#define ATTR_BUFFER_VALID 0x0010 // From FSM rules

struct SimpleSemanticDescriptor {
	enum SimpleSemanticDomain domain;
	uint32_t				  attributes;
	// No version or transition_count needed for this abstract test
};

struct SimpleContext {
	struct SimpleSemanticDescriptor* descriptor; // Points to the descriptor being modified
	enum SimpleSemanticDomain		 new_domain;
	// Add other fields if stubs need them, e.g., a dummy address for message validation
	void*  data_ptr; // For the message header check
	size_t data_size;
};

// Simplified Transition Rule Structure
struct SimpleTransitionRule {
	enum SimpleSemanticDomain from;
	enum SimpleSemanticDomain to;
	uint32_t				  required_attrs;
	uint32_t				  forbidden_attrs;
	int (*validator)(struct SimpleContext* ctx);
	void (*action)(struct SimpleContext* ctx);
};

// --- Stubs for Validators and Actions (Simplified) ---

// For MESSAGE_MAGIC
#define MESSAGE_MAGIC_LOCAL 0xABADCAFE
struct message_header_local {
	uint32_t mh_magic; /**< Message magic value for validation. */
	uint32_t mh_size;  /**< Size of the message payload in bytes. */
};

static int validate_simple_data_to_message(struct SimpleContext* ctx) {
	printf("  Validator: DATA -> MESSAGE (Simple). Context data_ptr: %p\n", ctx->data_ptr);
	if (ctx && ctx->data_ptr) {
		struct message_header_local* hdr = (struct message_header_local*) ctx->data_ptr;
		if (ctx->data_size < sizeof(struct message_header_local)) {
			printf("    Validation failed: region too small for header (size %zu, needed %zu).\n", ctx->data_size, sizeof(struct message_header_local));
			return 1; // Indicate error (EINVAL)
		}
		if (hdr->mh_magic != MESSAGE_MAGIC_LOCAL) {
			printf("    Validation failed: magic %x vs %x.\n", hdr->mh_magic, MESSAGE_MAGIC_LOCAL);
			return 1; // Indicate error (EINVAL)
		}
		if ((sizeof(struct message_header_local) + hdr->mh_size) > ctx->data_size) {
			printf("    Validation failed: message payload size %u exceeds region %zu (header %zu)\n", hdr->mh_size, ctx->data_size, sizeof(struct message_header_local));
			return 1; // EINVAL
		}
	} else if (ctx == NULL || ctx->data_ptr == NULL) {
		printf("    Validation failed: context or data_ptr is NULL.\n");
		return 1; // EINVAL
	}
	return 0; // Success
}
/** Perform the ACTION for a DATA to MESSAGE transition. */
static void action_simple_data_to_message(struct SimpleContext* ctx) {
	printf("  Action: DATA -> MESSAGE (Simple). Setting attributes.\n");
	ctx->descriptor->attributes |= ATTR_IN_TRANSIT;
	ctx->descriptor->attributes &= ~ATTR_BUFFER_VALID;
}

/** Validate a MESSAGE to DATA transition. */
static int validate_simple_message_to_data(struct SimpleContext* ctx) {
	(void) ctx; // Suppress unused parameter warning
	printf("  Validator: MESSAGE -> DATA (Simple).\n");
	return 0;
}
/** Perform the ACTION for a MESSAGE to DATA transition. */
static void action_simple_message_to_data(struct SimpleContext* ctx) {
	printf("  Action: MESSAGE -> DATA (Simple). Setting attributes.\n");
	ctx->descriptor->attributes &= ~(ATTR_IN_TRANSIT | ATTR_DELIVERED);
	ctx->descriptor->attributes |= ATTR_BUFFER_VALID;
}

/** Validate a HEAP to TEXT transition. */
static int validate_simple_heap_to_text(struct SimpleContext* ctx) {
	printf("  Validator: HEAP -> TEXT (Simple). Attrs: 0x%x\n", ctx->descriptor->attributes);
	if (!(ctx->descriptor->attributes & ATTR_COMPILED)) {
		printf("    Validation failed: ATTR_COMPILED missing.\n");
		return 1; // Indicate error (EPERM)
	}
	return 0;
}
/** Perform the ACTION for a HEAP to TEXT transition. */
static void action_simple_heap_to_text(struct SimpleContext* ctx) {
	printf("  Action: HEAP -> TEXT (Simple). Setting attributes.\n");
	ctx->descriptor->attributes &= ~ATTR_WRITABLE;
	ctx->descriptor->attributes |= ATTR_EXECUTABLE;
}
/** Validate a MATRIX to MESSAGE transition. */

static int validate_simple_matrix_to_message(struct SimpleContext* ctx) {
	(void) ctx; // Suppress unused parameter warning
	printf("  Validator: MATRIX -> MESSAGE (Simple).\n");
	return 0;
}
/** Perform the ACTION for a MATRIX to MESSAGE transition. */
static void action_simple_matrix_to_message(struct SimpleContext* ctx) {
	printf("  Action: MATRIX -> MESSAGE (Simple). Setting attributes.\n");
	ctx->descriptor->attributes |= ATTR_IN_TRANSIT;
}

// --- Simplified FSM Logic (based on vm_semantic_fsm.c) ---
static struct SimpleTransitionRule simple_rules[] = {
	{ DOMAIN_DATA, DOMAIN_MESSAGE, 0, ATTR_EXECUTING | ATTR_COMPUTING, validate_simple_data_to_message, action_simple_data_to_message },
	{ DOMAIN_MESSAGE, DOMAIN_DATA, ATTR_DELIVERED, ATTR_IN_TRANSIT, validate_simple_message_to_data, action_simple_message_to_data },
	// For HEAP -> TEXT, ATTR_WRITABLE is not forbidden at the start; the action will remove it.
	{ DOMAIN_HEAP, DOMAIN_TEXT, ATTR_COMPILED, 0 /* No forbidden ATTR_WRITABLE here */, validate_simple_heap_to_text, action_simple_heap_to_text },
	{ DOMAIN_MATRIX, DOMAIN_MESSAGE, 0, ATTR_COMPUTING, validate_simple_matrix_to_message, action_simple_matrix_to_message },
	{ DOMAIN_NONE, DOMAIN_NONE, 0, 0, NULL, NULL }
};

// Define abstract error codes for test assertions
#define EINVAL_ABSTRACT 1001
#define EBUSY_ABSTRACT 1002
#define EPERM_ABSTRACT 1003
#define VALIDATOR_FAILED_ABSTRACT 1004

/**
 * Check whether an FSM transition is permitted.
 *
 * @param current_desc Pointer to the descriptor to update.
 * @param new_domain Target domain for the transition.
 * @param data_for_validator Optional buffer for validator callbacks.
 * @param size_for_validator Size of @p data_for_validator.
 * @return 0 on success or an abstract error code.
 */
int check_fsm_transition(struct SimpleSemanticDescriptor* current_desc,
						 enum SimpleSemanticDomain		  new_domain,
						 void*							  data_for_validator,
						 size_t							  size_for_validator) {
	printf("  Checking FSM: %d -> %d (attrs: 0x%x)\n", current_desc->domain, new_domain, current_desc->attributes);
	struct SimpleTransitionRule* rule = NULL;

	if (current_desc->domain == new_domain)
		return 0; // No transition

	for (int i = 0; simple_rules[i].from != DOMAIN_NONE; ++i) {
		if (simple_rules[i].from == current_desc->domain && simple_rules[i].to == new_domain) {
			rule = &simple_rules[i];
			break;
		}
	}
	if (!rule) {
		printf("    No rule found.\n");
		return EINVAL_ABSTRACT;
	}

	if ((current_desc->attributes & rule->forbidden_attrs) != 0) {
		printf("    Forbidden attribute present (0x%x forbidden, current 0x%x).\n", rule->forbidden_attrs, current_desc->attributes);
		return EBUSY_ABSTRACT;
	}
	if ((current_desc->attributes & rule->required_attrs) != rule->required_attrs) {
		printf("    Required attribute missing (0x%x required, current 0x%x).\n", rule->required_attrs, current_desc->attributes);
		return EPERM_ABSTRACT;
	}

	struct SimpleContext ctx;
	ctx.descriptor = current_desc;
	ctx.new_domain = new_domain;
	ctx.data_ptr   = data_for_validator;
	ctx.data_size  = size_for_validator;

	if (rule->validator && rule->validator(&ctx) != 0) {
		printf("    Validator failed.\n");
		return VALIDATOR_FAILED_ABSTRACT;
	}

	// Simulate transition
	current_desc->domain = new_domain;
	if (rule->action) {
		rule->action(&ctx);
	}
	printf("    Transition successful. New domain: %d, New attrs: 0x%x\n", current_desc->domain, current_desc->attributes);
	return 0; // Success
}

// --- Test Cases ---
/** Test that JIT transitions from HEAP to TEXT succeed. */
void test_jit_scenario() {
	printf("--- Test JIT Scenario (HEAP -> TEXT) ---\n");
	struct SimpleSemanticDescriptor desc   = { DOMAIN_HEAP, ATTR_WRITABLE | ATTR_COMPILED };
	int								result = check_fsm_transition(&desc, DOMAIN_TEXT, NULL, 0);
	assert(result == 0);
	assert(desc.domain == DOMAIN_TEXT);
	assert(!(desc.attributes & ATTR_WRITABLE));
	assert(desc.attributes & ATTR_EXECUTABLE);
	assert(desc.attributes & ATTR_COMPILED); // Should persist
	printf("JIT Scenario PASSED\n\n");
}

/** Test sending a message from DATA to MESSAGE domain. */
void test_ipc_send_scenario() {
	printf("--- Test IPC Send Scenario (DATA -> MESSAGE) ---\n");
	char						 buffer[sizeof(struct message_header_local) + 10];
	struct message_header_local* hdr = (struct message_header_local*) buffer;
	hdr->mh_magic					 = MESSAGE_MAGIC_LOCAL;
	hdr->mh_size					 = 5;

	struct SimpleSemanticDescriptor desc   = { DOMAIN_DATA, ATTR_WRITABLE };
	int								result = check_fsm_transition(&desc, DOMAIN_MESSAGE, buffer, sizeof(buffer));
	assert(result == 0);
	assert(desc.domain == DOMAIN_MESSAGE);
	assert(desc.attributes & ATTR_IN_TRANSIT);
	printf("IPC Send Scenario PASSED\n\n");
}

/** Test receiving a message from MESSAGE back to DATA. */
void test_ipc_receive_scenario() {
	printf("--- Test IPC Receive Scenario (MESSAGE -> DATA) ---\n");
	// If a message is delivered, it should no longer be marked as IN_TRANSIT.
	// The FSM rule correctly forbids IN_TRANSIT for this transition.
	struct SimpleSemanticDescriptor desc   = { DOMAIN_MESSAGE, ATTR_DELIVERED };
	int								result = check_fsm_transition(&desc, DOMAIN_DATA, NULL, 0);
	assert(result == 0);
	assert(desc.domain == DOMAIN_DATA);
	assert(!(desc.attributes & ATTR_IN_TRANSIT));
	assert(!(desc.attributes & ATTR_DELIVERED));
	assert(desc.attributes & ATTR_BUFFER_VALID);
	printf("IPC Receive Scenario PASSED\n\n");
}

/** Test matrix domain transitions to MESSAGE. */
void test_matrix_to_message_scenario() {
	printf("--- Test Matrix to Message Scenario (MATRIX -> MESSAGE) ---\n");
	struct SimpleSemanticDescriptor desc   = { DOMAIN_MATRIX, 0 };
	int								result = check_fsm_transition(&desc, DOMAIN_MESSAGE, NULL, 0);
	assert(result == 0);
	assert(desc.domain == DOMAIN_MESSAGE);
	assert(desc.attributes & ATTR_IN_TRANSIT);
	printf("Matrix to Message Scenario PASSED\n\n");
}

/** Test behavior when no transition rule exists. */
void test_fail_no_rule() {
	printf("--- Test Fail No Rule (TEXT -> HEAP) ---\n");
	struct SimpleSemanticDescriptor desc   = { DOMAIN_TEXT, ATTR_EXECUTABLE };
	int								result = check_fsm_transition(&desc, DOMAIN_HEAP, NULL, 0);
	assert(result == EINVAL_ABSTRACT);
	assert(desc.domain == DOMAIN_TEXT);
	printf("Fail No Rule PASSED\n\n");
}

/** Test failure when forbidden attributes are present. */
void test_fail_forbidden_attr() {
	printf("--- Test Fail Forbidden Attr (DATA -> MESSAGE with EXECUTING) ---\n");
	struct SimpleSemanticDescriptor desc   = { DOMAIN_DATA, ATTR_WRITABLE | ATTR_EXECUTING };
	int								result = check_fsm_transition(&desc, DOMAIN_MESSAGE, NULL, 0);
	assert(result == EBUSY_ABSTRACT);
	assert(desc.domain == DOMAIN_DATA);
	printf("Fail Forbidden Attr PASSED\n\n");
}

/** Test failure when required attributes are missing. */
void test_fail_required_attr() {
	printf("--- Test Fail Required Attr (HEAP -> TEXT without COMPILED) ---\n");
	struct SimpleSemanticDescriptor desc   = { DOMAIN_HEAP, ATTR_WRITABLE };
	int								result = check_fsm_transition(&desc, DOMAIN_TEXT, NULL, 0);
	assert(result == EPERM_ABSTRACT);
	assert(desc.domain == DOMAIN_HEAP);
	printf("Fail Required Attr PASSED\n\n");
}

/** Test validation failure when message data is invalid. */
void test_fail_validator() {
	printf("--- Test Fail Validator (DATA -> MESSAGE with bad magic) ---\n");
	char						 buffer[sizeof(struct message_header_local) + 10];
	struct message_header_local* hdr = (struct message_header_local*) buffer;
	hdr->mh_magic					 = 0xBAD0CAFE; // Incorrect magic, made it a valid hex
	hdr->mh_size					 = 5;

	struct SimpleSemanticDescriptor desc   = { DOMAIN_DATA, ATTR_WRITABLE };
	int								result = check_fsm_transition(&desc, DOMAIN_MESSAGE, buffer, sizeof(buffer));
	assert(result == VALIDATOR_FAILED_ABSTRACT);
	assert(desc.domain == DOMAIN_DATA);
	printf("Fail Validator PASSED\n\n");
}

/**
 * Entry point for the FSM unit tests.
 */
int main() {
	printf("Starting Abstract FSM Rule Logic Tests...\n\n");
	test_jit_scenario();
	test_ipc_send_scenario();
	test_ipc_receive_scenario();
	test_matrix_to_message_scenario();
	test_fail_no_rule();
	test_fail_forbidden_attr();
	test_fail_required_attr();
	test_fail_validator();
	printf("\nAll Abstract FSM Rule Logic Tests Completed Successfully.\n");
	return 0;
}
