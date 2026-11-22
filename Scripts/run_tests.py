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

from ue_env import (
    DEFAULT_ENGINE_CANDIDATES,
    find_uproject_file,
    locate_editor_cmd,
    query_registry_engine_paths,
)

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

def evaluate_test_run_status(exit_code: int, passed_count: int, failed_count: int, other_count: int, filter_name: str) -> tuple[str, str]:
    """Determines strict PASS/FAIL status; refuses to pass on 0 executed tests or incomplete runs."""
    if exit_code != 0:
        return "FAILED", f"UnrealEditor-Cmd exited with non-zero code {exit_code}."
    if failed_count > 0:
        return "FAILED", f"{failed_count} automation test(s) failed."
    if other_count > 0:
        return "FAILED", f"{other_count} automation test(s) started but did not complete cleanly."
    if passed_count == 0:
        return "FAILED", f"Zero automation tests matched filter '{filter_name}' (0-test pass is forbidden)."
    return "PASSED", ""


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

    overall_status, failure_reason = evaluate_test_run_status(
        exit_code=exit_code,
        passed_count=passed_count,
        failed_count=failed_count,
        other_count=other_count,
        filter_name=filter_name,
    )
    summary = f"Automation Tests Completed: {passed_count} Passed, {failed_count} Failed, {other_count} Incomplete (Duration: {duration}s)."
    if failure_reason:
        summary += f" Reason: {failure_reason}"

    result = {
        "status": overall_status,
        "exit_code": exit_code,
        "filter": filter_name,
        "passed_count": passed_count,
        "failed_count": failed_count,
        "incomplete_count": other_count,
        "total_tests": len(tests),
        "duration_seconds": duration,
        "tests": tests,
        "summary": summary
    }
    if failure_reason:
        result["error"] = failure_reason
    return result

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
