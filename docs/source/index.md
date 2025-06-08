# About MaudeSE

```{toctree}
:hidden:
installation
getting_started
cmds
interface
examples
```


[![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/postechsv/maude-se/release.yml)](https://github.com/postechsv/maude-se)
[![PyPI - Version](https://img.shields.io/pypi/v/maude-se?color=66b2ff)](https://pypi.org/project/maude-se/)
[![GitHub License](https://img.shields.io/github/license/postechsv/maude-se)](https://github.com/postechsv/maude-se)


MaudeSE is a rewriting modulo SMT extension of Maude. It is a flexible yet efficient framework for connecting SMT solvers to Maude. It supports various analysis commands, including symbolic reachability analysis and satisfiability checking under various SMT theories. It also provides an abstract connector at the Python level which makes it easy to integrate and customize SMT solvers without needing to understand Maude's internal implementation.

```{button-link} https://rdcu.be/em1CY
:color: secondary

See our paper
```

---
<!-- 
Maude-SE is an SMT extension of Maude that tightly integrates Maude and SMT solvers with extra functionality. It extends the original Maude-SMT at the core Maude level.
In addition to the existing SMT solving capability of Maude, the tool provides three additional features that are not currently supported by Maude but that are very useful for rewriting modulo SMT: 

- building satisfying assignments by SMT solving, 
- simplifying formulas using the underlying SMT solver, and 
- dealing with non-linear arithmetic formulas. 

Maude-SE can analyze nontrivial systems that cannot be dealt with by the previous Maude-SMT implementation.

There are three versions of Maude SE (i.e., Z3, Yices2, CVC4), depending on an underlying SMT solver it uses. -->


## Bug report

To report bugs or provide suggestions, contact [maude-se@postech.ac.kr](mailto:maude-se@postech.ac.kr) or 
open an issue on our GitHub repository: [https://github.com/postechsv/maude-se/issues](https://github.com/postechsv/maude-se/issues).