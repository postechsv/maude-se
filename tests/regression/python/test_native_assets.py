"""Exercise upstream archive verification without using the network."""

import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from zipfile import ZipFile

from maudeSE import native_assets


class NativeAssetTests(unittest.TestCase):
    def test_linux_z3_uses_pinned_manylinux_asset(self):
        with patch.object(native_assets.platform, "system", return_value="Linux"), patch.object(
            native_assets.platform, "machine", return_value="x86_64"
        ):
            version, url, digest = native_assets.asset("z3")
        self.assertEqual(version, "4.13.0")
        self.assertIn("manylinux2014_x86_64.whl", url)
        self.assertEqual(len(digest), 64)

    def test_verified_install_and_repeat(self):
        suffix = ".dylib" if native_assets.platform.system() == "Darwin" else ".so"
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / "upstream.zip"
            with ZipFile(archive, "w") as release:
                release.writestr(f"upstream/bin/libz3{suffix}", b"shared-library-test")
                release.writestr("upstream/LICENSE.txt", b"license-test")
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            destination = root / "solver"
            with patch.object(native_assets, "asset", return_value=("4.13.0", "unused", digest)):
                native_assets.install("z3", destination, archive=archive)
                self.assertTrue(native_assets.ready("z3", destination))
                self.assertEqual((destination / f"libz3{suffix}").read_bytes(), b"shared-library-test")
                native_assets.install("z3", destination, archive=root / "missing.zip")

    def test_hash_mismatch_leaves_no_installation(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / "upstream.zip"
            with ZipFile(archive, "w") as release:
                release.writestr("upstream/bin/libz3.so", b"tampered")
            destination = root / "solver"
            with patch.object(native_assets, "asset", return_value=("4.13.0", "unused", "0" * 64)):
                with self.assertRaisesRegex(RuntimeError, "SHA-256 mismatch"):
                    native_assets.install("z3", destination, archive=archive)
            self.assertFalse(destination.exists())

    def test_yices_wheel_extracts_shared_library_only(self):
        filename = ("libyices.2.dylib" if native_assets.platform.system() == "Darwin"
                    else "libyices.so.2.6.5")
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / "yices.whl"
            with ZipFile(archive, "w") as release:
                release.writestr(f"yices_solver/lib/{filename}", b"shared-yices-test")
                release.writestr("yices_solver/lib/libyices.a", b"static-yices-test")
                release.writestr("yices_solver-2.6.5.post24.dist-info/licenses/LICENSE", b"GPL")
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            destination = root / "solver"
            with patch.object(native_assets, "asset", return_value=("2.6.5.post24", "unused", digest)):
                native_assets.install("yices", destination, archive=archive)
                self.assertTrue(native_assets.ready("yices", destination))
            self.assertEqual((destination / filename).read_bytes(), b"shared-yices-test")
            self.assertFalse((destination / "libyices.a").exists())
            self.assertEqual((destination / "LICENSE").read_bytes(), b"GPL")

    def test_linux_yices_soname_alias(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            archive = root / "yices.whl"
            with ZipFile(archive, "w") as release:
                release.writestr("yices_solver/lib/libyices.so.2.6.5", b"shared-yices-test")
            digest = hashlib.sha256(archive.read_bytes()).hexdigest()
            destination = root / "solver"
            with patch.object(native_assets.platform, "system", return_value="Linux"), patch.object(
                native_assets, "asset", return_value=("2.6.5.post24", "unused", digest)
            ):
                native_assets.install("yices", destination, archive=archive)
                self.assertTrue(native_assets.ready("yices", destination))
            self.assertEqual((destination / "libyices.so.2.6").read_bytes(), b"shared-yices-test")


if __name__ == "__main__":
    unittest.main()
