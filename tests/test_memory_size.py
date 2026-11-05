import importlib.util
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("memory_size", Path(__file__).resolve().parents[1] / "scripts" / "memory_size.py")
memory_size = importlib.util.module_from_spec(spec)
spec.loader.exec_module(memory_size)


class MemorySizeTests(unittest.TestCase):
    def test_parse_size_output_counts_pico_sdk_flash_and_static_ram(self):
        output = """.pio/build/waveshare_rp2040_zero/firmware.elf  :
section                size        addr
.boot2                  256   268435456
.ota                  10228   268435712
.partition             1804   268445940
.text                 91660   268447744
.rodata                4376   268539408
.binary_info             32   268543784
.ram_vector_table       192   536870912
.data                  4116   536871104
.tdata                    0   536875220
.tbss                     0   536875220
.bss                  11600   536875224
.heap                246232   536886824
.scratch_x                0   537133056
.scratch_y                0   537137152
.stack1_dummy          2048   537133056
.stack_dummy           2048   537137152
Total               1010038
"""

        parsed = memory_size.parse_size_output(output)

        self.assertEqual(parsed["program_size"], 100440)
        self.assertEqual(parsed["data_size"], 15908)

    def test_parse_size_output_matches_cmake_elf_sections(self):
        output = """H_SHIFTER_JAPATECH.elf  :
section                 size        addr
.boot2                   256   268435456
.text                  22336   268435712
.rodata                 4156   268458048
.binary_info              32   268462204
.ram_vector_table        192   536870912
.data                   3072   536871104
.bss                    3148   536874176
.heap                   2048   536877324
.stack_dummy            2048   537137152
Total                  37288
"""

        parsed = memory_size.parse_size_output(output)

        self.assertEqual(parsed["program_size"], 29852)
        self.assertEqual(parsed["data_size"], 6412)


if __name__ == "__main__":
    unittest.main()
