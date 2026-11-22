#!/usr/bin/env python3
"""
Unreal Engine 5.8 Multiplayer Network Degradation & Headless PIE Simulator
Runs headless UnrealEditor-Cmd under simulated network lag/loss/duplication:
- Validates that the target .umap asset actually exists before launch (fail fast).
- Injects -pktlag, -pktloss, -pktdup, -nullrhi, -unattended flags into UnrealEditor-Cmd.exe.
- Executes real FC automation tests and parses BOTH network log anomalies and test results.
- Refuses to pass on timeout, missing map, 0 executed tests, or critical network/assertion errors.
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

DEFAULT_MAP = "/Game/Level/Lv_BattleTest"
DEFAULT_FILTER = "FC."


def resolve_map_file(project_root: Path, map_name: str) -> Path | None:
    """Resolves a /Game/... map path to an actual .umap file on disk."""
    clean = map_name.strip().replace("\\", "/")
    if clean.startswith("/Game/"):
        rel = clean[len("/Game/") :]
        if "." in rel:
            rel = rel.split(".")[0]
        candidate = project_root / "Content" / f"{rel}.umap"
        return candidate if candidate.is_file() else None
    p = Path(clean)
    if not p.is_absolute():
        p = project_root / p
    if p.suffix != ".umap":
        p = p.with_suffix(".umap")
    return p if p.is_file() else None


def parse_network_logs(raw_output: str) -> dict:
    """Parses editor log output to detect network sync anomalies, RPC drops, and fatal errors."""
    anomalies = []
    rpc_drops = 0
    desync_warnings = 0
    assertions = 0

    patterns = [
        (re.compile(r"Assertion failed:\s*(?P<msg>.+)", re.IGNORECASE), "ASSERTION", "CRITICAL"),
        (re.compile(r"Ensure condition failed:\s*(?P<msg>.+)", re.IGNORECASE), "ENSURE", "WARNING"),
        (re.compile(r"LogNetTraffic:\s*Error:\s*(?P<msg>.+)", re.IGNORECASE), "NET_TRAFFIC_ERROR", "ERROR"),
        (re.compile(r"RPC\s+(?:dropped|buffer overflow|rejected):\s*(?P<msg>.+)", re.IGNORECASE), "RPC_DROP", "ERROR"),
        (re.compile(r"Client\s+desynchronized|Movement\s+Correction|Correction\s+Count\s+High", re.IGNORECASE), "DESYNC", "WARNING"),
        (re.compile(r"LogNet:\s*Warning:\s*Network\s+failure:\s*(?P<msg>.+)", re.IGNORECASE), "NET_FAILURE", "ERROR"),
    ]

    for line in raw_output.splitlines():
        line_clean = line.strip()
        for pat, err_type, severity in patterns:
            m = pat.search(line_clean)
            if m:
                msg = m.group("msg") if "msg" in pat.groupindex else line_clean
                anomalies.append({"type": err_type, "severity": severity, "message": msg})
                if err_type == "RPC_DROP":
                    rpc_drops += 1
                elif err_type == "DESYNC":
                    desync_warnings += 1
                elif err_type == "ASSERTION":
                    assertions += 1

    return {
        "anomalies": anomalies,
        "total_anomalies": len(anomalies),
        "rpc_drops": rpc_drops,
        "desync_warnings": desync_warnings,
        "assertions": assertions,
    }


def run_simulated_network_pie(
    uproject_path: Path,
    engine_dir: str | None = None,
    pkt_lag: int = 150,
    pkt_loss: int = 5,
    pkt_dup: int = 2,
    timeout_seconds: int = 120,
    map_name: str = DEFAULT_MAP,
    test_filter: str = DEFAULT_FILTER,
) -> dict:
    project_root = uproject_path.resolve().parent
    resolved_map = resolve_map_file(project_root, map_name)
    if not resolved_map:
        return {
            "status": "FAIL",
            "exit_code": -3,
            "engine_dir": "",
            "map": map_name,
            "filter": test_filter,
            "network_parameters": {
                "pkt_lag_ms": pkt_lag,
                "pkt_loss_pct": pkt_loss,
                "pkt_dup_pct": pkt_dup,
            },
            "metrics": {"total_anomalies": 1, "rpc_drops": 0, "desync_warnings": 0, "assertions": 0},
            "anomalies": [
                {
                    "type": "MAP_NOT_FOUND",
                    "severity": "CRITICAL",
                    "message": f"Target map '{map_name}' does not exist under Content/. Pass a valid --map (e.g. {DEFAULT_MAP}).",
                }
            ],
            "summary": f"Failed: Map '{map_name}' not found on disk.",
        }

    editor_cmd, detected_engine = locate_editor_cmd(engine_dir)
    if not editor_cmd:
        return {
            "status": "FAIL",
            "exit_code": -1,
            "engine_dir": "",
            "map": map_name,
            "filter": test_filter,
            "network_parameters": {
                "pkt_lag_ms": pkt_lag,
                "pkt_loss_pct": pkt_loss,
                "pkt_dup_pct": pkt_dup,
            },
            "metrics": {"total_anomalies": 1, "rpc_drops": 0, "desync_warnings": 0, "assertions": 0},
            "anomalies": [
                {
                    "type": "BINARY_NOT_FOUND",
                    "severity": "CRITICAL",
                    "message": "UnrealEditor-Cmd.exe could not be located. Verify UE 5.8 installation or set UE_5_8_PATH.",
                }
            ],
            "summary": "Failed: UnrealEditor-Cmd binary not found.",
        }

    exec_cmds = f"Automation RunTests {test_filter}; Quit"
    cmd = [
        str(editor_cmd),
        str(uproject_path.resolve()),
        map_name,
        "-game",
        f"-ExecCmds={exec_cmds}",
        f"-pktlag={pkt_lag}",
        f"-pktloss={pkt_loss}",
        f"-pktdup={pkt_dup}",
        "-nullrhi",
        "-unattended",
        "-nopause",
        "-nosplash",
        "-stdout",
        "-FullStdOutLogOutput",
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
            timeout=timeout_seconds,
        )
        duration = round(time.time() - start_time, 2)
        raw_output = proc.stdout
        exit_code = proc.returncode
    except subprocess.TimeoutExpired as ex:
        duration = round(time.time() - start_time, 2)
        partial_output = ""
        if ex.output:
            partial_output = ex.output.decode("utf-8", errors="replace") if isinstance(ex.output, bytes) else str(ex.output)
        log_metrics = parse_network_logs(partial_output)
        log_metrics["anomalies"].append(
            {
                "type": "TIMEOUT",
                "severity": "CRITICAL",
                "message": f"Network simulation timed out after {timeout_seconds}s (hang/deadlock or insufficient timeout; timeout is never treated as PASS).",
            }
        )
        log_metrics["total_anomalies"] += 1
        return {
            "status": "FAIL",
            "exit_code": -4,
            "duration_seconds": duration,
            "engine_dir": str(detected_engine) if detected_engine else "",
            "map": map_name,
            "filter": test_filter,
            "network_parameters": {
                "pkt_lag_ms": pkt_lag,
                "pkt_loss_pct": pkt_loss,
                "pkt_dup_pct": pkt_dup,
            },
            "metrics": {
                "total_anomalies": log_metrics["total_anomalies"],
                "rpc_drops": log_metrics["rpc_drops"],
                "desync_warnings": log_metrics["desync_warnings"],
                "assertions": log_metrics["assertions"],
            },
            "anomalies": log_metrics["anomalies"],
            "summary": f"Network simulation FAILED: timed out after {duration}s under {pkt_lag}ms lag / {pkt_loss}% loss.",
        }
    except Exception as ex:
        duration = round(time.time() - start_time, 2)
        return {
            "status": "FAIL",
            "exit_code": -2,
            "duration_seconds": duration,
            "map": map_name,
            "filter": test_filter,
            "network_parameters": {
                "pkt_lag_ms": pkt_lag,
                "pkt_loss_pct": pkt_loss,
                "pkt_dup_pct": pkt_dup,
            },
            "metrics": {"total_anomalies": 1, "rpc_drops": 0, "desync_warnings": 0, "assertions": 0},
            "anomalies": [
                {
                    "type": "EXEC_EXCEPTION",
                    "severity": "CRITICAL",
                    "message": f"Exception while launching PIE simulation: {str(ex)}",
                }
            ],
            "summary": f"Failed with exception: {str(ex)}",
        }

    from run_tests import parse_automation_output, evaluate_test_run_status

    log_metrics = parse_network_logs(raw_output)
    tests, passed_count, failed_count, other_count = parse_automation_output(raw_output)
    test_status, test_failure_reason = evaluate_test_run_status(
        exit_code=exit_code,
        passed_count=passed_count,
        failed_count=failed_count,
        other_count=other_count,
        filter_name=test_filter,
    )

    if test_status != "PASSED":
        log_metrics["anomalies"].append(
            {"type": "AUTOMATION_TEST_FAILURE", "severity": "CRITICAL", "message": test_failure_reason}
        )
        log_metrics["total_anomalies"] += 1

    has_critical_errors = (
        log_metrics["assertions"] > 0
        or log_metrics["rpc_drops"] > 0
        or exit_code != 0
        or test_status != "PASSED"
    )
    status = "FAIL" if has_critical_errors else "PASS"

    return {
        "status": status,
        "exit_code": exit_code,
        "duration_seconds": duration,
        "engine_dir": str(detected_engine) if detected_engine else "",
        "map": map_name,
        "filter": test_filter,
        "tests_passed": passed_count,
        "tests_failed": failed_count,
        "tests_incomplete": other_count,
        "network_parameters": {
            "pkt_lag_ms": pkt_lag,
            "pkt_loss_pct": pkt_loss,
            "pkt_dup_pct": pkt_dup,
        },
        "metrics": {
            "total_anomalies": log_metrics["total_anomalies"],
            "rpc_drops": log_metrics["rpc_drops"],
            "desync_warnings": log_metrics["desync_warnings"],
            "assertions": log_metrics["assertions"],
        },
        "anomalies": log_metrics["anomalies"],
        "summary": (
            f"Network simulation {status.lower()}: {passed_count} passed / {failed_count} failed tests, "
            f"{log_metrics['rpc_drops']} RPC drops, {log_metrics['desync_warnings']} desync warnings, "
            f"{log_metrics['assertions']} assertions."
        ),
    }


def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Network Degradation Simulator")
    parser.add_argument("--uproject", type=str, help="Path to .uproject file")
    parser.add_argument("--engine-dir", type=str, default=None, help="Explicit Unreal Engine root directory")
    parser.add_argument("--lag", type=int, default=150, help="Simulated network packet latency in ms (default: 150)")
    parser.add_argument("--loss", type=int, default=5, help="Simulated packet loss percentage (default: 5)")
    parser.add_argument("--dup", type=int, default=2, help="Simulated packet duplication percentage (default: 2)")
    parser.add_argument("--timeout", type=int, default=120, help="Test timeout/duration in seconds (default: 120)")
    parser.add_argument("--map", type=str, default=DEFAULT_MAP, help=f"Map asset to launch (default: {DEFAULT_MAP})")
    parser.add_argument("--filter", type=str, default=DEFAULT_FILTER, help=f"Automation test filter (default: {DEFAULT_FILTER})")
    parser.add_argument("--json", action="store_true", default=True, help="Emit output as JSON (default: True)")
    parser.add_argument("--pretty", action="store_true", help="Format JSON with indentation")

    args = parser.parse_args()

    project_root = Path.cwd()
    uproject_path = Path(args.uproject) if args.uproject else find_uproject_file(project_root)

    if not uproject_path or not uproject_path.is_file():
        result = {
            "status": "FAIL",
            "exit_code": -1,
            "metrics": {"total_anomalies": 1, "rpc_drops": 0, "desync_warnings": 0, "assertions": 0},
            "anomalies": [
                {
                    "type": "UPROJECT_NOT_FOUND",
                    "severity": "CRITICAL",
                    "message": f"Could not find any .uproject file in or above '{project_root}'.",
                }
            ],
            "summary": "Failed: .uproject not found.",
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    result = run_simulated_network_pie(
        uproject_path=uproject_path,
        engine_dir=args.engine_dir,
        pkt_lag=args.lag,
        pkt_loss=args.loss,
        pkt_dup=args.dup,
        timeout_seconds=args.timeout,
        map_name=args.map,
        test_filter=args.filter,
    )

    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "PASS" else 1)


if __name__ == "__main__":
    main()
