All code modifications must follow modern formatting conventions and include thorough Doxygen-compatible comments for every function that is touched. After modifications run:
- `./setup.sh`
- `./tests/unit/test_fsm_rules`
- `make check` in `tests/vm`
- `doxygen docs/Doxyfile`
Future contributors should decompose, unroll, flatten, factor, and modernize any legacy constructs encountered.
