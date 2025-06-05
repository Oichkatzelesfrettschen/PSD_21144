#include <stdarg.h> // Must be first for __gnuc_va_list
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "mock_hal_env.h"
#include "pcid_x86_test_stubs.h"

// TEST_HAL_PCID_ALLOCATOR macro is defined by Makefile for pcid_x86_test.o,
// and for this file (test_pcid_allocator.o)

// --- Forward declarations for functions from pcid_x86.c ---
extern void x86_pcid_init(void);
extern uint64_t x86_allocate_pcid(struct proc *p);
extern void x86_release_pcid(uint64_t pcid_gen_asid);
extern uint32_t x86_get_pcid_generation(void);

// --- Test Globals ---
// mock_hal_env_t test_env; // Removed, will use global g_mock_env

// --- Test Helper ---
void setup_test_env_and_pcid(void) {
    mock_hal_env_init(); // Initializes global g_mock_env
    assert(g_mock_env != NULL); // Ensure g_mock_env is set
    x86_pcid_init();
}

// Comparison function for qsort
static int compare_pcids(const void* a, const void* b) {
    uint32_t pcid_a = (*(const uint64_t*)a) & 0xFFF;
    uint32_t pcid_b = (*(const uint64_t*)b) & 0xFFF;
    if (pcid_a < pcid_b) return -1;
    if (pcid_a > pcid_b) return 1;
    return 0;
}

// --- Test Cases ---
void test_pcid_initial_state(void) {
    printf("--- Test: PCID Initial State ---\n");
    fflush(stdout);
    setup_test_env_and_pcid();

    struct proc dummy_proc; // Uses stub from pcid_x86_test_stubs.h
    uint64_t asid = x86_allocate_pcid(&dummy_proc);
    uint32_t pcid = asid & 0xFFF;
    uint32_t gen = asid >> 32;

    printf("Initial allocation: Gen %u, PCID %u (ASID 0x%llx)\n", gen, pcid, (unsigned long long)asid);
    fflush(stdout);
    assert(gen == 1);
    assert(pcid == 1);
    assert(g_mock_env->metrics.pcid_alloc_count == 1);
    printf("PCID Initial State Test PASSED\n\n");
    fflush(stdout);
}

void test_pcid_basic_allocation_release_reuse(void) {
    printf("--- Test: PCID Basic Allocation, Release, Reuse ---\n");
    fflush(stdout);
    setup_test_env_and_pcid();
    struct proc dummy_procs[3];
    uint64_t asid[3];
    uint32_t pcid[3], gen[3];

    for (int i = 0; i < 3; ++i) {
        asid[i] = x86_allocate_pcid(&dummy_procs[i]);
        pcid[i] = asid[i] & 0xFFF;
        gen[i] = asid[i] >> 32;
        printf("Allocated: Gen %u, PCID %u (ASID 0x%llx)\n", gen[i], pcid[i], (unsigned long long)asid[i]);
        fflush(stdout);
        assert(gen[i] == 1);
        assert(pcid[i] == (uint32_t)(i + 1));
        assert(g_mock_env->metrics.pcid_alloc_count == (uint32_t)(i + 1));
    }

    printf("Releasing PCID %u (Gen %u)\n", pcid[1], gen[1]);
    fflush(stdout);
    x86_release_pcid(asid[1]);
    assert(g_mock_env->metrics.pcid_release_count == 1);

    uint64_t new_asid = x86_allocate_pcid(&dummy_procs[0]);
    uint32_t new_pcid = new_asid & 0xFFF;
    uint32_t new_gen = new_asid >> 32;
    printf("Allocated after release: Gen %u, PCID %u (ASID 0x%llx)\n", new_gen, new_pcid, (unsigned long long)new_asid);
    fflush(stdout);

    assert(new_gen == 1);
    assert(new_pcid == pcid[1]);
    assert(g_mock_env->metrics.pcid_alloc_count == 4);

    x86_release_pcid(asid[0]);
    x86_release_pcid(asid[2]);
    assert(g_mock_env->metrics.pcid_release_count == 3);

    new_asid = x86_allocate_pcid(&dummy_procs[0]);
    assert((new_asid & 0xFFF) == pcid[0]);
    new_asid = x86_allocate_pcid(&dummy_procs[0]);
    assert((new_asid & 0xFFF) == pcid[2]);
    assert(g_mock_env->metrics.pcid_alloc_count == 6);

    printf("PCID Basic Allocation, Release, Reuse Test PASSED\n\n");
    fflush(stdout);
}

