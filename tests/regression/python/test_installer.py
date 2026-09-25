"""Check that solver removal targets only the requested packages."""

from importlib import metadata
import sys
import unittest
from unittest.mock import patch

from maudeSE import installer


class UninstallTests(unittest.TestCase):
    def test_native_solver(self):
        with patch.object(installer.metadata, "version", return_value="0.0.3"), patch.object(
            installer.subprocess, "call", return_value=0
        ) as call:
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
