#!/usr/bin/env python3
"""
Unreal Engine 5.8 Unified Pipeline Runner (Scripts/pipeline.py)
A token-efficient, fail-fast master runner for FC project verification and automation workflows.

Modes:
  - static  : Runs GAS verification, bandwidth audit, and replication analysis (Ultra fast, ~1s).
  - build   : Runs UBT build harness and parses compilation diagnostics.
  - balance : Runs mathematical TTK and weapon balance simulation.
  - test    : Runs headless automation tests.
  - net     : Runs multiplayer network degradation PIE simulation.
  - full    : Runs static -> build -> test sequentially with fail-fast.

Usage:
  python Scripts/pipeline.py static
  python Scripts/pipeline.py build
  python Scripts/pipeline.py balance
  python Scripts/pipeline.py full
  python Scripts/pipeline.py full --auto-commit -m "feat(combat): implement stamina ability"
"""

import sys
import os
import time
import json
import subprocess
import argparse
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
SCRIPTS_DIR = PROJECT_ROOT / "Scripts"

def run_script_json(script_name: str, extra_args: list[str] = None) -> tuple[int, dict, str]:
    """Runs a Python script in Scripts/ and parses its JSON output."""
    script_path = SCRIPTS_DIR / script_name
    if not script_path.exists():
        return 1, {
            "status": "FAIL",
            "error_count": 1,
            "summary": f"Script {script_name} not found in {SCRIPTS_DIR}."
        }, f"File not found: {script_path}"

    cmd = [sys.executable, str(script_path), "--json"]
    if extra_args:
        cmd.extend(extra_args)

    start_time = time.perf_counter()
    try:
        proc = subprocess.run(
            cmd,
            cwd=str(PROJECT_ROOT),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=300
        )
        elapsed = time.perf_counter() - start_time
    except subprocess.TimeoutExpired:
        return 1, {
            "status": "TIMEOUT",
            "error_count": 1,
            "summary": f"Execution timed out after 300s: {script_name}"
        }, "Timeout"
    except Exception as e:
        return 1, {
            "status": "ERROR",
            "error_count": 1,
            "summary": f"Failed to execute {script_name}: {str(e)}"
        }, str(e)

    stdout = proc.stdout.strip()
    parsed_json = None

    # Try parsing JSON from output
    for line in stdout.splitlines():
        line_s = line.strip()
        if (line_s.startswith("{") and line_s.endswith("}")) or (line_s.startswith("[") and line_s.endswith("]")):
            try:
                parsed_json = json.loads(line_s)
                break
            except Exception:
                pass

    if not parsed_json:
        try:
            parsed_json = json.loads(stdout)
        except Exception:
            parsed_json = {
                "status": "PASS" if proc.returncode == 0 else "FAIL",
                "error_count": 0 if proc.returncode == 0 else 1,
                "summary": stdout[:200] if stdout else (proc.stderr[:200] if proc.stderr else f"Exit code {proc.returncode}")
            }

    parsed_json["_duration"] = round(elapsed, 2)
    return proc.returncode, parsed_json, stdout

def run_static_checks(json_output: bool = False, skip_gas: bool = False, gas_only: bool = False, repl_only: bool = False) -> tuple[bool, dict]:
    """Runs GAS, Bandwidth, and Replication static verification modularly."""
    results = {}
    total_errors = 0
    total_warnings = 0
    all_passed = True

    checks = []
    if gas_only:
        checks.append(("GAS Static Verification", "verify_gas.py"))
    elif repl_only:
        checks.append(("Bandwidth & Net Audit", "audit_bandwidth.py"))
        checks.append(("Replication Architecture", "verify_replication.py"))
    else:
        if not skip_gas:
            checks.append(("GAS Static Verification", "verify_gas.py"))
        checks.append(("Bandwidth & Net Audit", "audit_bandwidth.py"))
        checks.append(("Replication Architecture", "verify_replication.py"))

    for label, script in checks:
        retcode, data, _ = run_script_json(script)
        status = data.get("status", "FAIL").upper()
        errs = data.get("error_count", 0)
        warns = data.get("warning_count", 0)
        duration = data.get("_duration", 0.0)

        total_errors += errs
        total_warnings += warns
        is_ok = (status in ["PASS", "PASSED", "WARN"] and errs == 0)
        if not is_ok:
            all_passed = False

        results[label] = {
            "passed": is_ok,
            "status": status,
            "errors": errs,
            "warnings": warns,
            "duration": duration,
            "details": data
        }

    summary = {
        "mode": "gas" if gas_only else ("replication" if repl_only else "static"),
        "passed": all_passed,
        "total_errors": total_errors,
        "total_warnings": total_warnings,
        "checks": results
    }
    return all_passed, summary

