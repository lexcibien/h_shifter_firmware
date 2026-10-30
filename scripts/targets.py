import json
import os
import re
import shlex
import shutil
import subprocess
from pathlib import Path

from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()


model_type = ""
version = ""

# Caminhos base
project_dir = env["PROJECT_DIR"]
print(f"[EXTRA SCRIPT] Diretório do projeto: {project_dir}")


def run_clang_format(check_only=False):
    clang_format = shutil.which("clang-format")
    if clang_format is None:
        print("[FORMAT] clang-format não encontrado no PATH")
        return 1

    extensions = ("*.c", "*.cpp", "*.h")
    files_result = subprocess.run(
        ["git", "ls-files", "-z", "--", *extensions],
        cwd=project_dir,
        check=True,
        capture_output=True,
    )
    files = [path.decode("utf-8") for path in files_result.stdout.split(b"\0") if path]
    if not files:
        print("[FORMAT] Nenhum arquivo C/C++ rastreado pelo Git foi encontrado")
        return 1

    action = "Verificando" if check_only else "Formatando"
    print(f"[FORMAT] {action} {len(files)} arquivo(s) C/C++")
    flags = ["--dry-run", "--Werror"] if check_only else ["-i"]

    original_contents = {}
    if not check_only:
        original_contents = {path: Path(project_dir, path).read_bytes() for path in files}

    result = subprocess.run([clang_format, *flags, *files], cwd=project_dir, check=False)
    if not check_only:
        changed_files = [path for path, original in original_contents.items() if Path(project_dir, path).read_bytes() != original]
        if changed_files:
            print("[FORMAT] Arquivos formatados:")
            for path in changed_files:
                print(f"  - {path}")
        elif result.returncode == 0:
            print("[FORMAT] Todos os arquivos já estavam formatados")

    if result.returncode == 0:
        if check_only:
            print("[FORMAT] Verificação concluída")
        elif changed_files:
            print(f"[FORMAT] Formatação concluída ({len(changed_files)} arquivo(s) alterado(s))")
    return result.returncode


def format_cpp_action(source, target, env):
    return run_clang_format()


def check_cpp_format_action(source, target, env):
    return run_clang_format(check_only=True)


def run_clang_tidy(platformio, environment):
    clang_tidy = shutil.which("clang-tidy")
    if clang_tidy is None:
        print("[LINT] clang-tidy não encontrado no PATH")
        return 1

    result = subprocess.run(
        [platformio, "run", "--environment", environment, "--target", "compiledb"],
        cwd=project_dir,
        check=False,
    )
    if result.returncode != 0:
        return result.returncode

    with open(Path(project_dir, "compile_commands.json"), encoding="utf-8") as database:
        entries = json.load(database)

    project_path = Path(project_dir).resolve()
    directory = project_path / "src"
    sources = sorted(
        {
            str((Path(entry["directory"]) / entry["file"]).resolve())
            for entry in entries
            if (Path(entry["directory"]) / entry["file"]).resolve().is_relative_to(directory)
        }
    )
    if not sources:
        print(f"[LINT] Nenhum fonte do projeto encontrado para {environment}")
        return 1

    compiler = shlex.split(next(entry["command"] for entry in entries if entry["file"].endswith(".cpp")))[0]
    includes_result = subprocess.run(
        [compiler, "-E", "-x", "c++", "-", "-v"],
        input="",
        text=True,
        capture_output=True,
        cwd=project_dir,
        check=False,
    )
    lines = includes_result.stderr.splitlines()
    try:
        first = lines.index("#include <...> search starts here:") + 1
        last = lines.index("End of search list.", first)
    except ValueError:
        print("[LINT] Não foi possível localizar os headers da toolchain")
        return 1
    if includes_result.returncode != 0 or first == last:
        print("[LINT] Falha ao consultar os headers da toolchain")
        return 1

    command = [
        clang_tidy,
        "--quiet",
        f"--header-filter=^{re.escape(str(project_path))}/(?:src|lib)/.*",
        f"--config-file={Path(project_dir, '.clang-tidy')}",
        f"-p={project_dir}",
        *sources,
        "--extra-arg=-Wno-unknown-warning-option",
        *(f"--extra-arg=-isystem{line.strip()}" for line in lines[first:last]),
    ]
    print(f"[LINT] clang-tidy: {len(sources)} fonte(s) de {environment}")
    return subprocess.run(command, cwd=project_dir, check=False).returncode


def lint_cpp_action(source, target, env):
    platformio = shutil.which("platformio") or shutil.which("pio")
    if platformio is None:
        print("[LINT] PlatformIO CLI não encontrado no PATH")
        return 1

    environments = ("waveshare_rp2040_zero_v1", "waveshare_rp2040_zero_v2")
    for environment in environments:
        print(f"[LINT] Analisando {environment}")
        result = subprocess.run(
            [platformio, "check", "--environment", environment, "--fail-on-defect=high"],
            cwd=project_dir,
            check=False,
        )
        if result.returncode != 0:
            print(f"[LINT] Análise falhou para {environment}")
            return result.returncode

        result_code = run_clang_tidy(platformio, environment)
        if result_code != 0:
            print(f"[LINT] clang-tidy falhou para {environment}")
            return result_code

    print("[LINT] Análise concluída para todos os ambientes")
    return 0


env.AddCustomTarget("format", None, format_cpp_action)

env.AddCustomTarget("format-check", None, check_cpp_format_action)

env.AddCustomTarget("lint", None, lint_cpp_action)
