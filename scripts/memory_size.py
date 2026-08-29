import argparse
import json
import os
import re
import shutil
import subprocess
from ast import literal_eval
from typing import Dict, Optional, Tuple

filesystem_size = 0
EEPROM_reserve = 4 * 1024  # 4 KiB EEPROM


def convert_size_expression_to_int(expression):
    conversion_factors = {
        "M": 1024 * 1024,
        "MB": 1024 * 1024,
        "K": 1024,
        "KB": 1024,
        "B": 1,
        "": 1,
    }
    extract_regex = r"^((?:[0-9]*[.])?[0-9]+)([mkbMKB]*)$"
    res = re.findall(extract_regex, str(expression))
    if not res:
        return 0
    number, factor = res[0]
    return int(float(number) * conversion_factors.get(factor.upper(), 1))


def find_linker_scripts(search_dirs=None):
    if search_dirs is None:
        search_dirs = [
            os.path.join(os.getcwd(), ".pio", "build"),
            os.path.join(os.getcwd(), ".."),
            os.getcwd(),
        ]

    found = []
    for d in search_dirs:
        if not os.path.isdir(d):
            continue
        for root, _, files in os.walk(d):
            for f in files:
                if f.endswith(".ld") and f.startswith("memmap_default"):
                    found.append(os.path.join(root, f))
    return found


def parse_memory_from_ld(path: str) -> Dict[str, int]:
    with open(path, "r", encoding="utf-8", errors="ignore") as fh:
        text = fh.read()

    mem_block = re.search(r"MEMORY\s*\{(.+?)\}", text, re.S)
    if not mem_block:
        return {}

    content = mem_block.group(1)
    memories = {}

    def _parse_length(length_str: str):
        if not length_str:
            return None
        s = length_str.strip()
        s = re.sub(r"/\*.*?\*/", "", s).strip()

        try:
            if s.lower().startswith("0x"):
                return int(s, 0)
        except Exception:
            pass

        def _suf_repl(m):
            num = m.group(1)
            suf = m.group(2).lower()
            return f"({num}*1024)" if suf == "k" else f"({num}*1024*1024)"

        s2 = re.sub(r"(\d+)\s*([kKmM])\b", _suf_repl, s)
        if re.match(r"^[0-9+\-*/().\s]+$", s2):
            try:
                return int(literal_eval(s2))
            except Exception:
                return None

        try:
            return int(s)
        except Exception:
            return None

    for line in content.splitlines():
        line = line.strip()
        if not line or line.startswith("/*"):
            continue
        line = re.sub(r"/\*.*?\*/", "", line).strip()
        m = re.search(
            r"(\w+)\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*([^,]+),\s*LENGTH\s*=\s*(.+)",
            line,
            re.IGNORECASE,
        )
        if m:
            name = m.group(1)
            length = _parse_length(m.group(3).strip())
            if length is not None:
                memories[name.upper()] = length
    return memories


def search_board_config():
    candidates = []
    root = os.getcwd()
    candidates += [
        os.path.join(root, ".pio", "platforms", "raspberrypi", "boards"),
        os.path.join(root, "boards"),
        os.path.join(root, ".pio", "build"),
    ]
    seen = set()
    for base in candidates:
        if not os.path.isdir(base):
            continue
        for root_dir, _, files in os.walk(base):
            for fname in files:
                if fname.endswith(".json"):
                    path = os.path.join(root_dir, fname)
                    if fname in seen and path not in seen:
                        continue
                    seen.add(path)
                    if "waveshare" in fname.lower() or "board" in fname.lower():
                        yield path
    for base in candidates:
        if not os.path.isdir(base):
            continue
        for root_dir, _, files in os.walk(base):
            for fname in files:
                if fname.endswith(".json"):
                    path = os.path.join(root_dir, fname)
                    if "waveshare" in fname.lower() or "board" in fname.lower():
                        yield path