def run_build_check(clean: bool = False) -> tuple[bool, dict]:
    """Runs UBT Build Harness."""
    extra = ["--clean"] if clean else []
    retcode, data, _ = run_script_json("build_harness.py", extra)
    status = data.get("status", "FAILURE").upper()
    errs = data.get("error_count", 0)
    warns = data.get("warning_count", 0)
    duration = data.get("_duration", 0.0)
    is_ok = (status == "SUCCESS" and errs == 0)

    summary = {
        "mode": "build",
        "passed": is_ok,
        "status": status,
        "errors": errs,
        "warnings": warns,
        "duration": duration,
        "details": data
    }
    return is_ok, summary

def run_balance_check() -> tuple[bool, dict]:
    """Runs TTK and weapon balance simulation."""
    retcode, data, _ = run_script_json("simulate_ttk_balance.py", ["--benchmark"])
    status = data.get("status", "PASS").upper()
    is_ok = (status == "PASS" and retcode == 0)
    duration = data.get("_duration", 0.0)

    summary = {
        "mode": "balance",
        "passed": is_ok,
        "status": status,
        "duration": duration,
        "details": data
    }
    return is_ok, summary

def run_test_check() -> tuple[bool, dict]:
    """Runs headless automation tests."""
    retcode, data, _ = run_script_json("run_tests.py")
    status = data.get("status", "SUCCESS").upper()
    errs = data.get("failed_count", 0)
    is_ok = (status in ["SUCCESS", "PASSED"] and errs == 0 and retcode == 0)
    duration = data.get("_duration", 0.0)

    summary = {
        "mode": "test",
        "passed": is_ok,
        "status": status,
        "errors": errs,
        "duration": duration,
        "details": data
    }
    return is_ok, summary

def run_net_pie_check() -> tuple[bool, dict]:
    """Runs multiplayer network degradation PIE simulation."""
    retcode, data, _ = run_script_json("simulate_net_pie.py")
    status = data.get("status", "PASS").upper()
    is_ok = (status == "PASS" and retcode == 0)
    duration = data.get("_duration", 0.0)

    summary = {
        "mode": "net",
        "passed": is_ok,
        "status": status,
        "duration": duration,
        "details": data
    }
    return is_ok, summary

