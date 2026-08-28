#!/usr/bin/env python3
"""
Unreal Engine 5.8 Build Harness
Automates compilation via UnrealBuildTool (UBT), parses verbose output, and formats errors/warnings as clean JSON diagnostics.
"""

import sys
import os
import re
import json
import time
import subprocess
import argparse
from pathlib import Path

DEFAULT_ENGINE_CANDIDATES = [
    r"C:\Program Files\Epic Games\UE_5.8",
    r"C:\Program Files\Epic Games\UE_5.7",
    r"C:\Program Files\Epic Games\UE_5.6",
    r"C:\Program Files\Epic Games\UE_5.5",
]

def find_uproject_file(start_dir: Path) -> Path | None:
    current = start_dir.resolve()
    for _ in range(5):
        uprojects = list(current.glob("*.uproject"))
        if uprojects:
            return uprojects[0]
        if current.parent == current:
            break
        current = current.parent
    return None

def query_registry_engine_paths() -> list[str]:
    paths = []
    if sys.platform != "win32":
        return paths
    try:
        import winreg
        for root_key in (winreg.HKEY_LOCAL_MACHINE, winreg.HKEY_CURRENT_USER):
            try:
                with winreg.OpenKey(root_key, r"SOFTWARE\EpicGames\Unreal Engine") as key:
                    num_subkeys = winreg.QueryInfoKey(key)[0]
                    for i in range(num_subkeys):
                        subkey_name = winreg.EnumKey(key, i)
                        with winreg.OpenKey(key, subkey_name) as subkey:
                            try:
                                installed_dir, _ = winreg.QueryValueEx(subkey, "InstalledDirectory")
                                if installed_dir and os.path.exists(installed_dir):
                                    paths.append(installed_dir)
                            except OSError:
                                pass
            except OSError:
                pass
    except Exception:
        pass
    return paths

def locate_ubt(explicit_engine_dir: str | None = None) -> tuple[Path | None, Path | None]:
    search_dirs = []
    
    # 1. Explicit argument
    if explicit_engine_dir:
        search_dirs.append(Path(explicit_engine_dir))

    # 2. Environment variables
    for env_var in ["UE_5_8_PATH", "UNREAL_ENGINE_PATH", "UE_ROOT", "ENGINE_PATH", "UE_ENGINE_DIR"]:
        val = os.environ.get(env_var)
        if val and os.path.exists(val):
            search_dirs.append(Path(val))

    # 3. Standard installation candidates
    for candidate in DEFAULT_ENGINE_CANDIDATES:
        if os.path.exists(candidate):
            search_dirs.append(Path(candidate))

    # 4. Registry paths
    for reg_path in query_registry_engine_paths():
        p = Path(reg_path)
        if p.exists() and p not in search_dirs:
            search_dirs.append(p)

    for engine_dir in search_dirs:
        # Check Build.bat / Build.sh (handles bundled DotNet environment automatically in UE5.x)
        ubt_batch = engine_dir / "Engine" / "Build" / "BatchFiles" / ("Build.bat" if sys.platform == "win32" else "Build.sh")
        if ubt_batch.is_file():
            return ubt_batch, engine_dir

        # Check standard DotNET UBT binary as fallback
        ubt_path = engine_dir / "Engine" / "Binaries" / "DotNET" / "UnrealBuildTool" / "UnrealBuildTool.exe"
        if ubt_path.is_file():
            return ubt_path, engine_dir

    return None, None

