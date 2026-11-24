import importlib.util
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("check_boot_image", Path(__file__).resolve().parents[1] / "scripts/check_boot_image.py")
check_boot_image = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check_boot_image)
validate_boot2 = check_boot_image.validate_boot2


class BootImageTests(unittest.TestCase):
    def test_accepts_boot2_at_start_of_flash(self):
        validate_boot2("  0 .boot2        00000100  10000000  10000000  00001000  2**0")

    def test_rejects_legacy_linker_placement(self):
        with self.assertRaisesRegex(RuntimeError, "invalid RP2040 boot2 placement"):
            validate_boot2("  1 .boot2        00000100  10004800  10004800  00005800  2**0")


if __name__ == "__main__":
    unittest.main()
