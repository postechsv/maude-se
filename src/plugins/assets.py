"""Pinned upstream shared-library archives for native solver plugins."""

import hashlib
import json
import platform
from pathlib import Path
import shutil
import tempfile
from urllib.request import urlopen
from zipfile import ZipFile


# These hashes are for the selected upstream release files and PyPI wheels.
ASSETS = {
    ("z3", "Darwin", "arm64"): (
        "4.13.0", "z3-4.13.0-arm64-osx-11.0.zip",
        "e7cd325cb2210d3b241d0d5517a293677030f58c1771e196c4574ef99dc45168",
    ),
    ("z3", "Darwin", "x86_64"): (
        "4.13.0", "z3-4.13.0-x64-osx-11.7.10.zip",
        "0c33d8574f7dcd041f1f4e7fe301840db6a527f866cb74b0b47518bf8053502d",
    ),
    ("z3", "Linux", "x86_64"): (
        "4.13.0", "z3_solver-4.13.0.0-py2.py3-none-manylinux2014_x86_64.whl",
        "8c42de82b6e3ff7ee61287d03c7af8a99f9f6554cdd1204c6b9bca96ff1cb7fb",
        "https://files.pythonhosted.org/packages/c6/79/0255fe0efee7ea9db8987ced14c70028a0007d4d4aaaed8965310bbd7bb1/z3_solver-4.13.0.0-py2.py3-none-manylinux2014_x86_64.whl",
    ),
    ("yices", "Darwin", "arm64"): (
        "2.6.5.post24", "yices_solver-2.6.5.post24-py3-none-macosx_14_0_arm64.whl",
        "c856e5d7f6bebd4ec8ccc5eee0713d01cb6b0f4c00fb8eb1a292c93d58f430e9",
        "https://files.pythonhosted.org/packages/c9/9d/13bac80514525cb3189a3aae8088d0a7aefd2bf450f7ec0f8448dea297f2/yices_solver-2.6.5.post24-py3-none-macosx_14_0_arm64.whl",
    ),
    ("yices", "Darwin", "x86_64"): (
        "2.6.5.post24", "yices_solver-2.6.5.post24-py3-none-macosx_13_0_x86_64.whl",
        "168a026b0e6b1715bec10699d498b93089d160afd13060b7dfc1408ece1c045b",
        "https://files.pythonhosted.org/packages/6e/cb/b5289d59ae98cc3ace0e1cd3c86999be738a98a80ec8f4623162da665383/yices_solver-2.6.5.post24-py3-none-macosx_13_0_x86_64.whl",
    ),
    ("yices", "Linux", "x86_64"): (
        "2.6.5.post24", "yices_solver-2.6.5.post24-py3-none-manylinux_2_28_x86_64.whl",
        "d0a7c8fad531fe3a2cd1616bab1b73bd1619604f0bf8fa2d55236e092c2f4219",
        "https://files.pythonhosted.org/packages/28/5c/85c81dcb3d772345a9bea5deb40d7ba4b24c8ab2423e7bd7175db9239aaa/yices_solver-2.6.5.post24-py3-none-manylinux_2_28_x86_64.whl",
    ),
    ("cvc5", "Darwin", "arm64"): (
        "1.4.0", "cvc5-macOS-arm64-shared.zip",
        "af67c7cb42173abf0f4ce628c76c137d2519e4fcff67cff5a440aebb7b1437c7",
    ),
    ("cvc5", "Darwin", "x86_64"): (
        "1.4.0", "cvc5-macOS-x86_64-shared.zip",
        "53c786f9360133d429d18b00bb7dbaaef88411d8a1ad4c253576985501ceb442",
    ),
    ("cvc5", "Linux", "x86_64"): (
        "1.4.0", "cvc5-Linux-x86_64-shared.zip",
        "b797f752e44a6d92f1be98ebfa87cd89e7591e1184a1c0bc7189b039718f2573",
    ),
}


def asset(solver):
    key = (solver, platform.system(), platform.machine())
    if key not in ASSETS:
        raise RuntimeError(f"no supported shared-library asset configured for {key}")
    version, filename, digest, *explicit_url = ASSETS[key]
    if explicit_url:
        return version, explicit_url[0], digest
    project = "Z3Prover/z3" if solver == "z3" else "cvc5/cvc5"
    tag = f"z3-{version}" if solver == "z3" else f"cvc5-{version}"
    url = f"https://github.com/{project}/releases/download/{tag}/{filename}"
    return version, url, digest


