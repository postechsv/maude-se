"""Check that solver removal targets only the requested packages."""

from importlib import metadata
import contextlib
import io
import sys
import unittest
from unittest.mock import patch

from maudeSE import installer


class UninstallTests(unittest.TestCase):
    def test_help_describes_native_plugin_commands(self):
        for arguments, expected in (
            (["--help"], "install native z3"),
            (["install", "--help"], "native"),
            (["install", "native", "--help"], "solver plugin to manage"),
            (["doctor", "native", "--help"], "default: all"),
        ):
            with self.subTest(arguments=arguments), contextlib.redirect_stdout(io.StringIO()) as output:
                with self.assertRaises(SystemExit) as exit_result:
                    installer.main(arguments)
            self.assertEqual(exit_result.exception.code, 0)
            self.assertIn(expected, output.getvalue())

    def test_native_install_builds_locally(self):
        with patch.object(installer.metadata, "version", return_value="0.0.3"), patch.object(
            installer.native_build, "install"
        ) as build, patch.object(installer, "doctor_native", return_value=0):
            self.assertEqual(installer.main([
                "install", "native", "z3", "--source-dir", "/checkout", "--asset-archive", "local.zip"
            ]), 0)
        build.assert_called_once_with(("z3",), source_dir="/checkout", build_dir=None,
                                      asset_archive="local.zip")

    def test_native_solver(self):
        with patch.object(installer.native_build, "uninstall") as remove:
            self.assertEqual(installer.main(["uninstall", "native", "z3"]), 0)
        remove.assert_called_once_with(("z3",))

    def test_python_yices(self):
        with patch.object(installer.metadata, "version", return_value="1"), patch.object(
            installer.subprocess, "call", return_value=0
        ) as call:
            self.assertEqual(installer.main(["uninstall", "yices"]), 0)
        call.assert_called_once_with(
            [sys.executable, "-m", "pip", "uninstall", "-y", "yices", "yices-solver"]
        )

    def test_absent_plugin_does_not_call_pip(self):
        with patch.object(installer.native_build, "uninstall") as remove, patch.object(
            installer.subprocess, "call"
        ) as call:
            self.assertEqual(installer.main(["uninstall", "native", "all"]), 0)
        call.assert_not_called()
        remove.assert_called_once()


if __name__ == "__main__":
    unittest.main()
