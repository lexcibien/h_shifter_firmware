import os
import re
import shutil
import subprocess
from typing import Dict
from ast import literal_eval

filesystem_size = 0 * 1024
EEPROM_reserve = 4 * 1024  # 4 KiB EEPROM


def find_linker_scripts(search_dirs=None):
    if search_dirs is None:
        search_dirs = [
            os.path.join(os.getcwd(), "..", ".."),
            os.getcwd(),
        ]

    found = []
    try:
        script_dir = os.path.dirname(os.path.abspath(__file__))
        candidate = os.path.normpath(os.path.join(script_dir, "..", "assets", "memmap_default.ld"))
        if os.path.isfile(candidate):
            found.append(candidate)
        return found
    except Exception:
        pass
    for d in search_dirs:
        if not os.path.isdir(d):
            continue
        for root, _, files in os.walk(d):
            for f in files:
                if f.endswith(".ld") and f.startswith("memmap_default"):
                    found.append(os.path.join(root, f))

    return found


def parse_memory_from_ld(path: str) -> Dict[str, int]:
    """
    Procura bloco MEMORY no linker script e retorna dict {name: length_in_bytes}.
    """
    with open(path, "r", encoding="utf-8", errors="ignore") as fh:
        text = fh.read()

    mem_block = re.search(r"MEMORY\s*\{(.+?)\}", text, re.S)
    if not mem_block:
        return {}

    content = mem_block.group(1)

    # cada linha: NAME (rx) : ORIGIN = 0x..., LENGTH = ...
    memories = {}

    def _parse_length(length_str: str):
        """Parse LENGTH expressions like '2093056', '512k', '4k', '0x2000', or '(2 * 1024 * 1024)'.

        Returns integer number of bytes or None on failure.
        """
        if not length_str:
            return None
        s = length_str.strip()
        # remove trailing comments if any
        s = re.sub(r"/\*.*\*/", "", s).strip()

        # hex literal
        try:
            if s.lower().startswith("0x"):
                return int(s, 0)
        except Exception:
            pass

        # Replace simple suffixes like 512k, 4K, 2M -> numeric expressions
        def _suf_repl(m):
            num = m.group(1)
            suf = m.group(2).lower()
            if suf == "k":
                return f"({num}*1024)"
            return f"({num}*1024*1024)"

        s2 = re.sub(r"(\d+)\s*([kKmM])\b", _suf_repl, s)

        # allow only safe characters before eval
        if re.match(r"^[0-9+\-*/().\s]+$", s2):
            try:
                return int(literal_eval(s2))
            except Exception:
                return None

        # fallback: try plain int
        try:
            return int(s)
        except Exception:
            return None

    for line in content.splitlines():
        line = line.strip()
        if not line or line.startswith("/*"):
            continue

        # Remover comentários inline
        line = re.sub(r"/\*.*?\*/", "", line).strip()

        # Capturar: NAME (rx) : ORIGIN = ..., LENGTH = ...
        m = re.search(r"(\w+)\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*([^,]+),\s*LENGTH\s*=\s*(.+)", line, re.IGNORECASE)
        if m:
            name = m.group(1)
            length_str = m.group(3).strip()

            length = _parse_length(length_str)
            if length is not None:
                memories[name.upper()] = length

    return memories


def detect_memory_sizes() -> Dict[str, int]:
    """
    Retorna { 'flash': bytes, 'ram': bytes } tentando:
      1) Ler linker scripts no projeto/build
      2) Usar heurísticas para nomes (FLASH, ROM, RAM, SRAM)
      3) Fallback para valores conhecidos (RP2350)
    """
    # 1) procurar linker scripts
    scripts = find_linker_scripts()
    memories = {}
    for s in scripts:
        parsed = parse_memory_from_ld(s)
        if parsed:
            memories.update(parsed)

    # heurística de seleção
    flash_candidates = ["FLASH", "ROM", "FLASH0", "QT_FLASH"]
    ram_candidates = ["RAM", "SRAM", "RAM0", "SRAM0"]

    flash = None
    ram = None
    for k, v in memories.items():
        if not flash and k in flash_candidates:
            flash = v
        if not ram and k in ram_candidates:
            ram = v

    # se não encontrou por nomes exatos, pega o maior como flash e maior restante como ram
    if (not flash or not ram) and memories:
        sorted_mem = sorted(memories.items(), key=lambda it: it[1], reverse=True)
        if not flash:
            flash = sorted_mem[0][1]
        if not ram:
            # pega o primeiro que pareça memória volátil (não a maior, se houver distinção)
            for _, size in sorted_mem[1:]:
                if size <= flash:
                    ram = size
                    break
            if not ram and len(sorted_mem) > 1:
                ram = sorted_mem[1][1]

    # Fallbacks conhecidos (RP2350):
    if not flash:
        flash = 2 * 1024 * 1024
        print(f"caiu em fallback de flash, usando: {flash}")
    if not ram:
        ram = 512 * 1024
        print(f"caiu em fallback de ram, usando: {ram}")

    flash_available = flash - filesystem_size - EEPROM_reserve

    return {"flash": flash, "flash_sketch": flash_available, "ram": ram}