def parse_ubt_output(raw_output: str) -> tuple[list[dict], list[dict]]:
    errors = []
    warnings = []
    
    # Pattern for MSVC/Clang: File.cpp(Line,Col): error/warning Cxxxx: message
    msvc_pattern = re.compile(
        r"^(?P<file>[A-Za-z]:\\[^:\r\n]+)\((?P<line>\d+)(?:,\s*(?P<col>\d+))?\)\s*:\s*(?P<severity>fatal error|error|warning)\s*(?P<code>[A-Za-z0-9]+)?\s*:\s*(?P<message>.+)$",
        re.MULTILINE
    )
    
    # Pattern for Clang/GCC: /path/file.cpp:Line:Col: error/warning: message
    clang_pattern = re.compile(
        r"^(?P<file>[A-Za-z]:\\[^:\r\n]+|/[^:\r\n]+):(?P<line>\d+):(?P<col>\d+):\s*(?P<severity>fatal error|error|warning):\s*(?P<message>.+)$",
        re.MULTILINE
    )

    # Pattern for UHT errors
    uht_pattern = re.compile(
        r"^(?P<file>[A-Za-z]:\\[^:\r\n]+)\((?P<line>\d+)\)\s*:\s*(?:LogWindows:\s*)?Error:\s*(?P<message>.+)$",
        re.MULTILINE
    )

    # Pattern for Linker errors: obj/lib : error LNKxxxx: message
    lnk_pattern = re.compile(
        r"^(?P<file>[^\r\n]+?\.(?:obj|lib))\s*:\s*error\s*(?P<code>LNK\d+)\s*:\s*(?P<message>.+)$",
        re.MULTILINE
    )

    # General error line
    generic_error = re.compile(r"^\s*Error\s*:\s*(?P<message>.+)$", re.MULTILINE)

    seen = set()

    for match in msvc_pattern.finditer(raw_output):
        item = {
            "file": match.group("file").strip(),
            "line": int(match.group("line")),
            "column": int(match.group("col")) if match.group("col") else None,
            "code": match.group("code") or "",
            "message": match.group("message").strip()
        }
        key = (item["file"], item["line"], item["code"], item["message"])
        if key not in seen:
            seen.add(key)
            if "warning" in match.group("severity").lower():
                warnings.append(item)
            else:
                errors.append(item)

    for match in clang_pattern.finditer(raw_output):
        item = {
            "file": match.group("file").strip(),
            "line": int(match.group("line")),
            "column": int(match.group("col")),
            "code": "",
            "message": match.group("message").strip()
        }
        key = (item["file"], item["line"], item["message"])
        if key not in seen:
            seen.add(key)
            if "warning" in match.group("severity").lower():
                warnings.append(item)
            else:
                errors.append(item)

    for match in uht_pattern.finditer(raw_output):
        item = {
            "file": match.group("file").strip(),
            "line": int(match.group("line")),
            "code": "UHT",
            "message": match.group("message").strip()
        }
        key = (item["file"], item["line"], item["code"], item["message"])
        if key not in seen:
            seen.add(key)
            errors.append(item)

    for match in lnk_pattern.finditer(raw_output):
        item = {
            "file": match.group("file").strip(),
            "line": 0,
            "code": match.group("code") or "LNK",
            "message": match.group("message").strip()
        }
        key = (item["file"], item["line"], item["code"], item["message"])
        if key not in seen:
            seen.add(key)
            errors.append(item)

    for match in generic_error.finditer(raw_output):
        msg = match.group("message").strip()
        if not any(msg in e["message"] for e in errors):
            errors.append({
                "file": "",
                "line": 0,
                "code": "GENERIC_ERROR",
                "message": msg
            })

    return errors, warnings

