"""Locations of locally built native SMT plugins."""

from pathlib import Path
import sys


def package_root():
    import maudeSE
    return Path(maudeSE.__file__).resolve().parent


def plugin_dir(solver):
    if solver not in ("z3", "yices", "cvc5"):
        raise ValueError(f"unknown native solver: {solver}")
    return package_root() / "native" / solver


def library_path(solver):
    suffix = "dylib" if sys.platform == "darwin" else "so"
    return plugin_dir(solver) / f"libmaude_se_{solver}.{suffix}"