void test_pcid_double_release(void) {
    printf("--- Test: PCID Double Release ---\n");
    fflush(stdout);
    setup_test_env_and_pcid();
    struct proc dummy_proc;
    uint64_t asid1 = x86_allocate_pcid(&dummy_proc);
    uint32_t pcid1 = asid1 & 0xFFF;
    uint32_t initial_alloc_count = g_mock_env->metrics.pcid_alloc_count;
    uint32_t initial_release_count = g_mock_env->metrics.pcid_release_count;

    printf("Releasing PCID %u once\n", pcid1);
    fflush(stdout);
    x86_release_pcid(asid1);
    assert(g_mock_env->metrics.pcid_release_count == initial_release_count + 1);

    printf("Releasing PCID %u again (double release)\n", pcid1);
    fflush(stdout);
    x86_release_pcid(asid1);
    assert(g_mock_env->metrics.pcid_release_count == initial_release_count + 2);

    uint64_t asid_new = x86_allocate_pcid(&dummy_proc);
    assert((asid_new & 0xFFF) == pcid1);
    assert(g_mock_env->metrics.pcid_alloc_count == initial_alloc_count + 1);

    printf("PCID Double Release Test PASSED\n\n");
    fflush(stdout);
}

void test_pcid_wraparound(void) {
    printf("--- Test: PCID Wraparound ---\n");
    fflush(stdout);
    setup_test_env_and_pcid();
    struct proc dummy_proc;
    uint64_t asid;
    uint32_t pcid, gen;
    uint32_t expected_alloc_count = 0;

    for (int i = 1; i < PCID_MAX; i++) { // PCID_MAX is from pcid_x86_test_stubs.h
        asid = x86_allocate_pcid(&dummy_proc);
        pcid = asid & 0xFFF;
        gen = asid >> 32;
        assert(gen == 1);
        assert(pcid == (uint32_t)i);
        expected_alloc_count++;
        assert(g_mock_env->metrics.pcid_alloc_count == expected_alloc_count);
    }
    assert(g_mock_env->metrics.pcid_wraparound_count == 0);

    printf("Attempting allocation that should trigger wraparound (current alloc count: %u)...\n", expected_alloc_count);
    fflush(stdout);

    // Before triggering wraparound, set the test flag in g_mock_env
    if (g_mock_env) g_mock_env->force_pcid_exhaustion = true;

    asid = x86_allocate_pcid(&dummy_proc);

    if (g_mock_env) g_mock_env->force_pcid_exhaustion = false; // Reset flag

    pcid = asid & 0xFFF;
    gen = asid >> 32;
    expected_alloc_count++;

    printf("After wraparound: PCID=%u, Gen=%u, AllocCount=%u, WrapCount=%u\n",
           pcid, gen, g_mock_env->metrics.pcid_alloc_count, g_mock_env->metrics.pcid_wraparound_count);
    fflush(stdout);

    assert(gen == 2);
    assert(pcid == 1);
    assert(g_mock_env->metrics.pcid_alloc_count == expected_alloc_count);
    assert(g_mock_env->metrics.pcid_wraparound_count == 1);

    // Verify bitmap reset indirectly by trying to allocate many PCIDs again in the new generation.
    bool re_alloc_ok = true;
    for (int i = 1; i < PCID_MAX -1 ; ++i) {
        uint64_t temp_asid = x86_allocate_pcid(&dummy_proc);
        if (((temp_asid & 0xFFF) == 0) || ((temp_asid >> 32) != 2)) { // PCID 0 invalid, gen must be 2
            re_alloc_ok = false;
            printf("Error re-allocating after wrap: PCID %lu, Gen %lu\n", (unsigned long)(temp_asid & 0xFFF), (unsigned long)(temp_asid >> 32));
            fflush(stdout);
            break;
        }
    }
    assert(re_alloc_ok);

    printf("PCID Wraparound Test PASSED\n\n");
    fflush(stdout);
}

