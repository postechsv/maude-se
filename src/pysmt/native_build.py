"""Build native solver connections locally for an installed maude-se wheel."""

from importlib import metadata
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

from . import native_assets, native_plugin


SOURCE_URL = "https://github.com/postechsv/maude-se.git"


def _run(command, cwd, env=None):
    print("Running:", " ".join(map(str, command)), flush=True)
    subprocess.run(command, cwd=cwd, env=env, check=True)


def install(solvers, source_dir=None, build_dir=None, asset_archive=None):
    if sys.platform != "darwin":
        raise RuntimeError("local native plugin compilation currently supports macOS only")
    root = native_plugin.package_root()
    core = root / "maude" / "libmaude.dylib"
    if not core.is_file():
        raise RuntimeError(f"maude-se core library is missing: {core}")
    if source_dir:
        source = Path(source_dir).expanduser().resolve()
        if not (source / "build.sh").is_file():
            raise RuntimeError(f"maude-se source checkout is missing build.sh: {source}")
    else:
        work = Path(build_dir).expanduser().resolve() if build_dir else root / ".native-build"
        work.mkdir(parents=True, exist_ok=True)
        source = work / "maude-se"
        if not source.exists():
            version = metadata.version("maude-se")
            _run(["git", "clone", "--branch", f"v{version}", "--depth", "1", SOURCE_URL, str(source)], work)
        elif not (source / "build.sh").is_file():
            raise RuntimeError(f"build source is invalid: {source}")
        else:
            version = metadata.version("maude-se")
            tag_commit = subprocess.run(
                ["git", "rev-list", "-n", "1", f"v{version}"], cwd=source,
                capture_output=True, text=True, check=True,
            ).stdout.strip()
            head_commit = subprocess.run(
                ["git", "rev-parse", "HEAD"], cwd=source,
                capture_output=True, text=True, check=True,
            ).stdout.strip()
            if tag_commit != head_commit:
                raise RuntimeError(f"cached sources do not match installed maude-se tag v{version}: {source}")
    env = os.environ.copy()
    tool_paths = [str(Path(sys.executable).parent)]
    for tool in ("bison", "flex"):
        prefix = subprocess.run(["brew", "--prefix", tool], capture_output=True,
                                text=True, check=True).stdout.strip()
        tool_paths.append(str(Path(prefix) / "bin"))
    env["PATH"] = os.pathsep.join(tool_paths + [env.get("PATH", "")])
    for command in ("setup", "deps", "build-maude"):
        _run([str(source / "build.sh"), command], source, env)
    for solver in solvers:
        _run([str(source / "build" / "native.sh"), "deps", solver], source, env)
        destination = native_plugin.plugin_dir(solver)
        destination.parent.mkdir(parents=True, exist_ok=True)
        if destination.exists() and not (destination / "maude-se-local-build").is_file():
            raise RuntimeError(f"refusing to replace unmanaged native plugin: {destination}")
        with tempfile.TemporaryDirectory(prefix=f".{solver}-", dir=destination.parent) as temporary:
            stage = Path(temporary) / solver
            stage.mkdir()
            if not asset_archive:
                tag = f"cp{sys.version_info.major}{sys.version_info.minor}"
                cached_asset = (source / ".build-wheel" /
                                f"{tag}-{os.uname().machine}" / "native-plugin-deps" / solver)
                if native_assets.ready(solver, cached_asset):
                    shutil.copytree(cached_asset, stage / "solver")
            suffix = native_plugin.library_path(solver).suffix
            build_env = env.copy()
            build_env["MAUDE_SE_CORE_LIBRARY"] = str(core)
            build_env["MAUDE_SE_PLUGIN_OUTPUT"] = str(stage / f"libmaude_se_{solver}{suffix}")
            build_env["MAUDE_SE_PLUGIN_ASSET_DIR"] = str(stage / "solver")
            if asset_archive:
                build_env["MAUDE_SE_ASSET_ARCHIVE"] = str(Path(asset_archive).expanduser().resolve())
            _run([str(source / "build.sh"), "plugin", solver], source, build_env)
            (stage / "maude-se-local-build").write_text("locally compiled by maude-se-installer\n")
            if destination.exists():
                shutil.rmtree(destination)
            stage.rename(destination)
        print(f"native {solver}: installed at {destination}")


def uninstall(solvers):
    for solver in solvers:
        destination = native_plugin.plugin_dir(solver)
        if not destination.exists():
            continue
        if not destination.is_dir() or destination.is_symlink() or not (destination / "maude-se-local-build").is_file():
            raise RuntimeError(f"refusing to remove unmanaged native plugin: {destination}")
        shutil.rmtree(destination)
        print(f"native {solver}: removed {destination}")