def run_build(
    uproject_path: Path,
    target: str,
    configuration: str,
    platform: str,
    engine_dir: str | None = None,
    clean: bool = False
) -> dict:
    ubt_exe, detected_engine = locate_ubt(engine_dir)
    
    if not ubt_exe:
        return {
            "status": "FAILURE",
            "exit_code": -1,
            "target": target,
            "configuration": configuration,
            "platform": platform,
            "error_count": 1,
            "warning_count": 0,
            "errors": [{
                "file": "",
                "line": 0,
                "code": "UBT_NOT_FOUND",
                "message": f"Could not locate UnrealBuildTool / Build.bat. Ensure UE 5.8 is installed or set UE_5_8_PATH / UNREAL_ENGINE_PATH."
            }],
            "warnings": [],
            "duration_seconds": 0.0,
            "summary": "Failed: UnrealBuildTool binary not found."
        }

    cmd = [
        str(ubt_exe),
        target,
        platform,
        configuration,
        f"-Project={str(uproject_path.resolve())}",
        "-WaitMutex",
        "-NoHotReload"
    ]

    if clean:
        cmd.append("-Clean")

    start_time = time.time()
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace"
        )
        duration = round(time.time() - start_time, 2)
        raw_output = proc.stdout
        exit_code = proc.returncode
    except Exception as ex:
        duration = round(time.time() - start_time, 2)
        return {
            "status": "FAILURE",
            "exit_code": -2,
            "target": target,
            "configuration": configuration,
            "platform": platform,
            "error_count": 1,
            "warning_count": 0,
            "errors": [{
                "file": "",
                "line": 0,
                "code": "EXEC_EXCEPTION",
                "message": f"Subprocess exception while executing UBT: {str(ex)}"
            }],
            "warnings": [],
            "duration_seconds": duration,
            "summary": f"Failed with exception: {str(ex)}"
        }

    errors, warnings = parse_ubt_output(raw_output)

    # If return code is non-zero but regex captured 0 errors, extract last lines
    if exit_code != 0 and len(errors) == 0:
        lines = [l.strip() for l in raw_output.strip().splitlines() if l.strip()]
        tail_lines = lines[-10:] if len(lines) >= 10 else lines
        errors.append({
            "file": "",
            "line": 0,
            "code": f"EXIT_{exit_code}",
            "message": "UBT exited with non-zero code. Output summary: " + " | ".join(tail_lines)
        })

    status = "SUCCESS" if exit_code == 0 and len(errors) == 0 else "FAILURE"
    summary = f"Build {status.lower()} in {duration}s. Errors: {len(errors)}, Warnings: {len(warnings)}."

    return {
        "status": status,
        "exit_code": exit_code,
        "target": target,
        "configuration": configuration,
        "platform": platform,
        "engine_dir": str(detected_engine) if detected_engine else "",
        "error_count": len(errors),
        "warning_count": len(warnings),
        "errors": errors,
        "warnings": warnings,
        "duration_seconds": duration,
        "summary": summary
    }

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Build Harness")
    parser.add_argument("--uproject", type=str, help="Path to .uproject file")
    parser.add_argument("--target", type=str, default=None, help="Target to build (default: <ProjectName>Editor)")
    parser.add_argument("--config", type=str, default="Development", help="Configuration (default: Development)")
    parser.add_argument("--platform", type=str, default="Win64", help="Platform (default: Win64)")
    parser.add_argument("--engine-dir", type=str, default=None, help="Explicit Unreal Engine root directory")
    parser.add_argument("--clean", action="store_true", help="Perform a clean build before compilation")
    parser.add_argument("--json", action="store_true", default=True, help="Emit output as JSON (default: True)")
    parser.add_argument("--pretty", action="store_true", help="Format JSON with indentations")

    args = parser.parse_args()

    project_root = Path.cwd()
    uproject_path = Path(args.uproject) if args.uproject else find_uproject_file(project_root)

    if not uproject_path or not uproject_path.is_file():
        result = {
            "status": "FAILURE",
            "error_count": 1,
            "warning_count": 0,
            "errors": [{
                "file": "",
                "line": 0,
                "code": "UPROJECT_NOT_FOUND",
                "message": f"Could not find any .uproject file in or above '{project_root}'."
            }],
            "warnings": [],
            "summary": "Failed: .uproject not found."
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    project_name = uproject_path.stem
    target_name = args.target if args.target else f"{project_name}Editor"

    result = run_build(
        uproject_path=uproject_path,
        target=target_name,
        configuration=args.config,
        platform=args.platform,
        engine_dir=args.engine_dir,
        clean=args.clean
    )

    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "SUCCESS" else 1)

if __name__ == "__main__":
    main()
