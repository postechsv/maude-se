"""Check native source selection without downloading or compiling dependencies."""

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

from maudeSE.plugins import build as native_build


class SourceCheckoutTests(unittest.TestCase):
    def test_versions_use_distinct_clone_directories(self):
        with tempfile.TemporaryDirectory() as temporary, patch.object(
            native_build.metadata, "version", side_effect=("0.0.4", "0.0.5")
        ), patch.object(native_build, "_run") as run:
            root = Path(temporary)
            old = native_build._source_checkout(root)
            new = native_build._source_checkout(root)

        self.assertEqual(old, root / ".native-build/v0.0.4/maude-se")
        self.assertEqual(new, root / ".native-build/v0.0.5/maude-se")
        self.assertNotEqual(old, new)
        self.assertEqual(run.call_args_list[0].args[0][3], "v0.0.4")
        self.assertEqual(run.call_args_list[1].args[0][3], "v0.0.5")

    def test_explicit_source_does_not_require_a_release_tag(self):
        with tempfile.TemporaryDirectory() as temporary, patch.object(
            native_build.metadata, "version"
        ) as version, patch.object(native_build, "_run") as run:
            source = Path(temporary)
            (source / "build.sh").touch()
            self.assertEqual(native_build._source_checkout(source, source_dir=source), source.resolve())
            version.assert_not_called()
            run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
