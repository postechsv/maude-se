"""Check that solver removal targets only the requested packages."""

from importlib import metadata
from pathlib import Path
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch

from maudeSE import installer


class UninstallTests(unittest.TestCase):
    def test_native_install_fetches_assets(self):
        with patch.object(installer.metadata, "version", return_value="0.0.3"), patch.object(
            installer.subprocess, "call", return_value=0
        ) as call, patch.object(installer.native_assets, "install") as assets, patch.object(
            installer.importlib, "import_module",
            return_value=SimpleNamespace(library_path=lambda: Path("/tmp/libmaude_se_z3.dylib")),
        ):
            self.assertEqual(installer.main([
                "install", "native", "z3", "--find-links", "out", "--asset-archive", "local.zip"
            ]), 0)
        call.assert_called_once_with([
            sys.executable, "-m", "pip", "install", "--no-index", "--find-links", "out",
            "--no-deps", "maude-se-native-z3==0.0.3",
        ])
        self.assertEqual(assets.call_args.kwargs["archive"], "local.zip")

    def test_native_solver(self):
        with patch.object(installer.metadata, "version", return_value="0.0.3"), patch.object(
            installer.subprocess, "call", return_value=0
        ) as call, patch.object(installer.native_assets, "managed", return_value=False), patch.object(
            installer.importlib, "import_module",
            return_value=SimpleNamespace(library_path=lambda: Path("/tmp/libmaude_se_z3.dylib")),
        ):
            self.assertEqual(installer.main(["uninstall", "native", "z3"]), 0)
        call.assert_called_once_with(
            [sys.executable, "-m", "pip", "uninstall", "-y", "maude-se-native-z3"]
        )

    def test_python_yices(self):
        with patch.object(installer.metadata, "version", return_value="1"), patch.object(
            installer.subprocess, "call", return_value=0
        ) as call:
            self.assertEqual(installer.main(["uninstall", "yices"]), 0)
        call.assert_called_once_with(
            [sys.executable, "-m", "pip", "uninstall", "-y", "yices", "yices-solver"]
        )

    def test_absent_plugin_does_not_call_pip(self):
        with patch.object(installer.metadata, "version", side_effect=metadata.PackageNotFoundError), patch.object(
            installer.subprocess, "call"
        ) as call:
            self.assertEqual(installer.main(["uninstall", "native", "all"]), 0)
        call.assert_not_called()


if __name__ == "__main__":
    unittest.main()
