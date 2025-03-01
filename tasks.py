"""
This requires invoke to be installed on the machine (not venv), which can be
done via:
    pipx install invoke
"""

from pathlib import Path
from shutil import rmtree as shutil_rmtree
from invoke import task

PROJECT: str = "rengpu"
SRC_PATH: Path = Path(__file__).parent

# Define workspace directories.
WORKSPACE: Path = SRC_PATH / ".cache"

def get_cmake_workspace() -> Path:
    return WORKSPACE / f"{PROJECT}"

def get_build_path(preset: str = "default") -> Path:
    """
    Returns the build directory based on the preset.
    Default preset uses "build" and the emscripten preset uses "build-web".
    """
    if preset == "emscripten":
        return get_cmake_workspace() / "build-web"
    else:
        return get_cmake_workspace() / "build"

def get_install_path() -> Path:
    """
    Returns the install directory.
    This should match the CMAKE_INSTALL_PREFIX set in your CMakePresets.json.
    """
    return get_cmake_workspace() / "install"

@task
def info(c, preset="default"):
    """Show project info using the specified CMake preset."""
    print(f"Project         = {PROJECT}")
    print(f"Source path     = {SRC_PATH}")
    print(f"Using CMake preset: {preset}")
    print("Ensure your CMakePresets.json is properly configured.")

@task
def config(c, preset="default"):
    """Configure the project using CMakePresets and link compile_commands.json."""
    build_path = get_build_path(preset)
    build_path.mkdir(parents=True, exist_ok=True)
    cmd = f"cmake --preset {preset}"
    print(f"Running: {cmd}")
    c.run(cmd, pty=True)

    # Symlink compile_commands.json if it exists in the build directory
    src_ccdb_file = SRC_PATH / "compile_commands.json"
    build_ccdb_file = build_path / "compile_commands.json"
    if build_ccdb_file.exists() and not src_ccdb_file.exists():
        print(f"Linking {build_ccdb_file} to {src_ccdb_file}")
        src_ccdb_file.symlink_to(build_ccdb_file)

@task
def build(c, preset="default"):
    """Build the project using CMakePresets."""
    cmd = f"cmake --build --preset {preset}"
    print(f"Running: {cmd}")
    c.run(cmd, pty=True)

@task
def install(c, preset="default"):
    """
    Install the project using the build directory from CMakePresets.
    
    Since the CMAKE_INSTALL_PREFIX is already set in the preset,
    we do not use DESTDIR.
    """
    build_path = get_build_path(preset)
    cmd = f"cmake --install {build_path}"
    print(f"Running: {cmd}")
    c.run(cmd, pty=True)

@task
def run(c, preset="default"):
    """
    Run the installed native binary.
    
    This task will configure, build, and install using the specified preset.
    The installed binary is expected to be at:
        <install_path>/bin/app
    """
    config(c, preset=preset)
    build(c, preset=preset)
    install(c, preset=preset)

    install_dir = get_install_path()
    binary_path = install_dir / "bin/app"
    if not binary_path.exists():
        print("Error: installed binary not found. Check your build configuration.")
        return

    print(f"{PROJECT}: running binary at {binary_path}")
    c.run(str(binary_path), pty=True)

@task
def clean(c):
    """Clean build and install directories."""
    ccdb_file = SRC_PATH / "compile_commands.json"
    if ccdb_file.exists():
        ccdb_file.unlink()
        print(f"Removed {ccdb_file}")
    else:
        print("compile_commands.json absent. Nothing to do.")

    for subdir in ["build", "build-web", "install"]:
        path = get_cmake_workspace() / subdir
        if path.exists():
            shutil_rmtree(path)
            print(f"Cleaned {path}")
        else:
            print(f"{path} absent. Nothing to do.")

#
# Emscripten/Web Build Tasks using a separate preset (e.g., "emscripten")
#

@task
def clean_web(c):
    """Clean web build directory."""
    web_build_path = get_build_path("emscripten")
    if web_build_path.exists():
        shutil_rmtree(web_build_path)
        print(f"Cleaned web build directory: {web_build_path}")
    else:
        print("Web build path absent. Nothing to do.")

@task
def config_web(c, preset="emscripten"):
    """Configure CMake for Emscripten (WebAssembly) build using CMakePresets."""
    # Here we assume you use emcmake to set up the environment for Emscripten.
    cmd = f"emcmake cmake --preset {preset}"
    print(f"Running: {cmd}")
    c.run(cmd, pty=True)
    
    # Link compile_commands.json for the web build as well.
    web_build_path = get_build_path(preset)
    src_ccdb_file = SRC_PATH / "compile_commands.json"
    build_ccdb_file = web_build_path / "compile_commands.json"
    if build_ccdb_file.exists() and not src_ccdb_file.exists():
        print(f"Linking {build_ccdb_file} to {src_ccdb_file}")
        src_ccdb_file.symlink_to(build_ccdb_file)

@task
def build_web(c, preset="emscripten"):
    """Build the project for the Web (WASM) via CMakePresets."""
    cmd = f"cmake --build --preset {preset}"
    print(f"Running: {cmd}")
    c.run(cmd, pty=True)

@task
def run_web(c, preset="emscripten"):
    """
    Run the web build locally using Python's HTTP server.
    
    This task will configure and build the web target using the specified preset.
    """
    config_web(c, preset=preset)
    build_web(c, preset=preset)

    web_build_path = get_build_path(preset)
    html_file = web_build_path / "index.html"
    if not html_file.exists():
        print("index.html not found in the web build. Please check your web build configuration.")
        return

    print(f"Starting local server in: {html_file.parent}")
    print("Open your browser at http://localhost:8000/")
    c.run(f"python -m http.server -d {html_file.parent}", pty=True)

