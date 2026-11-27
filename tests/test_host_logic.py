import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


class HostLogicTests(unittest.TestCase):
    def test_device_config_and_gear_limits_for_both_projects(self):
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("host C++ compiler is not available")

        root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as directory:
            for project in ("SHIFTER_PROJECT_V1", "SHIFTER_PROJECT_V2"):
                executable = Path(directory) / project
                subprocess.run(
                    [
                        compiler,
                        "-std=c++23",
                        f"-DSHIFTER_PROJECT_VERSION={project}",
                        f"-I{root / 'src'}",
                        str(root / "tests/host_logic_test.cpp"),
                        str(root / "src/config/DeviceConfig.cpp"),
                        str(root / "src/shifter/ShifterLogic.cpp"),
                        "-o",
                        str(executable),
                    ],
                    check=True,
                )
                subprocess.run([str(executable)], check=True)

    def test_config_protocol_on_cdc_streams(self):
        compiler = shutil.which("c++")
        if compiler is None:
            self.skipTest("host C++ compiler is not available")

        root = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory() as directory:
            executable = Path(directory) / "config_protocol_test"
            subprocess.run(
                [
                    compiler,
                    "-std=c++23",
                    "-Wall",
                    "-Werror",
                    f"-I{root / 'src'}",
                    str(root / "tests/config_protocol_test.cpp"),
                    str(root / "src/usb/ConfigProtocol.cpp"),
                    "-o",
                    str(executable),
                ],
                check=True,
            )
            subprocess.run([str(executable)], check=True)


if __name__ == "__main__":
    unittest.main()
