"""Native Z3 connector binary discovery."""

from pathlib import Path
import sys


def library_path() -> Path:
    suffix = ".dylib" if sys.platform == "darwin" else ".so"
    return Path(__file__).with_name("libmaude_se_z3" + suffix)
