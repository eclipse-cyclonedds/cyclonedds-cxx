# -- Project information -----------------------------------------------------

project = '@PROJECT_NAME@'
copyright = '2021, @PROJECT_NAME@ committers'
author = '@PROJECT_NAME@ committers'

# The short X.Y version
version = '@PROJECT_VERSION_MAJOR@.@PROJECT_VERSION_MINOR@.@PROJECT_VERSION_PATCH@'

# The full version, including alpha/beta/rc tags
release = '@PROJECT_VERSION@'


# -- General configuration ---------------------------------------------------

extensions = [
    'breathe',
    'sphinx.ext.intersphinx',
    "sphinx.ext.viewcode",
    "sphinx.ext.autosummary"
]

templates_path = ['_templates']
language = 'en'
exclude_patterns = ['_build', 'Thumbs.db', '.DS_Store']


# -- Options for HTML output -------------------------------------------------

try:
    import piccolo_theme
    extensions.append('piccolo_theme')
    html_theme = 'piccolo_theme'
except ImportError:
    import warnings
    warnings.warn("piccolo_theme is not installed. Falling back to alabaster.")
    html_theme = 'alabaster'


html_static_path = ['_static']
html_css_files = ['/_static/style.css']

autosummary_generate=True

breathe_domain_by_extension = { "h" : "cpp" , "c" : "cpp", "hpp": "cpp", "cpp": "cpp"}
breathe_show_define_initializer = True
breathe_show_include = True

rst_epilog = '\n'.join(map(lambda x: f".. |var-{x[0]}| replace:: {x[1]}", {
    "project": "Eclipse Cyclone DDS: C++ Binding",
    "project-short": "Cyclone DDS C++",
    "core-project": "Eclipse Cyclone DDS",
    "core-project-short": "Cyclone DDS"
}.items()))
