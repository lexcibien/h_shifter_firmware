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

    print("[LINT] Análise concluída para todos os ambientes")
    return 0


env.AddCustomTarget("format", None, format_cpp_action)

env.AddCustomTarget("format-check", None, check_cpp_format_action)

env.AddCustomTarget("lint", None, lint_cpp_action)
