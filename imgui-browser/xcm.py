#!/usr/bin/env python3
"""
xcm.py -- cross-platform launcher / dev CLI for imgui-browser.

Usage
-----
  python xcm.py list                         List available profiles
  python xcm.py run <profile> [options]      Build-if-needed then launch
  python xcm.py dry-run <profile> [options]  Print what would be launched
  python xcm.py build [--clean]              Build the native app only
  python xcm.py serve-wasm [options]         Start only the PHP-WASM server
  python xcm.py stop-wasm                    Stop a background WASM server

Profile options (any subcommand that accepts a profile)
  --url <url>           Override startup_url for this run
  --project <path>      PHP project root to serve via WASM
  --port <n>            Override wasm_port for this run
  --width  <n>          Override window width
  --height <n>          Override window height

Examples
  python xcm.py run grab_bar
  python xcm.py run explore --url https://example.com
  python xcm.py run wasm --project /path/to/my-php-app
  python xcm.py dry-run wasm
  python xcm.py serve-wasm --project ../  --port 8082
  python xcm.py build
  python xcm.py build --clean

Cross-platform notes
  macOS   Binary: build/imgui_browser.app/Contents/MacOS/imgui_browser
            or:   build/imgui_browser
  Linux   Binary: build/imgui_browser
  Windows Binary: build\\Release\\imgui_browser.exe  or  build\\imgui_browser.exe
          Build:  cmake --build must be run from a Developer Command Prompt;
                  xcm.py will attempt to locate cmake automatically.

PHP-WASM server (no native PHP required)
  Node.js must be installed.  xcm.py starts `node server.js` inside
  php-wasm-project/.  If --project is given, the project files are
  copied into php-wasm-project/public/ before starting the server.
  The wasm server PID is written to .wasm-server.pid so it can be
  stopped with `python xcm.py stop-wasm`.

MIT License  --  Crissy's Style Tool / XcaliburMoon Web Development
"""

import sys
import os
import json
import subprocess
import shutil
import platform
import time
import signal
import argparse
import textwrap

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------
SCRIPT_DIR   = os.path.dirname(os.path.abspath(__file__))
SETTINGS_JSON = os.path.join(SCRIPT_DIR, "settings.json")
BUILD_DIR     = os.path.join(SCRIPT_DIR, "build")
SRC_DIR       = os.path.join(SCRIPT_DIR, "src")
PHP_WASM_DIR  = os.path.join(SCRIPT_DIR, "php-wasm-project")
PID_FILE      = os.path.join(SCRIPT_DIR, ".wasm-server.pid")
VENDOR_DIR    = os.path.join(SCRIPT_DIR, "vendor")

OS = platform.system()  # "Darwin" | "Linux" | "Windows"

# ---------------------------------------------------------------------------
# Terminal colour helpers (disabled on Windows unless FORCE_COLOR is set)
# ---------------------------------------------------------------------------
def _supports_color() -> bool:
    if os.environ.get("FORCE_COLOR"):
        return True
    if OS == "Windows" and not os.environ.get("WT_SESSION"):
        return False
    return hasattr(sys.stdout, "isatty") and sys.stdout.isatty()

USE_COLOR = _supports_color()

def _c(code: str, text: str) -> str:
    if not USE_COLOR:
        return text
    return f"\033[{code}m{text}\033[0m"

def cyan(t):    return _c("36",    t)
def green(t):   return _c("32",    t)
def yellow(t):  return _c("33",    t)
def red(t):     return _c("31",    t)
def bold(t):    return _c("1",     t)
def dim(t):     return _c("2",     t)

def info(msg:  str): print(green( "[xcm] ") + msg)
def warn(msg:  str): print(yellow("[xcm] ") + msg)
def err(msg:   str): print(red(   "[xcm] ") + msg, file=sys.stderr)
def step(msg:  str): print(cyan(  "  ->  ") + msg)
def note(msg:  str): print(dim(   "       " + msg))

# ---------------------------------------------------------------------------
# settings.json loader
# ---------------------------------------------------------------------------
def load_settings() -> dict:
    if not os.path.isfile(SETTINGS_JSON):
        err(f"settings.json not found at {SETTINGS_JSON}")
        sys.exit(1)
    with open(SETTINGS_JSON, "r", encoding="utf-8") as fh:
        return json.load(fh)