def detect_memory_limits() -> Tuple[int, int]:
    board_limits = None
    for app_dir in [os.getcwd(), os.path.dirname(os.path.dirname(os.path.abspath(__file__)))]:
        for root, _, files in os.walk(app_dir):
            if "waveshare_rp2040_zero.json" in files:
                board_path = os.path.join(root, "waveshare_rp2040_zero.json")
                with open(board_path, "r", encoding="utf-8") as fp:
                    board_limits = json.load(fp)
                break
            if "boards" in os.path.basename(root) and root.endswith("boards"):
                for f in files:
                    if f.endswith(".json"):
                        board_path = os.path.join(root, f)
                        try:
                            with open(board_path, "r", encoding="utf-8") as fp:
                                data = json.load(fp)
                            if isinstance(data, dict) and data.get("upload", {}).get("maximum_size"):
                                board_limits = data
                                break
                        except Exception:
                            continue
            if board_limits:
                break
        if board_limits:
            break

    if board_limits:
        flash = int(board_limits.get("upload", {}).get("maximum_size", 0))
        ram = int(board_limits.get("upload", {}).get("maximum_ram_size", 0))
        if flash and ram:
            return flash, ram

    scripts = find_linker_scripts()
    memories = {}
    for s in scripts:
        parsed = parse_memory_from_ld(s)
        if parsed:
            memories.update(parsed)

    flash = None
    ram = None
    for key, value in memories.items():
        if key in ("FLASH", "ROM", "FLASH0", "QT_FLASH") and flash is None:
            flash = value
        if key in ("RAM", "SRAM", "RAM0", "SRAM0") and ram is None:
            ram = value

    if flash is None or ram is None:
        if memories:
            ordered = sorted(memories.items(), key=lambda item: item[1], reverse=True)
            flash = flash if flash is not None else ordered[0][1]
            ram = ram if ram is not None else ordered[1][1] if len(ordered) > 1 else ordered[0][1]

    if flash is None:
        flash = 2 * 1024 * 1024
    if ram is None:
        ram = 512 * 1024

    return int(flash), int(ram)


def parse_size_output(output: str) -> Dict[str, int]:
    def _calculate_size(output_text: str, pattern: str) -> int:
        if not output_text or not pattern:
            return -1
        total = 0
        regexp = re.compile(pattern)
        for line in output_text.splitlines():
            line = line.strip()
            if not line:
                continue
            match = regexp.search(line)
            if not match:
                continue
            total += sum(int(value) for value in match.groups())
        return total

    program_pattern = r"^(?:\.boot2|\.text|\.data|\.rodata|\.text\.align|\.ARM\.exidx)\s+(\d+).*"
    data_pattern = r"^(?:\.data|\.bss|\.noinit)\s+(\d+).*"
    program_size = _calculate_size(output, program_pattern)
    data_size = _calculate_size(output, data_pattern)
    return {"program_size": program_size, "data_size": data_size}


def find_elf_file(explicit_path: Optional[str] = None):
    if explicit_path:
        if os.path.exists(explicit_path):
            return explicit_path
        return None

    candidates = []
    root = os.getcwd()
    for base in [root, os.path.join(root, ".pio", "build")]:
        if not os.path.isdir(base):
            continue
        for current, _, files in os.walk(base):
            for name in files:
                if name.endswith(".elf"):
                    candidates.append(os.path.join(current, name))
    if candidates:
        candidates.sort()
        return candidates[0]
    return None


def detect_memory_limits(config_path: Optional[str] = None, linker_path: Optional[str] = None) -> Tuple[int, int]:
    board_limits = None
    if config_path:
        path = os.path.abspath(config_path)
        if os.path.exists(path):
            try:
                with open(path, "r", encoding="utf-8") as fp:
                    board_limits = json.load(fp)
            except Exception:
                board_limits = None

    if board_limits is None:
        for app_dir in [os.getcwd(), os.path.dirname(os.path.dirname(os.path.abspath(__file__)))]:
            for root, _, files in os.walk(app_dir):
                if "waveshare_rp2040_zero.json" in files:
                    board_path = os.path.join(root, "waveshare_rp2040_zero.json")
                    with open(board_path, "r", encoding="utf-8") as fp:
                        board_limits = json.load(fp)
                    break
                if "boards" in os.path.basename(root) and root.endswith("boards"):
                    for f in files:
                        if f.endswith(".json"):
                            board_path = os.path.join(root, f)
                            try:
                                with open(board_path, "r", encoding="utf-8") as fp:
                                    data = json.load(fp)
                                if isinstance(data, dict) and data.get("upload", {}).get("maximum_size"):
                                    board_limits = data
                                    break
                            except Exception:
                                continue
                if board_limits:
                    break
            if board_limits:
                break

    if board_limits:
        flash = int(board_limits.get("upload", {}).get("maximum_size", 0))
        ram = int(board_limits.get("upload", {}).get("maximum_ram_size", 0))
        if flash and ram:
            return flash, ram

    scripts = []
    if linker_path:
        if os.path.exists(linker_path):
            scripts.append(os.path.abspath(linker_path))
    if not scripts:
        scripts = find_linker_scripts()

    memories = {}
    for s in scripts:
        parsed = parse_memory_from_ld(s)
        if parsed:
            memories.update(parsed)

    flash = None
    ram = None
    for key, value in memories.items():
        if key in ("FLASH", "ROM", "FLASH0", "QT_FLASH") and flash is None:
            flash = value
        if key in ("RAM", "SRAM", "RAM0", "SRAM0") and ram is None:
            ram = value

    if flash is None or ram is None:
        if memories:
            ordered = sorted(memories.items(), key=lambda item: item[1], reverse=True)
            flash = flash if flash is not None else ordered[0][1]
            ram = ram if ram is not None else ordered[1][1] if len(ordered) > 1 else ordered[0][1]

    if flash is None:
        flash = 2 * 1024 * 1024
    if ram is None:
        ram = 512 * 1024

    return int(flash), int(ram)


