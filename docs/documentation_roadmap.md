# Documentation Roadmap

This roadmap outlines the tasks required to document the PSD_21144 codebase using Doxygen and Sphinx.

1. **Inventory Source Trees**
   - Review the `sys/` directory and identify public headers and modules.
   - Audit other top-level directories (`bin/`, `lib/`, etc.) for source files.

2. **Doxygen Comments**
   - Gradually annotate functions, data structures and global variables with
     `///` or `/** */` comments.
   - Focus on kernel interfaces in `sys/` first, then expand to userland tools.

3. **Build Documentation**
   - Run `doxygen docs/Doxyfile` to generate API reference HTML under
     `docs/doxygen/html`.
   - Integrate Sphinx for higher level guides and link to the Doxygen output.

4. **Continuous Improvement**
   - Fix warnings reported by Doxygen and Sphinx.
   - Keep the documentation in sync with code changes via CI jobs.