def get_profile(settings: dict, name: str) -> dict:
    profiles = settings.get("profiles", {})
    if name not in profiles:
        err(f"Unknown profile '{name}'. Run `python xcm.py list` to see available profiles.")
        sys.exit(1)
    defaults = settings.get("defaults", {})
    profile  = dict(defaults)
    profile.update(profiles[name])
    return profile

# ---------------------------------------------------------------------------
# Binary detection
# ---------------------------------------------------------------------------
def find_binary() -> str | None:
    """Return path to the compiled binary, or None if not built yet."""
    candidates: list[str] = []
    if OS == "Darwin":
        candidates = [
            os.path.join(BUILD_DIR, "imgui_browser.app", "Contents", "MacOS", "imgui_browser"),
            os.path.join(BUILD_DIR, "imgui_browser"),
        ]
    elif OS == "Linux":
        candidates = [
            os.path.join(BUILD_DIR, "imgui_browser"),
        ]
    else:  # Windows
        candidates = [
            os.path.join(BUILD_DIR, "Release", "imgui_browser.exe"),
            os.path.join(BUILD_DIR, "imgui_browser.exe"),
            os.path.join(BUILD_DIR, "imgui_browser"),
        ]
    for c in candidates:
        if os.path.isfile(c):
            return c
    return None

# ---------------------------------------------------------------------------
# Arg builder: profile -> list of str CLI args for the binary
# ---------------------------------------------------------------------------
def build_argv(profile: dict, overrides: dict) -> list[str]:
    """Merge profile with CLI overrides and produce the binary's argv list."""
    p = dict(profile)
    p.update({k: v for k, v in overrides.items() if v is not None})

    argv: list[str] = []

    # UI mode
    ui_mode = p.get("ui_mode", "full")
    argv += ["--ui-mode", str(ui_mode)]

    # Startup URL
    url = p.get("startup_url") or p.get("url") or ""
    if url and url != "about:blank":
        argv += ["--url", url]

    # Window size
    if p.get("win_w"):
        argv += ["--width",  str(p["win_w"])]
    if p.get("win_h"):
        argv += ["--height", str(p["win_h"])]

    # WASM
    wasm_enabled = bool(p.get("wasm_enabled", False))
    if wasm_enabled:
        argv.append("--wasm")
        wasm_port = int(p.get("wasm_port", 8082))
        argv += ["--wasm-port", str(wasm_port)]
        wasm_dir = str(p.get("wasm_dir") or "")
        if wasm_dir:
            argv += ["--wasm-dir", wasm_dir]
    else:
        argv.append("--no-wasm")

    # PHP + cmd ports
    if p.get("php_port"):
        argv += ["--php-port", str(p["php_port"])]
    if p.get("cmd_port"):
        argv += ["--cmd-port", str(p["cmd_port"])]

    return argv

# ---------------------------------------------------------------------------
# Node.js detection
# ---------------------------------------------------------------------------
def find_node() -> str | None:
    for candidate in ["node", "nodejs"]:
        found = shutil.which(candidate)
        if found:
            return found
    return None

# ---------------------------------------------------------------------------
# WASM server management
# ---------------------------------------------------------------------------
def wasm_server_is_running() -> bool:
    if not os.path.isfile(PID_FILE):
        return False
    try:
        pid = int(open(PID_FILE).read().strip())
        if OS == "Windows":
            import ctypes
            handle = ctypes.windll.kernel32.OpenProcess(0x100000, False, pid)
            if handle == 0:
                return False
            ctypes.windll.kernel32.CloseHandle(handle)
            return True
        else:
            os.kill(pid, 0)
            return True
    except (ValueError, OSError, FileNotFoundError):
        return False

