import argparse
import re
import subprocess


def validate_boot2(section_output: str) -> None:
    match = re.search(r"^\s*\d+\s+\.boot2\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)\s+([0-9a-fA-F]+)", section_output, re.MULTILINE)
    if match is None:
        raise RuntimeError("ELF does not contain a .boot2 section")

    size, virtual_address, load_address = (int(value, 16) for value in match.groups())
    if size != 0x100 or virtual_address != 0x10000000 or load_address != 0x10000000:
        raise RuntimeError(
            f"invalid RP2040 boot2 placement: size=0x{size:x}, VMA=0x{virtual_address:x}, LMA=0x{load_address:x}"
        )


def main() -> None:
    parser = argparse.ArgumentParser(description="Validate the RP2040 boot2 placement in an ELF image.")
    parser.add_argument("--objdump", required=True)
    parser.add_argument("--file", required=True)
    args = parser.parse_args()
    output = subprocess.check_output([args.objdump, "-h", args.file], text=True)
    validate_boot2(output)


if __name__ == "__main__":
    main()
