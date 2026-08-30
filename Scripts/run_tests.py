#!/usr/bin/env python3
"""
Unreal Engine 5.8 Automation Test Runner
Executes headless automation tests via UnrealEditor-Cmd.exe and formats results as clean JSON.
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

def locate_editor_cmd(explicit_engine_dir: str | None = None) -> tuple[Path | None, Path | None]:
    search_dirs = []
    
    if explicit_engine_dir:
        search_dirs.append(Path(explicit_engine_dir))

    for env_var in ["UE_5_8_PATH", "UNREAL_ENGINE_PATH", "UE_ROOT", "ENGINE_PATH", "UE_ENGINE_DIR"]:
        val = os.environ.get(env_var)
        if val and os.path.exists(val):
            search_dirs.append(Path(val))

    for candidate in DEFAULT_ENGINE_CANDIDATES:
        if os.path.exists(candidate):
            search_dirs.append(Path(candidate))

    for reg_path in query_registry_engine_paths():
        p = Path(reg_path)
        if p.exists() and p not in search_dirs:
            search_dirs.append(p)

    for engine_dir in search_dirs:
        editor_cmd = engine_dir / "Engine" / "Binaries" / ("Win64" if sys.platform == "win32" else "Linux") / ("UnrealEditor-Cmd.exe" if sys.platform == "win32" else "UnrealEditor-Cmd")
        if editor_cmd.is_file():
            return editor_cmd, engine_dir

    return None, None

def parse_automation_output(raw_output: str) -> tuple[list[dict], int, int, int]:
    tests = []
    test_map = {}
    
    # Patterns for Automation Logs
    test_start_pat = re.compile(r"(?:Test\s+'(?P<name1>[^']+)'\s+Started|Test\s+Started\.\s+Name=\{(?P<name2>[^}]+)\}\s+Path=\{(?P<path2>[^}]+)\})", re.IGNORECASE)
    test_finish_pat = re.compile(r"(?:Test\s+'(?P<name1>[^']+)'\s+Completed\.\s*Result\s*=\s*(?P<result1>Passed|Failed|Warning)|Test\s+Completed\.\s+Result=\{(?P<result2>[^}]+)\}\s+Name=\{(?P<name2>[^}]+)\}\s+Path=\{(?P<path2>[^}]+)\})", re.IGNORECASE)
    test_result_alt = re.compile(r"Automation:\s+(?P<status>Passed|Failed)\s+(?P<name>[^\r\n]+)", re.IGNORECASE)
    error_line_pat = re.compile(r"Error:\s*(?P<msg>.+)", re.IGNORECASE)

    current_test = None

    for line in raw_output.splitlines():
        line_str = line.strip()
        
        start_match = test_start_pat.search(line_str)
        if start_match:
            test_name = start_match.group("path2") or start_match.group("name1") or start_match.group("name2")
            current_test = test_name
            if test_name not in test_map:
                test_map[test_name] = {
                    "test_name": test_name,
                    "status": "RUNNING",
                    "messages": []
                }
            continue

        finish_match = test_finish_pat.search(line_str)
        if finish_match:
            test_name = finish_match.group("path2") or finish_match.group("name1") or finish_match.group("name2")
            res_str = (finish_match.group("result1") or finish_match.group("result2") or "").upper()
            status = "PASSED" if res_str in ("PASSED", "SUCCESS") else "FAILED"
            if test_name in test_map:
                test_map[test_name]["status"] = status
            else:
                test_map[test_name] = {
                    "test_name": test_name,
                    "status": status,
                    "messages": []
                }
            current_test = None
            continue

        alt_match = test_result_alt.search(line_str)
        if alt_match:
            test_name = alt_match.group("name").strip()
            status = alt_match.group("status").upper()
            if test_name not in test_map:
                test_map[test_name] = {
                    "test_name": test_name,
                    "status": status,
                    "messages": []
                }
            else:
                test_map[test_name]["status"] = status
            continue

        err_match = error_line_pat.search(line_str)
        if err_match and current_test and current_test in test_map:
            test_map[current_test]["messages"].append(err_match.group("msg").strip())

    tests = list(test_map.values())
    passed_count = sum(1 for t in tests if t["status"] == "PASSED")
    failed_count = sum(1 for t in tests if t["status"] == "FAILED")
    other_count = len(tests) - passed_count - failed_count

    return tests, passed_count, failed_count, other_count

def run_tests(
    uproject_path: Path,
    filter_name: str,
    engine_dir: str | None = None,
    timeout_seconds: int = 300
) -> dict:
    editor_cmd, detected_engine = locate_editor_cmd(engine_dir)

    if not editor_cmd:
        return {
            "status": "FAILED",
            "passed_count": 0,
            "failed_count": 1,
            "duration_seconds": 0.0,
            "tests": [],
            "error": "Could not locate UnrealEditor-Cmd.exe. Ensure UE 5.8 is installed.",
            "summary": "Failed: UnrealEditor-Cmd binary not found."
        }

    exec_cmds = f"Automation RunTests {filter_name}; Quit"
    cmd = [
        str(editor_cmd),
        str(uproject_path.resolve()),
        f"-ExecCmds={exec_cmds}",
        "-nullrhi",
        "-unattended",
        "-nopause",
        "-nosplash",
        "-stdout",
        "-FullStdOutLogOutput"
    ]

    start_time = time.time()
    try:
        proc = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout_seconds
        )
        duration = round(time.time() - start_time, 2)
        raw_output = proc.stdout
        exit_code = proc.returncode
    except subprocess.TimeoutExpired:
        duration = round(time.time() - start_time, 2)
        return {
            "status": "FAILED",
            "passed_count": 0,
            "failed_count": 1,
            "duration_seconds": duration,
            "tests": [],
            "error": f"Automation test execution timed out after {timeout_seconds} seconds.",
            "summary": f"Failed: Execution timed out after {timeout_seconds}s."
        }
    except Exception as ex:
        duration = round(time.time() - start_time, 2)
        return {
            "status": "FAILED",
            "passed_count": 0,
            "failed_count": 1,
            "duration_seconds": duration,
            "tests": [],
            "error": str(ex),
            "summary": f"Failed with exception: {str(ex)}"
        }

    tests, passed_count, failed_count, other_count = parse_automation_output(raw_output)

    # Determine overall status
    overall_status = "PASSED" if failed_count == 0 and exit_code == 0 else "FAILED"
    summary = f"Automation Tests Completed: {passed_count} Passed, {failed_count} Failed (Duration: {duration}s)."

    return {
        "status": overall_status,
        "exit_code": exit_code,
        "filter": filter_name,
        "passed_count": passed_count,
        "failed_count": failed_count,
        "total_tests": len(tests),
        "duration_seconds": duration,
        "tests": tests,
        "summary": summary
    }

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Automation Test Runner")
    parser.add_argument("--uproject", type=str, help="Path to .uproject file")
    parser.add_argument("--filter", type=str, default=None, help="Test filter (default: project name)")
    parser.add_argument("--engine-dir", type=str, default=None, help="Explicit Unreal Engine root directory")
    parser.add_argument("--timeout", type=int, default=300, help="Execution timeout in seconds (default: 300)")
    parser.add_argument("--json", action="store_true", default=True, help="Emit output as JSON (default: True)")
    parser.add_argument("--pretty", action="store_true", help="Format JSON with indentations")

    args = parser.parse_args()

    project_root = Path.cwd()
    uproject_path = Path(args.uproject) if args.uproject else find_uproject_file(project_root)

    if not uproject_path or not uproject_path.is_file():
        result = {
            "status": "FAILED",
            "passed_count": 0,
            "failed_count": 1,
            "tests": [],
            "error": f"Could not find any .uproject file in or above '{project_root}'.",
            "summary": "Failed: .uproject not found."
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    project_name = uproject_path.stem
    filter_name = args.filter if args.filter else (project_name + ".")

    result = run_tests(
        uproject_path=uproject_path,
        filter_name=filter_name,
        engine_dir=args.engine_dir,
        timeout_seconds=args.timeout
    )

    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "PASSED" else 1)

if __name__ == "__main__":
    main()
