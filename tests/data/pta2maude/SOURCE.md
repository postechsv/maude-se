# PTA2Maude case studies

These files are adapted from the companion artifact for Arias et al.,
*Rewriting Logic Semantics and Symbolic Analysis for Parametric Timed
Automata* (FTSCS 2022), DOI: 10.1145/3563822.3569923.

- Original repository: https://depot.lipn.univ-paris13.fr/real-time-maude/pta2maude
- Former URL cited by the paper: https://depot.lipn.univ-paris13.fr/arias/pta2maude
- Upstream locations: `maude/`, `maude/examples/`, and `benchmarks/models/`
- Upstream license: GPL-3.0; this repository's top-level `LICENSE` contains
  the GPL-3.0 license text.

The files here are **not byte-for-byte copies** of the current upstream
versions. In particular, `meta-pta.maude` uses Maude-SE's `metaSmtCheck`, and
`coffee.maude` and other models contain local changes. Do not replace these
files with upstream versions without reviewing the semantic differences and
recording the upstream revision used. The source repository was inspected at
commit `4835382e82c5f8aa9b5083d0fcc3a6f7c53f7874`; that commit is a
reference point for comparison, not a claim that the local files were copied
from that exact revision.

`pta-base.maude` and `meta-pta.maude` are shared modules for the folding
examples. Keep them alongside the models so that the existing `load` commands
can resolve from this directory.