def ready(solver, destination):
    try:
        version, _, digest = asset(solver)
        record = json.loads((destination / "maude-se-asset.json").read_text())
        expected = "libz3.dylib" if platform.system() == "Darwin" else "libz3.so"
        if solver == "cvc5":
            expected = "libcvc5.dylib" if platform.system() == "Darwin" else "libcvc5.so"
        elif solver == "yices":
            expected = "libyices.2.dylib" if platform.system() == "Darwin" else "libyices.so.2.6.5"
        return (record == {"solver": solver, "version": version, "sha256": digest}
                and (destination / expected).is_file()
                and (solver != "yices" or platform.system() != "Linux"
                     or (destination / "libyices.so.2.6").is_file()))
    except (OSError, ValueError, RuntimeError):
        return False


def managed(solver, destination):
    try:
        record = json.loads((destination / "maude-se-asset.json").read_text())
        return destination.is_dir() and not destination.is_symlink() and record.get("solver") == solver
    except (OSError, ValueError):
        return False


def install(solver, destination, archive=None):
    version, url, digest = asset(solver)
    destination = Path(destination)
    if ready(solver, destination):
        return destination
    destination.parent.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="maude-se-asset-", dir=destination.parent) as temp:
        temp = Path(temp)
        source = temp / "archive.zip"
        if archive:
            if Path(archive).stat().st_size > 200_000_000:
                raise RuntimeError(f"{solver} upstream archive exceeds 200 MB")
            shutil.copyfile(archive, source)
        else:
            with urlopen(url, timeout=60) as response, source.open("wb") as output:
                total = 0
                while block := response.read(1024 * 1024):
                    total += len(block)
                    if total > 200_000_000:
                        raise RuntimeError(f"{solver} upstream archive exceeds 200 MB")
                    output.write(block)
        actual = hashlib.sha256(source.read_bytes()).hexdigest()
        if actual != digest:
            raise RuntimeError(f"{solver} archive SHA-256 mismatch: {actual}")
        stage = temp / "solver"
        stage.mkdir()
        names = set()
        with ZipFile(source) as release:
            total_uncompressed = 0
            for member in release.infolist():
                path = Path(member.filename)
                if member.is_dir() or len(path.parts) < 2:
                    continue
                filename = path.name
                if solver == "z3":
                    suffix = ".dylib" if platform.system() == "Darwin" else ".so"
                    selected = (path.parent.name in ("bin", "lib")
                                and filename.startswith("libz3") and suffix in filename)
                elif solver == "yices":
                    expected = "libyices.2.dylib" if platform.system() == "Darwin" else "libyices.so.2.6.5"
                    selected = path.parent.name == "lib" and filename == expected
                else:
                    selected = path.parent.name == "lib" and (
                        filename.endswith(".dylib") or ".so" in filename
                    )
                selected |= filename in ("LICENSE.txt", "COPYING") and len(path.parts) == 2
                selected |= (solver == "cvc5" and len(path.parts) == 3
                             and path.parent.name == "licenses" and filename.endswith(".txt"))
                selected |= (solver == "yices" and path.parent.name == "licenses"
                             and filename == "LICENSE" and len(path.parts) == 3)
                if not selected:
                    continue
                if member.file_size > 200_000_000:
                    raise RuntimeError(f"oversized {solver} archive member: {filename}")
                total_uncompressed += member.file_size
                if total_uncompressed > 500_000_000:
                    raise RuntimeError(f"{solver} archive expands beyond 500 MB")
                if filename in names:
                    raise RuntimeError(f"duplicate {solver} archive member: {filename}")
                names.add(filename)
                with release.open(member) as input_file, (stage / filename).open("wb") as output:
                    shutil.copyfileobj(input_file, output)
                (stage / filename).chmod(0o755 if filename.endswith(".dylib") or ".so" in filename else 0o644)
        if solver == "yices" and platform.system() == "Linux":
            (stage / "libyices.so.2.6").symlink_to("libyices.so.2.6.5")
        (stage / "maude-se-asset.json").write_text(json.dumps({
            "solver": solver, "version": version, "sha256": digest,
        }))
        if not ready(solver, stage):
            raise RuntimeError(f"{solver} archive lacks its shared library")
        if destination.exists():
            if not managed(solver, destination):
                raise RuntimeError(f"refusing to replace unmanaged directory: {destination}")
            shutil.rmtree(destination)
        stage.rename(destination)
    return destination
