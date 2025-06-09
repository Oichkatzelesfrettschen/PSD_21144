All code modifications must follow modern formatting conventions and include thorough Doxygen-compatible comments for every function that is touched. After modifications run:
- `./setup.sh`
- `./tests/unit/test_fsm_rules`
- `make check` in `tests/vm`
- `doxygen docs/Doxyfile`
Future contributors should decompose, unroll, flatten, factor, and modernize any legacy constructs encountered.
All touched files must be rewritten in the most modern idiomatic style possible, informed by mathematical reasoning and explicit decomposition, unrolling, flattening, factoring, and synthesis of obsolete patterns.
Every modified file must also be reviewed in the context of the entire architecture.  Functions should be decomposed and refactored for SIMD-friendly algorithms (MMX, SSE1‑4.2, SSSE3, SSE4a, FMA1‑4, AVX1/2/512/512VNNI, and later).  Apply modern formatting only to the code itself—not comments—and ensure all code is documented using Doxygen.
\nContributors must further ensure all decompositions and refactorings follow mathematically proven correctness and use modern idiomatic patterns.
