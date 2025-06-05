CC = gcc
# Base CFLAGS, -I. for headers in current dir (e.g. mock_hal_env.h for test_pcid_allocator.c)
CFLAGS_BASE = -Wall -Werror -std=c99 -g -I.

# CFLAGS for test harness files (e.g., test_pcid_allocator.c, mock_hal_env_test.c)
# It needs to find mock_hal_env.h and pcid_x86_test_stubs.h, both in "." (test/hal/)
CFLAGS_TEST_HARNESS = $(CFLAGS_BASE) -DMOCK_HAL_ENV_STANDALONE # For mock_hal_env_test.c
                                                           # test_pcid_allocator.c will get TEST_HAL_PCID_ALLOCATOR separately

# CFLAGS for compiling pcid_x86.c specifically for testing.
# -DTEST_HAL_PCID_ALLOCATOR enables the test-specific code paths in pcid_x86.c.
# -I. allows pcid_x86.c (when compiled from test/hal CWD) to find "pcid_x86_test_stubs.h"
# (assuming pcid_x86_test_stubs.h is in "test/hal/" and pcid_x86.c uses #include "pcid_x86_test_stubs.h")
# -I../../test/hal is added so that pcid_x86_test_stubs.h can find "mock_hal_env.h" using that path if needed,
# but it should find it via -I. if mock_hal_env.h is also in test/hal.
# Crucially, no broad -I../../sys or -I../../sys/arch/i386/include for this test object.
CFLAGS_PCID_TEST_OBJ = -Wall -Werror -std=c99 -g -DTEST_HAL_PCID_ALLOCATOR -I. -I../../test/hal

LDFLAGS = -pthread

.PHONY: all clean check run_tests test_summary

# Default target
all: mock_hal_env_test test_pcid_allocator

# Object files
MOCK_OBJS = mock_hal_env.o
PCID_TEST_OBJ = pcid_x86_test.o
TEST_PCID_ALLOCATOR_OBJ = test_pcid_allocator.o

# Build object files
mock_hal_env.o: mock_hal_env.c mock_hal_env.h
	$(CC) $(CFLAGS_TEST_HARNESS) -DMOCK_HAL_ENV_STANDALONE -c mock_hal_env.c -o mock_hal_env.o

# Rule for compiling pcid_x86.c in test mode
# pcid_x86_test_stubs.h and mock_hal_env.h are expected to be found via -I. and -I../../test/hal
$(PCID_TEST_OBJ): ../../sys/arch/i386/i386/pcid_x86.c pcid_x86_test_stubs.h mock_hal_env.h
	$(CC) $(CFLAGS_PCID_TEST_OBJ) -c ../../sys/arch/i386/i386/pcid_x86.c -o $(PCID_TEST_OBJ)

# Rule for compiling the test_pcid_allocator harness
$(TEST_PCID_ALLOCATOR_OBJ): test_pcid_allocator.c mock_hal_env.h pcid_x86_test_stubs.h
	$(CC) $(CFLAGS_TEST_HARNESS) -DTEST_HAL_PCID_ALLOCATOR -c test_pcid_allocator.c -o $(TEST_PCID_ALLOCATOR_OBJ)

# Build rule for test_pcid_allocator executable
test_pcid_allocator: $(TEST_PCID_ALLOCATOR_OBJ) $(PCID_TEST_OBJ) $(MOCK_OBJS)
	$(CC) $(CFLAGS_TEST_HARNESS) $(TEST_PCID_ALLOCATOR_OBJ) $(PCID_TEST_OBJ) $(MOCK_OBJS) -o $@ $(LDFLAGS)

# Test for mock environment itself
MOCK_HAL_ENV_TEST_OBJ = mock_hal_env_test.o # This variable is not used if .c directly in rule
mock_hal_env_test: mock_hal_env_test.c $(MOCK_OBJS)
	$(CC) $(CFLAGS_TEST_HARNESS) -DMOCK_HAL_ENV_STANDALONE mock_hal_env_test.c $(MOCK_OBJS) -o $@ $(LDFLAGS)

# Updated run_tests target
run_tests: all
	@echo "--- Running Mock HAL Env Test ---"
	./mock_hal_env_test
	@echo "--- Running PCID Allocator Test ---"
	./test_pcid_allocator

check: run_tests

# Updated clean target
clean:
	rm -f *.o mock_hal_env_test test_pcid_allocator $(PCID_TEST_OBJ) $(TEST_PCID_ALLOCATOR_OBJ)
