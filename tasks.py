"""
This requires invoke to be installed on the machine (not venv), which can be
done via:
    pipx install invoke
"""
from datetime import datetime
from hashlib import md5
from pathlib import Path
from shutil import rmtree as shutil_rmtree
from typing import Optional

from invoke import task

PROJECT: str = "rengpu"
SRC_PATH: Path = Path(__file__).parent
WORKSPACE: Path = SRC_PATH / ".cache"
MD5: Optional[str] = None
BUILD_PATH: Optional[Path] = None
INSTALL_PATH: Optional[Path] = None

def get_md5(content: str) -> str:
    global MD5
    if MD5 is None:
        MD5 = md5(str.encode(content)).hexdigest()
    return MD5

def get_cmake_workspace() -> Path:
    return WORKSPACE / f"{PROJECT}"

def get_build_path() -> Path:
    global BUILD_PATH
    if BUILD_PATH is None:
        BUILD_PATH = get_cmake_workspace() / "build"
    return BUILD_PATH

def get_install_path() -> Path:
    global INSTALL_PATH
    if INSTALL_PATH is None:
        INSTALL_PATH = get_cmake_workspace() / "install"
    return INSTALL_PATH

def get_web_build_path() -> Path:
    return get_cmake_workspace() / "build-web"

@task
def info(c, topic="all"):
    """Show project info."""
    if topic == "all":
        print(f"Project         = {PROJECT}")
        print(f"Source path     = {SRC_PATH}")
        print(f"Build path      = {get_build_path()}")
        print(f"Web Build path  = {get_web_build_path()}")
        print(f"Install path    = {get_install_path()}")
    elif topic == "build_path":
        print(get_build_path())
    elif topic == "install_path":
        print(get_install_path())
    else:
        print("Error: Valid 'topic' names are 'build_path'/'install_path'")

@task
def config(c):
    """Run CMake configure for native build."""
    build_path = get_build_path()
    build_path.mkdir(parents=True, exist_ok=True)
    cmd = [
        "cmake",
        "-S", str(SRC_PATH),
        "-B", str(build_path),
        "-GNinja",
        "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=1",
        "-DGLFW_BUILD_WAYLAND=OFF",
    ]
    c.run(" ".join(cmd), pty=True)

    # Symlink compile_commands.json
    src_ccdb_file = SRC_PATH / "compile_commands.json"
    build_ccdb_file = build_path / "compile_commands.json"
    if build_ccdb_file.exists() and not src_ccdb_file.exists():
        src_ccdb_file.symlink_to(build_ccdb_file)

@task
def build(c, config=False):
    """Run build for native application via cmake."""
    build_path = get_build_path()

    if not build_path.exists():
        if config:
            do_config(c)
        else:
            print("Error: build path doesn't exist.")
            return

    cmd = ["cmake", "--build", str(build_path)]
    c.run(" ".join(cmd), pty=True)

@task
def install(c):
    """Run install via cmake."""
    build_path = get_build_path()
    install_path = get_install_path()

    if not build_path.exists():
        print("Error: build path doesn't exist.")
        return

    cmd = ["cmake", "--install", str(build_path)]
    c.run(" ".join(cmd), env={"DESTDIR": str(install_path)}, pty=True)

@task
def run(c):
    """Run the installed native binary.
    If the native build is not configured, it will run config automatically.
    """
    build_path = get_build_path()
    if not build_path.exists():
        print("Native build path doesn't exist. Running 'invoke config'...")
        config(c)

    build(c)

    install(c)

    binary_path = get_install_path() / "usr/local/bin/app"
    if not binary_path.exists():
        print("Error: installed binary not found. Check your build configuration.")
        return

    print(f"{PROJECT}: running on {binary_path}")
    print(" ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ \n")
    c.run(str(binary_path), pty=True)

@task
def clean(c):
    """Clean native build directory."""
    ccdb_file = SRC_PATH.joinpath("compile_commands.json")
    if ccdb_file.exists():
        ccdb_file.unlink()
        print(f"Removed {ccdb_file}")
    else:
        print("compile_commands.json absent. Nothing to do.")

    build_path = get_build_path()
    if build_path.exists():
        shutil_rmtree(build_path)
        print(f"Cleaned {build_path}")
    else:
        print("Build path absent. Nothing to do.")

@task(pre=[clean])
def clean_all(c):
    """Clean native build and install directories."""
    install_path = get_install_path()
    if install_path.exists():
        shutil_rmtree(install_path)
        print(f"Cleaned {install_path}")
    else:
        print("Install path absent. Nothing to do.")

@task
def ls(c):
    """List files using lsd and skip .cache folder."""
    cmd = [
        "lsd",
        "--tree",
        "--ignore-glob", ".cache",
    ]
    c.run(" ".join(cmd), pty=True)

#
# Emscripten/Web Build Tasks
#

@task
def clean_web(c):
    """Clean web build directory."""
    web_build_path = get_web_build_path()
    if web_build_path.exists():
        shutil_rmtree(web_build_path)
        print(f"Cleaned web build directory: {web_build_path}")
    else:
        print("Web build path absent. Nothing to do.")

@task
def config_web(c):
    """Configure CMake for Emscripten (WebAssembly) build."""
    web_build_path = get_web_build_path()
    web_build_path.mkdir(parents=True, exist_ok=True)
    cmd = [
        "emcmake", "cmake",
        "-S", str(SRC_PATH),
        "-B", str(web_build_path),
        "-GNinja",
        "-DCMAKE_BUILD_TYPE=RelWithDebInfo",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=1",
        "-DGLFW_BUILD_WAYLAND=OFF",
    ]
    c.run(" ".join(cmd), pty=True)

@task
def build_web(c, config=False):
    """Build the project for the Web (WASM) via CMake."""
    web_build_path = get_web_build_path()
    if not web_build_path.exists():
        if config:
            config_web(c)
        else:
            print("Error: web build path doesn't exist. Run 'invoke config_web' first.")
            return
    cmd = ["cmake", "--build", str(web_build_path)]
    c.run(" ".join(cmd), pty=True)

@task
def run_web(c):
    """Run the web build locally using Python's HTTP server.
    If the web build is not configured, it will run config_web automatically.
    """
    web_build_path = get_web_build_path()
    if not web_build_path.exists():
        print("Web build path doesn't exist. Running 'invoke config_web'...")
        config_web(c)

    # Build the web build
    build_web(c)

    html_file = web_build_path / "index.html"
    if not html_file.exists():
        print("app.html not found in the web build. Reconfiguring...")
        config_web(c)
        build_web(c)
        if not html_file.exists():
            print("Error: app.html still not found. Please check your web build configuration.")
            return

    print(f"Starting local server in: {web_build_path}")
    print("Open your browser at http://localhost:8000/")
    c.run(f"python -m http.server -d {web_build_path}", pty=True)