def start_wasm_server(port: int = 8082,
                      project_path: str | None = None,
                      dry_run: bool = False) -> int | None:
    """
    Start `node server.js` inside php-wasm-project.
    If project_path is given, its contents are synced into php-wasm-project/public
    before starting.
    Returns the PID (int) or None on dry-run / failure.
    """
    node = find_node()
    if not node:
        err("Node.js not found.  Install Node.js (https://nodejs.org) to use the PHP-WASM server.")
        return None

    server_js = os.path.join(PHP_WASM_DIR, "server.js")
    if not os.path.isfile(server_js):
        err(f"php-wasm-project/server.js not found at {server_js}")
        err("Run `make` inside php-wasm-project/ to build the WASM binary first.")
        return None

    # Copy project files into public/
    if project_path:
        src = os.path.abspath(project_path)
        dst = os.path.join(PHP_WASM_DIR, "public")
        if not os.path.isdir(src):
            err(f"Project path does not exist: {src}")
            return None
        if dry_run:
            note(f"Would sync: {src}")
            note(f"      into: {dst}")
        else:
            info(f"Syncing project into {dst} ...")
            if os.path.isdir(dst):
                shutil.rmtree(dst)
            shutil.copytree(src, dst, dirs_exist_ok=True)
            info("Sync done.")

    env = os.environ.copy()
    env["PORT"] = str(port)

    step(f"node server.js  (PORT={port})")
    if dry_run:
        note(f"cwd: {PHP_WASM_DIR}")
        note(f"env PORT={port}")
        return None

    if OS == "Windows":
        proc = subprocess.Popen(
            [node, server_js],
            cwd=PHP_WASM_DIR,
            env=env,
            creationflags=subprocess.CREATE_NEW_PROCESS_GROUP,
        )
    else:
        proc = subprocess.Popen(
            [node, server_js],
            cwd=PHP_WASM_DIR,
            env=env,
            start_new_session=True,
        )

    with open(PID_FILE, "w") as fh:
        fh.write(str(proc.pid))

    # Wait briefly until the port is open (up to 12 s)
    info(f"Waiting for WASM server on port {port} ...")
    ready = False
    for _ in range(24):
        time.sleep(0.5)
        if _port_open("127.0.0.1", port):
            ready = True
            break

    if ready:
        info(f"PHP-WASM server ready at http://localhost:{port}  (pid {proc.pid})")
    else:
        warn(f"WASM server pid={proc.pid} started but port {port} not yet open after 12 s")

    return proc.pid

