"""Inspect and install optional solver dependencies for the current Python."""

import argparse
import ctypes.util
import importlib
import importlib.util
from importlib import metadata
from pathlib import Path
import re
import shutil
import subprocess
import sys
from zipfile import BadZipFile

from . import native_assets


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


def doctor_native(names):
    failed = False
    try:
        core_version = metadata.version("maude-se")
    except metadata.PackageNotFoundError:
        print("maude-se is not installed in this Python environment")
        return 1
    for name in names:
        distribution = f"maude-se-native-{name}"
        try:
            version = metadata.version(distribution)
            module = importlib.import_module(f"maude_se_native_{name}")
            library = module.library_path()
            if version != core_version:
                detail = f"version {version} does not match maude-se {core_version}"
            elif not library.is_file():
                detail = f"library is missing: {library}"
            elif not native_assets.ready(name, library.parent / "solver"):
                detail = "upstream shared library is not installed"
            else:
                from maudeSE.maude import loadNativeSmtPlugin, nativeSmtPluginError
                if loadNativeSmtPlugin(str(library), name):
                    detail = f"ready ({version})"
                else:
                    detail = f"cannot load: {nativeSmtPluginError()}"
        except (metadata.PackageNotFoundError, ImportError):
            detail = "not installed"
        print(f"native {name}: {detail}")
        if not detail.startswith("ready"):
            failed = True
            print(f"  Install: maude-se-installer install native {name}")
    return 1 if failed else 0


def install(name, native=False, find_links=None, asset_archive=None):
    try:
        version = metadata.version("maude-se")
    except metadata.PackageNotFoundError:
        print("error: maude-se is not installed in this Python environment", file=sys.stderr)
        return 1

    if native:
        names = SOLVERS if name == "all" else (name,)
        requirements = [f"maude-se-native-{solver}=={version}" for solver in names]
    else:
        extra = "all-solvers" if name == "all" else name
        requirements = ["maude-se[{}]=={}".format(extra, version)]
    print("Installing {} into {}".format(", ".join(requirements), sys.executable), flush=True)
    command = [sys.executable, "-m", "pip", "install"]
    if find_links:
        command += ["--no-index", "--find-links", find_links]
        if native:
            command.append("--no-deps")
    command += requirements
    try:
        result = subprocess.call(command)
        if result or not native:
            return result
        for solver in names:
            importlib.invalidate_caches()
            plugin = importlib.import_module(f"maude_se_native_{solver}")
            native_assets.install(
                solver, plugin.library_path().parent / "solver", archive=asset_archive
            )
        return 0
    except OSError as exc:
        print("error: could not run pip: {}".format(exc), file=sys.stderr)
        return 1
    except (ImportError, RuntimeError, ValueError, BadZipFile) as exc:
        print(f"error: native solver installation failed: {exc}", file=sys.stderr)
        return 1


def uninstall(name, native=False):
    names = SOLVERS if name == "all" else (name,)
    if native:
        distributions = [f"maude-se-native-{solver}" for solver in names]
    else:
        distributions = list(dict.fromkeys(
            distribution
            for solver in names
            for distribution in SOLVERS[solver][0]
        ))
    installed = []
    for distribution in distributions:
        try:
            metadata.version(distribution)
        except metadata.PackageNotFoundError:
            continue
        installed.append(distribution)
    if not installed:
        print("No matching solver packages are installed in {}".format(sys.executable))
        return 0
    if native:
        for solver in names:
            if f"maude-se-native-{solver}" not in installed:
                continue
            plugin = importlib.import_module(f"maude_se_native_{solver}")
            destination = plugin.library_path().parent / "solver"
            if native_assets.managed(solver, destination):
                shutil.rmtree(destination)
    print("Uninstalling {} from {}".format(", ".join(installed), sys.executable), flush=True)
    try:
        return subprocess.call([sys.executable, "-m", "pip", "uninstall", "-y", *installed])
    except OSError as exc:
        print("error: could not run pip: {}".format(exc), file=sys.stderr)
        return 1


def main(argv=None):
    argv = list(sys.argv[1:] if argv is None else argv)
    if argv and argv[0] == "solver":
        argv.pop(0)
    parser = argparse.ArgumentParser(
        prog="maude-se-installer",
        description="Manage Python SMT solvers and native C++ solver plugins in this Python environment.",
        epilog="Examples: install z3 (Python solver); install native z3 (C++ plugin); doctor native",
    )
    actions = parser.add_subparsers(dest="action", required=True)
    for action, description in (
        ("doctor", "Check solver availability"),
        ("install", "Install solver packages"),
        ("uninstall", "Uninstall solver packages"),
    ):
        action_parser = actions.add_parser(action, help=description)
        targets = action_parser.add_subparsers(dest="target", required=action != "doctor")
        for solver in (*SOLVERS, "all"):
            if action == "doctor" and solver == "all":
                continue
            solver_parser = targets.add_parser(solver, help=f"{action.capitalize()} Python {solver} solver packages")
            if action == "install":
                solver_parser.add_argument("--find-links", metavar="DIRECTORY",
                                           help="install from locally built wheels instead of an index")
        native_parser = targets.add_parser("native", help=f"{action.capitalize()} native C++ solver plugins")
        if action == "doctor":
            native_parser.add_argument("name", nargs="?", choices=tuple(SOLVERS),
                                       help="check one solver (default: all)")
        else:
            native_parser.add_argument("name", choices=(*SOLVERS, "all"),
                                       help="solver plugin to manage")
        if action == "install":
            native_parser.add_argument("--find-links", metavar="DIRECTORY",
                                       help="install plugin wheels from a local directory")
            native_parser.add_argument("--asset-archive", metavar="FILE",
                                       help="use a locally downloaded, checksum-verified solver archive")
    args = parser.parse_args(argv)
    native = args.target == "native"
    name = args.name if native else args.target

    if args.action == "doctor":
        names = (name,) if name else SOLVERS
        return doctor_native(names) if native else doctor(names)
    if args.action == "uninstall":
        return uninstall(name, native=native)
    if native and args.asset_archive and name == "all":
        parser.error("--asset-archive requires one native solver")
    return install(name, native=native, find_links=args.find_links,
                   asset_archive=args.asset_archive if native else None)


if __name__ == "__main__":
    sys.exit(main())