typedef struct {
    int num_allocs;
    uint64_t *output_pcids;
    int thread_id;
} thread_arg_t;

void *concurrent_alloc_worker(void *arg) {
    thread_arg_t *th_arg = (thread_arg_t *)arg;
    struct proc dummy_proc;
    for (int i = 0; i < th_arg->num_allocs; i++) {
        th_arg->output_pcids[i] = x86_allocate_pcid(&dummy_proc);
        assert((th_arg->output_pcids[i] & 0xFFF) != 0);
    }
    return NULL;
}

void test_pcid_concurrent_allocation(void) {
    printf("--- Test: PCID Concurrent Allocation ---\n");
    fflush(stdout);
    setup_test_env_and_pcid();

    #define NUM_CONCURRENT_THREADS 8
    #define ALLOCS_PER_CONCURRENT_THREAD ( (PCID_MAX / NUM_CONCURRENT_THREADS) / 2 )

    pthread_t threads[NUM_CONCURRENT_THREADS];
    thread_arg_t args[NUM_CONCURRENT_THREADS];
    uint64_t *all_allocated_pcids = malloc(sizeof(uint64_t) * NUM_CONCURRENT_THREADS * ALLOCS_PER_CONCURRENT_THREAD);
    assert(all_allocated_pcids != NULL);

    int total_allocs_attempted = 0;

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        args[i].num_allocs = ALLOCS_PER_CONCURRENT_THREAD;
        args[i].output_pcids = &all_allocated_pcids[total_allocs_attempted];
        args[i].thread_id = i;
        total_allocs_attempted += args[i].num_allocs;
        if (pthread_create(&threads[i], NULL, concurrent_alloc_worker, &args[i]) != 0) {
            perror("Failed to create thread");
            assert(0);
        }
    }

    for (int i = 0; i < NUM_CONCURRENT_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    assert(g_mock_env->metrics.pcid_alloc_count == (uint32_t)total_allocs_attempted);
    printf("Total PCIDs allocated concurrently: %u\n", total_allocs_attempted);
    fflush(stdout);

    uint32_t first_gen = (uint32_t)(all_allocated_pcids[0] >> 32);
    bool all_same_gen = true;
    for (int i = 1; i < total_allocs_attempted; i++) {
        if ((uint32_t)(all_allocated_pcids[i] >> 32) != first_gen) {
            all_same_gen = false;
            break;
        }
    }

    if (all_same_gen) {
        printf("All allocations were in the same generation (%u). Checking for PCID uniqueness.\n", first_gen);
        fflush(stdout);
        qsort(all_allocated_pcids, total_allocs_attempted, sizeof(uint64_t), compare_pcids);
        for (int i = 0; i < total_allocs_attempted - 1; i++) {
            assert( (all_allocated_pcids[i] & 0xFFF) != (all_allocated_pcids[i+1] & 0xFFF) );
        }
         printf("No duplicate PCIDs found in the same generation.\n");
         fflush(stdout);
    } else {
        printf("WARN: PCIDs allocated across multiple generations in concurrency test. Uniqueness check more complex and might be fine.\n");
        fflush(stdout);
        // This might be okay if total_allocs_attempted > PCID_MAX, forcing a wrap.
        // For this test size, it should ideally be same generation.
        assert(all_same_gen == true); // Forcing this test to expect same generation for given numbers
    }

    free(all_allocated_pcids);
    printf("PCID Concurrent Allocation Test PASSED\n\n");
    fflush(stdout);
}


int main(void) {
    printf("Starting PCID Allocator Unit Tests...\n");
    fflush(stdout);
    test_pcid_initial_state();
    test_pcid_basic_allocation_release_reuse();
    test_pcid_double_release();
    test_pcid_wraparound();
    test_pcid_concurrent_allocation();
    printf("\nAll PCID allocator tests completed.\n");
    fflush(stdout);
    return 0;
}