def execute_auto_commit(commit_msg: str | None = None) -> bool:
    """Stages modified and new files, then creates a Git commit."""
    try:
        git_check = subprocess.run(["git", "status", "--porcelain"], cwd=str(PROJECT_ROOT), stdout=subprocess.PIPE, text=True)
        if not git_check.stdout.strip():
            print("[GIT] Nothing to commit, working tree clean.")
            return True

        subprocess.run(["git", "add", "Source/", "Config/", "Scripts/", "AGENTS.md"], cwd=str(PROJECT_ROOT), check=True)
        
        msg = commit_msg or "refactor(core): verify and pass pipeline checks"
        commit_proc = subprocess.run(["git", "commit", "-m", msg], cwd=str(PROJECT_ROOT), stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if commit_proc.returncode == 0:
            print(f"[GIT COMMIT] {msg.splitlines()[0]}")
            return True
        else:
            print(f"[GIT WARN] Commit failed: {commit_proc.stderr.strip()}")
            return False
    except Exception as e:
        print(f"[GIT ERROR] {str(e)}")
        return False

def print_compact_summary(summary: dict):
    """Prints a clean, ultra-token-efficient summary."""
    mode = summary.get("mode", "")
    passed = summary.get("passed", False)
    badge = "PASS" if passed else "FAIL"

    if mode in ["static", "gas", "replication"]:
        mode_title = "GAS Verification" if mode == "gas" else ("Replication & Bandwidth Audit" if mode == "replication" else "Static Verification Summary")
        print(f"[{badge}] {mode_title}:")
        for name, item in summary.get("checks", {}).items():
            item_badge = "PASS" if item["passed"] else "FAIL"
            print(f"  - [{item_badge}] {name}: {item['errors']} err, {item['warnings']} warn ({item['duration']}s)")
            if not item["passed"]:
                violations = item["details"].get("violations", []) or item["details"].get("issues", [])
                for v in violations[:3]:
                    f = v.get("file", "")
                    l = v.get("line", 0)
                    msg = v.get("detail") or v.get("message") or v.get("description") or ""
                    print(f"      -> {f}:{l} - {msg}")
        print(f"Total: {summary['total_errors']} error(s), {summary['total_warnings']} warning(s).")

    elif mode == "build":
        duration = summary.get("duration", 0)
        print(f"[{badge}] Build UBT Harness ({duration}s): {summary.get('errors', 0)} error(s), {summary.get('warnings', 0)} warning(s)")
        if not passed:
            errs = summary["details"].get("errors", [])
            for e in errs[:3]:
                f = e.get("file", "")
                l = e.get("line", 0)
                msg = e.get("message", "")
                print(f"  -> Error: {f}:{l} - {msg}")

    elif mode == "balance":
        duration = summary.get("duration", 0)
        print(f"[{badge}] TTK / Balance Simulation ({duration}s):")
        archetypes = summary.get("details", {}).get("archetypes", {})
        for arch, data in list(archetypes.items())[:4]:
            tiers = data.get("tiers", {})
            unarmored_dps = tiers.get("Unarmored", {}).get("effective_dps", 0)
            heavy_dps = tiers.get("Heavy", {}).get("effective_dps", 0)
            print(f"  - {arch:<14} | Unarmored DPS: {unarmored_dps:>6.1f} | Heavy DPS: {heavy_dps:>6.1f}")

    elif mode in ["test", "net"]:
        duration = summary.get("duration", 0)
        print(f"[{badge}] {mode.upper()} Simulation ({duration}s): status={summary.get('status')}")

    elif mode == "full":
        print(f"[{badge}] Full Pipeline Execution:")
        for step in summary.get("steps", []):
            s_mode = step.get("mode")
            s_badge = "PASS" if step.get("passed") else "FAIL"
            s_dur = step.get("duration", 0)
            print(f"  - [{s_badge}] Step '{s_mode}' ({s_dur}s)")
            if not step.get("passed"):
                if s_mode == "static":
                    print(f"      Static errors: {step.get('total_errors', 0)}")
                elif s_mode == "build":
                    errs = step.get("details", {}).get("errors", [])
                    if errs:
                        print(f"      Build error: {errs[0].get('message')}")

def main():
    parser = argparse.ArgumentParser(
        description="Unified Lean Pipeline Runner for FC (UE5.8)",
        formatter_class=argparse.RawTextHelpFormatter
    )
    parser.add_argument(
        "mode",
        nargs="?",
        default="static",
        choices=["static", "gas", "replication", "build", "balance", "test", "net", "full"],
        help="Pipeline mode to execute:\n"
             "  static      : Fast static checks (Bandwidth, Replication, GAS)\n"
             "  gas         : Focused GAS static verification (verify_gas.py)\n"
             "  replication : Focused Replication & Bandwidth audit\n"
             "  build       : UBT compile harness with diagnostic extraction\n"
             "  balance     : Mathematical TTK and balance benchmark simulation\n"
             "  test        : Automation test execution\n"
             "  net         : Headless PIE multiplayer network degradation simulation\n"
             "  full        : Sequential full pipeline (static -> build -> test)"
    )
    parser.add_argument("--json", action="store_true", help="Emit output strictly as aggregated JSON")
    parser.add_argument("--skip-gas", action="store_true", help="Skip GAS static check in static/full mode (useful for non-GAS tasks)")
    parser.add_argument("--clean", action="store_true", help="Clean build before compiling (build/full mode)")
    parser.add_argument("--auto-commit", "-c", action="store_true", help="Automatically git commit on successful run")
    parser.add_argument("--message", "-m", type=str, default=None, help="Custom commit message for --auto-commit")

    args = parser.parse_args()

    overall_passed = True
    aggregated_report = {}

    if args.mode == "static":
        overall_passed, aggregated_report = run_static_checks(json_output=args.json, skip_gas=args.skip_gas)

    elif args.mode == "gas":
        overall_passed, aggregated_report = run_static_checks(json_output=args.json, gas_only=True)

    elif args.mode == "replication":
        overall_passed, aggregated_report = run_static_checks(json_output=args.json, repl_only=True)

    elif args.mode == "build":
        overall_passed, aggregated_report = run_build_check(clean=args.clean)

    elif args.mode == "balance":
        overall_passed, aggregated_report = run_balance_check()

    elif args.mode == "test":
        overall_passed, aggregated_report = run_test_check()

    elif args.mode == "net":
        overall_passed, aggregated_report = run_net_pie_check()

    elif args.mode == "full":
        steps = []
        # Step 1: Static
        passed_static, static_data = run_static_checks(skip_gas=args.skip_gas)
        steps.append(static_data)
        if not passed_static:
            overall_passed = False
        else:
            # Step 2: Build
            passed_build, build_data = run_build_check(clean=args.clean)
            steps.append(build_data)
            if not passed_build:
                overall_passed = False
            else:
                # Step 3: Test
                passed_test, test_data = run_test_check()
                steps.append(test_data)
                if not passed_test:
                    overall_passed = False

        aggregated_report = {
            "mode": "full",
            "passed": overall_passed,
            "steps": steps
        }

    # Auto commit if requested and passed
    if overall_passed and args.auto_commit:
        commit_success = execute_auto_commit(args.message)
        aggregated_report["commit_success"] = commit_success

    if args.json:
        print(json.dumps(aggregated_report, indent=2))
    else:
        print_compact_summary(aggregated_report)

    sys.exit(0 if overall_passed else 1)

if __name__ == "__main__":
    main()