def stop_wasm_server() -> None:
    if not os.path.isfile(PID_FILE):
        info("No .wasm-server.pid file found -- nothing to stop.")
        return
    try:
        pid = int(open(PID_FILE).read().strip())
    except ValueError:
        os.remove(PID_FILE)
        return

    try:
        if OS == "Windows":
            subprocess.call(["taskkill", "/F", "/T", "/PID", str(pid)],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        else:
            os.killpg(os.getpgid(pid), signal.SIGTERM)
        info(f"Stopped WASM server (pid {pid}).")
    except (ProcessLookupError, OSError) as exc:
        warn(f"Could not kill pid {pid}: {exc}")
    finally:
        if os.path.isfile(PID_FILE):
            os.remove(PID_FILE)

def _port_open(host: str, port: int) -> bool:
    import socket
    try:
        with socket.create_connection((host, port), timeout=0.3):
            return True
    except OSError:
        return False


# ---------------------------------------------------------------------------
# Dependency bootstrap
# ---------------------------------------------------------------------------
def _git_clone_if_missing(name: str, url: str, tag: str) -> bool:
    dest = os.path.join(VENDOR_DIR, name)
    if os.path.isdir(dest):
        return True

    git = shutil.which("git")
    if not git:
        err(f"git not found; cannot fetch missing dependency: {name}")
        return False

    os.makedirs(VENDOR_DIR, exist_ok=True)
    info(f"Fetching dependency: {name} ({tag})")
    cmd = [git, "clone", "--branch", tag, "--depth", "1", url, dest]
    result = subprocess.run(cmd)
    if result.returncode != 0:
        err(f"Failed to clone dependency: {name}")
        return False
    return True


def _find_webview2_sdk() -> tuple[str | None, str | None]:
    """
    Return (include_dir, static_lib_path) for WebView2 SDK, or (None, None).
    Supports both vendor/webview2/include layout and NuGet package layout.
    """
    candidates = [
        (
            os.path.join(VENDOR_DIR, "webview2", "include"),
            os.path.join(VENDOR_DIR, "webview2", "lib", "x64", "WebView2LoaderStatic.lib"),
        ),
    ]

    webview2_root = os.path.join(VENDOR_DIR, "webview2")
    if os.path.isdir(webview2_root):
        for entry in os.listdir(webview2_root):
            if not entry.lower().startswith("microsoft.web.webview2"):
                continue
            pkg_root = os.path.join(webview2_root, entry)
            candidates.append(
                (
                    os.path.join(pkg_root, "build", "native", "include"),
                    os.path.join(pkg_root, "build", "native", "x64", "WebView2LoaderStatic.lib"),
                )
            )

    for include_dir, lib_path in candidates:
        if os.path.isfile(os.path.join(include_dir, "WebView2.h")) and os.path.isfile(lib_path):
            return include_dir, lib_path

    return None, None


def _install_webview2_sdk() -> bool:
    include_dir, lib_path = _find_webview2_sdk()
    if include_dir and lib_path:
        return True

    nuget = shutil.which("nuget")
    if not nuget:
        warn("nuget not found; cannot auto-install WebView2 SDK. Build will use stubs unless SDK is installed.")
        warn("Install manually: nuget install Microsoft.Web.WebView2 -OutputDirectory vendor/webview2")
        return False

    os.makedirs(os.path.join(VENDOR_DIR, "webview2"), exist_ok=True)
    info("Installing WebView2 SDK via NuGet ...")
    cmd = [
        nuget,
        "install",
        "Microsoft.Web.WebView2",
        "-OutputDirectory",
        os.path.join(VENDOR_DIR, "webview2"),
        "-ExcludeVersion",
    ]
    result = subprocess.run(cmd, cwd=SCRIPT_DIR)
    if result.returncode != 0:
        warn("NuGet WebView2 install failed; continuing (stub webview build may still compile).")
        return False

    include_dir, lib_path = _find_webview2_sdk()
    return bool(include_dir and lib_path)


def ensure_native_deps() -> tuple[bool, list[str]]:
    """
    Ensure native source dependencies are present.
    Returns (ok, cmake_extra_args).
    """
    deps = [
        ("imgui", "https://github.com/ocornut/imgui.git", "v1.91.5"),
        ("glfw", "https://github.com/glfw/glfw.git", "3.4"),
        ("httplib", "https://github.com/yhirose/cpp-httplib.git", "v0.18.1"),
    ]

    ok = True
    for name, url, tag in deps:
        ok = _git_clone_if_missing(name, url, tag) and ok

    cmake_args: list[str] = []
    if OS == "Windows":
        if _install_webview2_sdk():
            include_dir, lib_path = _find_webview2_sdk()
            if include_dir and lib_path:
                cmake_args.extend([
                    f"-DWEBVIEW2_INCLUDE_DIR={include_dir}",
                    f"-DWEBVIEW2_LOADER_LIB={lib_path}",
                ])
        else:
            warn("Proceeding without WebView2 SDK auto-detection; browser tab rendering may be disabled.")
    elif OS == "Linux":
        pkg_config = shutil.which("pkg-config")
        if not pkg_config:
            err("pkg-config is required on Linux (install package: pkg-config).")
            return False, cmake_args

        check = subprocess.run([pkg_config, "--exists", "webkit2gtk-4.1"], check=False)
        if check.returncode != 0:
            err("webkit2gtk-4.1 development files are missing.")
            err("Install package: libwebkit2gtk-4.1-dev")
            return False, cmake_args

    return ok, cmake_args

# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------
def build_app(clean: bool = False) -> bool:
    """Run the platform-appropriate build.  Returns True on success."""
    cmake = shutil.which("cmake")
    if not cmake:
        err("cmake not found.  Install CMake (https://cmake.org).")
        return False

    deps_ok, cmake_dep_args = ensure_native_deps()
    if not deps_ok:
        err("One or more native dependencies failed to install.")
        return False

    if clean and os.path.isdir(BUILD_DIR):
        info(f"Removing {BUILD_DIR} ...")
        shutil.rmtree(BUILD_DIR)

    os.makedirs(BUILD_DIR, exist_ok=True)

    # Configure
    configure_cmd = [
        cmake, "..",
        "-DCMAKE_BUILD_TYPE=Release",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]
    configure_cmd += cmake_dep_args

    if OS == "Darwin":
        import subprocess as sp
        arch = sp.check_output(["uname", "-m"], text=True).strip()
        configure_cmd += [
            f"-DCMAKE_OSX_ARCHITECTURES={arch}",
            "-DCMAKE_OSX_DEPLOYMENT_TARGET=12.0",
        ]
    elif OS == "Windows":
        configure_cmd.append("-DCMAKE_BUILD_TYPE=Release")

    info("Configuring ...")
    step(" ".join(configure_cmd))
    result = subprocess.run(configure_cmd, cwd=BUILD_DIR)
    if result.returncode != 0:
        err("cmake configure failed.")
        return False

    # Determine parallel jobs
    import multiprocessing
    jobs = max(1, multiprocessing.cpu_count())
    build_cmd = [cmake, "--build", ".", "--config", "Release", "-j", str(jobs)]

    info("Building ...")
    step(" ".join(build_cmd))
    result = subprocess.run(build_cmd, cwd=BUILD_DIR)
    if result.returncode != 0:
        err("cmake build failed.")
        return False

    # macOS ad-hoc sign
    if OS == "Darwin":
        app_bundle = os.path.join(BUILD_DIR, "imgui_browser.app")
        if os.path.isdir(app_bundle):
            info("Ad-hoc signing the app bundle ...")
            subprocess.run(["codesign", "--force", "--deep", "--sign", "-", app_bundle],
                           check=False)
            subprocess.run(["xattr", "-rd", "com.apple.quarantine", app_bundle],
                           check=False, capture_output=True)

    info("Build complete.")
    return True

# ---------------------------------------------------------------------------
# Launch
# ---------------------------------------------------------------------------
def launch(binary: str, extra_argv: list[str], dry_run: bool = False) -> None:
    cmd = [binary] + extra_argv
    info("Command:")
    print("   " + bold(binary))
    for i in range(0, len(extra_argv), 2):
        chunk = extra_argv[i:i+2]
        print("   " + "  ".join(cyan(c) for c in chunk))
    if dry_run:
        note("(dry-run: binary not launched)")
        return
    os.execv(binary, cmd)     # replaces this process; never returns on success

# ---------------------------------------------------------------------------
# Subcommand: list
# ---------------------------------------------------------------------------
def cmd_list(settings: dict) -> None:
    profiles = settings.get("profiles", {})
    print()
    print(bold("Available profiles"))
    print()
    max_len = max((len(k) for k in profiles), default=10)
    for name, data in profiles.items():
        desc    = data.get("_desc", "")
        dry_only = data.get("_dry_run_only", False)
        tag = dim(" [dry-run only]") if dry_only else ""
        print(f"  {cyan(name.ljust(max_len+2))}  {desc}{tag}")
    print()
    print(f"  Run a profile:     {bold('python xcm.py run <profile>')}")
    print(f"  Dry-run check:     {bold('python xcm.py dry-run <profile>')}")
    print()

# ---------------------------------------------------------------------------
# Subcommand: run / dry-run
# ---------------------------------------------------------------------------
def cmd_run(settings: dict,
            profile_name: str,
            overrides: dict,
            dry_run: bool = False) -> None:

    profile = get_profile(settings, profile_name)

    if profile.get("_dry_run_only") and not dry_run:
        warn(f"Profile '{profile_name}' is marked _dry_run_only.  Forcing dry-run.")
        dry_run = True

    print()
    info(f"Profile: {bold(profile_name)}" + (" (dry-run)" if dry_run else ""))
    note(profile.get("_desc", ""))
    print()

    # Merge --url / --port / --width / --height overrides
    merged_overrides: dict = {}
    if overrides.get("url"):
        merged_overrides["startup_url"] = overrides["url"]
        merged_overrides["url"]         = overrides["url"]
    if overrides.get("port"):
        merged_overrides["wasm_port"] = overrides["port"]
    if overrides.get("width"):
        merged_overrides["win_w"] = overrides["width"]
    if overrides.get("height"):
        merged_overrides["win_h"] = overrides["height"]
    # wasm_enabled may be forced on if --project was supplied
    if overrides.get("project"):
        merged_overrides["wasm_enabled"] = True

    binary = find_binary()

    if not dry_run and not binary:
        warn("Binary not found.  Building first ...")
        if not build_app():
            err("Build failed -- cannot launch.")
            sys.exit(1)
        binary = find_binary()
        if not binary:
            err("Binary still not found after build.  Check the build output.")
            sys.exit(1)

    if dry_run and not binary:
        step(f"Binary would be: {os.path.join(BUILD_DIR, 'imgui_browser')}")
    elif binary:
        step(f"Binary: {binary}")

    # Handle WASM server
    wasm_enabled = bool(
        merged_overrides.get("wasm_enabled", profile.get("wasm_enabled", False))
    )
    wasm_port = int(merged_overrides.get("wasm_port", profile.get("wasm_port", 8082)))

    if wasm_enabled:
        if wasm_server_is_running():
            info(f"PHP-WASM server already running on port {wasm_port}.")
        else:
            pid = start_wasm_server(
                port         = wasm_port,
                project_path = overrides.get("project"),
                dry_run      = dry_run,
            )
            if pid is None and not dry_run:
                err("WASM server failed to start.  Cannot continue.")
                sys.exit(1)
        # Point the startup_url at the WASM server if not overridden
        if not merged_overrides.get("startup_url"):
            merged_overrides["startup_url"] = f"http://localhost:{wasm_port}"
            merged_overrides["url"]         = merged_overrides["startup_url"]

    argv = build_argv(profile, merged_overrides)

    if not dry_run and binary:
        launch(binary, argv, dry_run=False)
    else:
        launch(binary or "<binary>", argv, dry_run=True)

# ---------------------------------------------------------------------------
# Subcommand: build
# ---------------------------------------------------------------------------
def cmd_build(clean: bool = False) -> None:
    print()
    info(f"Building imgui-browser ({OS}) ...")
    if not build_app(clean=clean):
        sys.exit(1)

# ---------------------------------------------------------------------------
# Subcommand: serve-wasm
# ---------------------------------------------------------------------------
def cmd_serve_wasm(port: int = 8082,
                   project_path: str | None = None) -> None:
    print()
    if wasm_server_is_running():
        info(f"PHP-WASM server is already running (see {PID_FILE}).")
        return
    pid = start_wasm_server(port=port, project_path=project_path)
    if pid is None:
        sys.exit(1)
    print()
    info(f"Open:  http://localhost:{port}")
    info(f"Stop:  python xcm.py stop-wasm")
    print()
    # Keep the process alive so Ctrl-C stops everything
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        print()
        info("Keyboard interrupt -- stopping WASM server ...")
        stop_wasm_server()

# ---------------------------------------------------------------------------
# Subcommand: stop-wasm
# ---------------------------------------------------------------------------
def cmd_stop_wasm() -> None:
    print()
    stop_wasm_server()

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
def build_parser() -> argparse.ArgumentParser:
    root = argparse.ArgumentParser(
        prog="xcm.py",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description=textwrap.dedent("""\
            xcm.py -- cross-platform launcher / dev CLI for imgui-browser.
            Run `python xcm.py list` to see available profiles.
        """),
    )
    sub = root.add_subparsers(dest="command", metavar="<command>")

    # list
    sub.add_parser("list", help="List available launch profiles")

    # run
    p_run = sub.add_parser("run", help="Build if needed, then launch a profile")
    p_run.add_argument("profile",          help="Profile name from settings.json")
    p_run.add_argument("--url",            help="Override startup URL")
    p_run.add_argument("--project",        help="PHP project root to serve via WASM")
    p_run.add_argument("--port",  type=int, help="Override wasm_port")
    p_run.add_argument("--width", type=int, help="Override window width")
    p_run.add_argument("--height",type=int, help="Override window height")

    # dry-run
    p_dry = sub.add_parser("dry-run", help="Print what would be launched (no side effects)")
    p_dry.add_argument("profile",          help="Profile name from settings.json")
    p_dry.add_argument("--url",            help="Override startup URL")
    p_dry.add_argument("--project",        help="PHP project root to serve via WASM")
    p_dry.add_argument("--port",  type=int, help="Override wasm_port")
    p_dry.add_argument("--width", type=int, help="Override window width")
    p_dry.add_argument("--height",type=int, help="Override window height")

    # build
    p_build = sub.add_parser("build", help="Build the native app (without launching)")
    p_build.add_argument("--clean", action="store_true",
                         help="Remove build/ directory before building")

    # serve-wasm
    p_sw = sub.add_parser("serve-wasm", help="Start only the PHP-WASM server")
    p_sw.add_argument("--project", help="PHP project root to sync into public/")
    p_sw.add_argument("--port", type=int, default=8082, help="Port (default 8082)")

    # stop-wasm
    sub.add_parser("stop-wasm", help="Stop a background PHP-WASM server")

    return root

# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
def main() -> None:
    parser = build_parser()
    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(0)

    args   = parser.parse_args()
    settings = load_settings()

    if args.command == "list":
        cmd_list(settings)

    elif args.command in ("run", "dry-run"):
        is_dry = (args.command == "dry-run")
        overrides = {
            "url":     getattr(args, "url",     None),
            "project": getattr(args, "project", None),
            "port":    getattr(args, "port",    None),
            "width":   getattr(args, "width",   None),
            "height":  getattr(args, "height",  None),
        }
        cmd_run(settings, args.profile, overrides, dry_run=is_dry)

    elif args.command == "build":
        cmd_build(clean=getattr(args, "clean", False))

    elif args.command == "serve-wasm":
        cmd_serve_wasm(
            port         = getattr(args, "port",    8082),
            project_path = getattr(args, "project", None),
        )

    elif args.command == "stop-wasm":
        cmd_stop_wasm()

    else:
        parser.print_help()
        sys.exit(1)


if __name__ == "__main__":
    main()
