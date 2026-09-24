"""Inspect and install optional solver dependencies for the current Python."""

import argparse
import ctypes.util
import importlib
import importlib.util
from importlib import metadata
from pathlib import Path
import re
import subprocess
import sys


SOLVERS = {
    "z3": (("z3-solver",), "z3"),
    "yices": (("yices", "yices-solver"), "yices"),
    "cvc5": (("cvc5",), "cvc5"),
}


def import_solver(name, module):
    if name != "yices":
        return importlib.import_module(module)

    spec = importlib.util.find_spec("yices_solver")
    if spec is None or not spec.submodule_search_locations:
        return importlib.import_module(module)
    library_dir = Path(next(iter(spec.submodule_search_locations))) / "lib"
    libraries = sorted(library_dir.glob("libyices*.dylib")) + sorted(
        library_dir.glob("libyices*.so*")
    )
    if not libraries:
        return importlib.import_module(module)

    original_find_library = ctypes.util.find_library

    def find_library(name):
        if name == "yices":
            return str(libraries[0])
        return original_find_library(name)

    ctypes.util.find_library = find_library
    try:
        return importlib.import_module(module)
    finally:
        ctypes.util.find_library = original_find_library


def required_versions():
    versions = {}
    for requirement in metadata.requires("maude-se") or ():
        match = re.match(r"^([A-Za-z0-9_.-]+)==([^;\s]+)", requirement)
        if match:
            marker = requirement.split(";", 1)[1] if ";" in requirement else ""
            if re.search(r"python_version\s*<\s*['\"]3\.9['\"]", marker) and sys.version_info >= (3, 9):
                continue
            if re.search(r"python_version\s*>=\s*['\"]3\.9['\"]", marker) and sys.version_info < (3, 9):
                continue
            versions[match.group(1).lower().replace("_", "-")] = match.group(2)
    return versions


def check_solver(name):
    distributions, module = SOLVERS[name]
    expected_versions = required_versions()
    installed = []
    for distribution in distributions:
        try:
            version = metadata.version(distribution)
        except metadata.PackageNotFoundError:
            return False, "{} is not installed".format(distribution)
        expected = expected_versions.get(distribution)
        if expected and version != expected:
            return False, "{} {} is installed; MaudeSE requires {}".format(
                distribution, version, expected
            )
        installed.append("{} {}".format(distribution, version))

    try:
        imported = import_solver(name, module)
        if name == "z3":
            imported.Solver()
        elif name == "yices":
            imported.Config()
        else:
            imported.Solver()
    except Exception as exc:
        return False, "packages are installed but {} cannot run: {}".format(name, exc)

    return True, "ready ({})".format(", ".join(installed))


def doctor(names):
    failed = False
    for name in names:
        ready, detail = check_solver(name)
        print("{}: {}".format(name, detail))
        if not ready:
            failed = True
            print("  Install: maude-se-installer install {}".format(name))
    return 1 if failed else 0


def install(name):
    try:
        version = metadata.version("maude-se")
    except metadata.PackageNotFoundError:
        print("error: maude-se is not installed in this Python environment", file=sys.stderr)
        return 1

    extra = "all-solvers" if name == "all" else name
    requirement = "maude-se[{}]=={}".format(extra, version)
    print("Installing {} into {}".format(requirement, sys.executable), flush=True)
    try:
        return subprocess.call([sys.executable, "-m", "pip", "install", requirement])
    except OSError as exc:
        print("error: could not run pip: {}".format(exc), file=sys.stderr)
        return 1


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] == "solver":
        argv.pop(0)

    parser = argparse.ArgumentParser(
        prog="maude-se-installer",
        description="Check or install MaudeSE solver packages in this Python environment.",
    )
    actions = parser.add_subparsers(dest="action", required=True)
    actions.add_parser("doctor", help="Check installed solver packages").add_argument(
        "name", nargs="?", choices=tuple(SOLVERS), help="Check one solver (default: all)"
    )
    actions.add_parser("install", help="Install solver packages").add_argument(
        "name", choices=tuple(SOLVERS) + ("all",)
    )
    args = parser.parse_args(argv)

    if args.action == "doctor":
        return doctor((args.name,) if args.name else SOLVERS)
    return install(args.name)


if __name__ == "__main__":
    sys.exit(main())