def format_bytes(n):
    for unit in ("B", "KiB", "MiB"):
        if n < 1024.0:
            return f"{n:.0f} {unit}"
        n /= 1024.0
    return f"{n:.1f} MiB"


def parse_memory_output(output):
    program_size = 0
    data_size = 0

    prog_sections = [".text", ".data", ".rodata", ".text.align", ".ARM.exidx"]
    data_sections = [".data", ".bss", ".noinit"]

    # regex: procura o nome da seção em qualquer lugar da linha e captura o primeiro número após ela
    prog_re = re.compile(r"(?P<section>" + "|".join(re.escape(s) for s in prog_sections) + r")\b.*?(\d+)")
    data_re = re.compile(r"(?P<section>" + "|".join(re.escape(s) for s in data_sections) + r")\b.*?(\d+)")

    for line in output.splitlines():
        line = line.strip()
        if not line:
            continue

        m = prog_re.search(line)
        if m:
            try:
                program_size += int(m.group(2))
            except (ValueError, IndexError):
                pass
        m2 = data_re.search(line)
        if m2:
            try:
                data_size += int(m2.group(2))
            except (ValueError, IndexError):
                pass

    return {"program_size": program_size, "data_size": data_size}


def get_memory_usage():
    try:
        size_bin = shutil.which("arm-none-eabi-size") or "arm-none-eabi-size"

        # Usa caminho relativo build/H_SHIFTER_JAPATECH.elf
        elf_path = "H_SHIFTER_JAPATECH.elf"
        print(os.path.join(os.getcwd(), elf_path))

        # Tentar saída detalhada (todas as seções). Se falhar, fallback para formato simples.
        try:
            output = subprocess.check_output(
                [size_bin, "-A", "-d", elf_path],
                universal_newlines=True,
                stderr=subprocess.STDOUT,
            )
        except (subprocess.CalledProcessError, OSError):
            # fallback: tentar sem flags
            try:
                output = subprocess.check_output([size_bin, elf_path], universal_newlines=True, stderr=subprocess.STDOUT)
            except (subprocess.CalledProcessError, OSError) as e:
                print(f"Error while running size tool: {e}")
                return None

        mem_used = parse_memory_output(output)
        mem_limits = detect_memory_sizes()

        program_used = mem_used.get("program_size", 0)
        data_used = mem_used.get("data_size", 0)
        flash_total = mem_limits["flash"]
        flash_sketch = mem_limits["flash_sketch"]
        ram_total = mem_limits["ram"]

        result = {
            "program_used": program_used,
            "data_used": data_used,
            "flash_total": flash_total,
            "flash_sketch": flash_sketch,
            "ram_total": ram_total,
            "flash_used": program_used,
            "ram_used": data_used,
        }
        return result
    except (subprocess.CalledProcessError, OSError) as e:
        print(f"Error while getting memory usage: {e}")
        return None


def _format_available_bytes(value, total):
    percent_raw = float(value) / float(total)
    blocks_per_progress = 10
    used_blocks = min(int(round(blocks_per_progress * percent_raw)), blocks_per_progress)
    progress_bar = "=" * used_blocks
    return f"[{progress_bar:{blocks_per_progress}}] {percent_raw: 6.1%} (used {value:d} bytes from {total:d} bytes)"


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


if __name__ == "__main__":
    memory_info_json = get_memory_usage()
    print(memory_info_json)
    print_memory_usage(memory_info_json)
