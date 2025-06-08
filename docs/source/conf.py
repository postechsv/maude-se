import os
import sys
sys.path.insert(0, os.path.abspath('..'))


from docutils import nodes
from docutils.parsers.rst import Directive

class JustifyDirective(Directive):
    has_content = True

    def run(self):
        paragraph_node = nodes.paragraph(text='\n'.join(self.content))
        paragraph_node['classes'].append('text-justify')
        return [paragraph_node]

def setup(app):
    app.add_directive("text-justify", JustifyDirective)


# Project info
project = 'MaudeSE'
author = 'Geunyeol Yu'
copyright = '2019-2025, Geunyeol Yu'

html_show_sourcelink = False
html_last_updated_fmt = '%b %d, %Y'


# Extensions
extensions = [
    'myst_parser',
    'sphinx_design',
    'sphinxcontrib.bibtex',
]

source_suffix = {
    '.rst': 'restructuredtext',
    '.md': 'markdown',
}

# Theme
# html_theme = 'furo'
html_theme = 'sphinx_rtd_theme'

# register Maude lexer
from _maude_lexer import MaudeLexer
from sphinx.highlighting import lexers
lexers['maude'] = MaudeLexer()


html_static_path = ['_static']
html_extra_path = ['assets']
html_css_files = ['custom.css']

bibtex_bibfiles = ['refs.bib']