import os
import sys

project = "PSD_21144"
extensions = ["breathe"]

breathe_projects = {"PSD_21144": os.path.join("..", "doxygen", "xml")}

breathe_default_project = "PSD_21144"

templates_path = ["_templates"]
exclude_patterns = []
html_theme = "sphinx_rtd_theme"
html_static_path = ["_static"]