def get_memory_usage(elf_path: Optional[str] = None, config_path: Optional[str] = None, linker_path: Optional[str] = None):
    size_bin = shutil.which("arm-none-eabi-size") or "arm-none-eabi-size"
    elf_path = find_elf_file(elf_path)
    if not elf_path or not os.path.exists(elf_path):
        return None

    try:
        output = subprocess.check_output(
            [size_bin, "-A", "-d", elf_path],
            universal_newlines=True,
            stderr=subprocess.STDOUT,
        )
    except (subprocess.CalledProcessError, OSError):
        return None

    mem_used = parse_size_output(output)
    program_max_size, data_max_size = detect_memory_limits(config_path=config_path, linker_path=linker_path)

    result = {
        "program_used": mem_used["program_size"],
        "data_used": mem_used["data_size"],
        "flash_total": program_max_size,
        "flash_sketch": program_max_size,
        "ram_total": data_max_size,
        "flash_used": mem_used["program_size"],
        "ram_used": mem_used["data_size"],
    }
    return result


def _format_available_bytes(value, total):
    percent_raw = float(value) / float(total) if total else 0.0
    blocks_per_progress = 10
    used_blocks = min(int(round(blocks_per_progress * percent_raw)), blocks_per_progress)
    return "[{:{}}] {: 6.1%} (used {:d} bytes from {:d} bytes)".format("=" * used_blocks, blocks_per_progress, percent_raw, value, total)


def format_bytes(n):
    for unit in ("B", "KiB", "MiB"):
        if n < 1024.0:
            return f"{n:.0f} {unit}"
        n /= 1024.0
    return f"{n:.1f} MiB"


def print_memory_usage(memory_info):
    if not memory_info:
        print("Failed to retrieve memory usage information.")
        return

    print("=== Device memory partitioning ===")
    print(f"Total Flash:          {memory_info['flash_total']} bytes ({format_bytes(memory_info['flash_total'])})")
    print(f"  ├─ Sketch area:     {memory_info['flash_sketch']} bytes ({format_bytes(memory_info['flash_sketch'])})")
    print(f"  ├─ Filesystem:      {filesystem_size} bytes ({format_bytes(filesystem_size)})")
    print(f"  └─ EEPROM res:      {EEPROM_reserve} bytes ({format_bytes(EEPROM_reserve)})")
    print(f"Total RAM:            {memory_info['ram_total']} bytes ({format_bytes(memory_info['ram_total'])})")
    print()
    print("=== Summary ===")
    print(f"RAM:   {_format_available_bytes(memory_info['ram_used'], memory_info['ram_total'])}")
    print(f"Flash: {_format_available_bytes(memory_info['flash_used'], memory_info['flash_sketch'])}")


def parse_cli_args():
    parser = argparse.ArgumentParser(description="Report flash/RAM usage with the same base used by PlatformIO.")
    parser.add_argument("--file", dest="elf_path", help="Path to the ELF file to analyze (default: auto-detect).")
    parser.add_argument("--config", dest="config_path", help="Path to the board JSON config (default: auto-detect).")
    parser.add_argument("--ld", dest="linker_path", help="Path to the linker script used for fallback memory limit detection.")
    return parser.parse_args()


if __name__ == "__main__":
    args = parse_cli_args()
    memory_info_json = get_memory_usage(args.elf_path, args.config_path, args.linker_path)
    print(memory_info_json)
    print_memory_usage(memory_info_json)
