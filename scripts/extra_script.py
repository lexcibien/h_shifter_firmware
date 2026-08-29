import os
import shutil
import subprocess

from SCons.Script import DefaultEnvironment

env = DefaultEnvironment()

print(f"[EXTRA SCRIPT] Script carregado para ambiente: {env['PIOENV']}")

# Caminhos base
project_dir = env["PROJECT_DIR"]
print(f"[EXTRA SCRIPT] Diretório do projeto: {project_dir}")

def generate_compile_commands():
    try:
        print("[POST-BUILD] Gerando compile_commands.json...")
        subprocess.run(["pio", "run", "-t", "compiledb"], cwd=project_dir, check=True)

    except (EnvironmentError, FileNotFoundError) as e:
        print(f"[POST-BUILD] Erro ao gerar compile_commands.json: {e}")


def after_build(source, target, env):
    print(f"[POST-BUILD] Função after_build chamada para ambiente: {env['PIOENV']}")

    generate_compile_commands()


# Registrar função pós build
print(f"[EXTRA SCRIPT] Registrando post-action para ambiente: {env['PIOENV']}")
env.AddPostAction("$BUILD_DIR/firmware.bin.signed", after_build)
