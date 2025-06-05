#include "mock_hal_env.h"
#include <stdio.h>

// Define X86_CR4_PCIDE if not available from a shared header for tests
// This should match the one in mock_hal_env.c
#ifndef X86_CR4_PCIDE
#define X86_CR4_PCIDE (1 << 17)
#endif

int main() {
  // mock_hal_env_t env; // Removed local env
  mock_hal_env_init(); // Initialize the global g_mock_env

  // Ensure g_mock_env is not NULL after init, otherwise tests below are invalid
  assert(g_mock_env != NULL);

  printf("Mock HAL Environment Test Program (using global g_mock_env)\n");
  printf("Initial Mock CR4: 0x%llx\n", (unsigned long long)g_mock_env->cpu.cr4);
  printf("Initial LCR4 count: %u\n", g_mock_env->metrics.lcr4_count);

  // Test if PCID is enabled by default in mock (it should be if cpuid mock says so)
  if (g_mock_env->cpu.cr4 & X86_CR4_PCIDE) {
      printf("PCIDE is enabled in CR4 by default in mock_hal_env_init as expected.\n");
  } else {
      printf("WARN: PCIDE is NOT enabled in CR4 by default in mock_hal_env_init.\n");
      assert(0); // Should be enabled
  }

  // Test lcr4
  mock_x86_lcr4(g_mock_env, g_mock_env->cpu.cr4 & ~X86_CR4_PCIDE);
  printf("Mock CR4 after PCIDE off: 0x%llx\n", (unsigned long long)g_mock_env->cpu.cr4);
  assert((g_mock_env->cpu.cr4 & X86_CR4_PCIDE) == 0);
  assert(g_mock_env->metrics.lcr4_count == 1);

  mock_x86_lcr4(g_mock_env, g_mock_env->cpu.cr4 | X86_CR4_PCIDE);
  printf("Mock CR4 after PCIDE on: 0x%llx\n", (unsigned long long)g_mock_env->cpu.cr4);
  assert((g_mock_env->cpu.cr4 & X86_CR4_PCIDE) != 0);
  assert(g_mock_env->metrics.lcr4_count == 2);

  // Test CPUID mock (basic leaf 1, ecx for PCID bit)
  uint32_t eax, ebx, ecx, edx;
  mock_x86_cpuid(g_mock_env, 0x1, 0, &eax, &ebx, &ecx, &edx);
  printf("Mock CPUID Leaf 1 ECX: 0x%x (PCID bit should be set)\n", ecx);
  assert((ecx & X86_CR4_PCIDE) != 0);

  printf("LCR4 count: %u\n", g_mock_env->metrics.lcr4_count);

  // Test reset functionality
  mock_hal_env_reset();
  assert(g_mock_env != NULL); // Still should be valid
  printf("Mock CR4 after reset: 0x%llx\n", (unsigned long long)g_mock_env->cpu.cr4);
  assert((g_mock_env->cpu.cr4 & X86_CR4_PCIDE) != 0); // Should be re-enabled by default
  assert(g_mock_env->metrics.lcr4_count == 0); // Metrics should be reset
  printf("LCR4 count after reset: %u\n", g_mock_env->metrics.lcr4_count);


  printf("Mock HAL Environment Test PASSED.\n");
  return 0;
}
