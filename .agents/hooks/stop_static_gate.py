#!/usr/bin/env python3
"""
Antigravity Stop Hook: Deterministic Pre-Completion Static Gate
Prevents an agent from ending its turn with unverified C++ (.h/.cpp) static violations.
Checks git-modified/untracked C++ files under Source/ and blocks stop once via
decision='continue' if `Scripts/pipeline.py static` reports errors.
"""

import json
import subprocess
import sys
from pathlib import Path


def get_workspace_root(payload: dict) -> Path:
    cwd = payload.get("cwd")
    if cwd and Path(cwd).exists():
        return Path(cwd).resolve()
    ws_paths = payload.get("workspacePaths") or []
    for p in ws_paths:
        if p and Path(p).exists():
            return Path(p).resolve()
    return Path(__file__).resolve().parent.parent.parent


def get_dirty_cpp_files(repo_root: Path) -> list[str]:
    try:
        proc = subprocess.run(
            ["git", "status", "--porcelain", "--", "Source"],
            cwd=str(repo_root),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=10,
        )
        if proc.returncode != 0:
            return []
    except Exception:
        return []

    dirty = []
    for line in proc.stdout.splitlines():
        if len(line) < 4:
            continue
        status = line[:2]
        if "D" in status:
            continue
        path_part = line[3:].strip()
        if " -> " in path_part:
            path_part = path_part.split(" -> ", 1)[1].strip()
        path_part = path_part.strip('"')
        if path_part.endswith((".h", ".cpp")):
            full = repo_root / path_part
            if full.exists():
                dirty.append(path_part.replace("\\", "/"))
    return sorted(set(dirty))


def get_dirty_harness_files(repo_root: Path) -> list[str]:
    """Detects modified/untracked protected harness files (catches run_command edits that bypass PreToolUse)."""
    try:
        proc = subprocess.run(
            ["git", "status", "--porcelain", "--", "Scripts", ".agents", "AGENTS.md"],
            cwd=str(repo_root),
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=10,
        )
        if proc.returncode != 0:
            return []
    except Exception:
        return []

    protected_exact = {
        "agents.md",
        "scripts/verify_replication.py",
        "scripts/verify_gas.py",
        "scripts/audit_bandwidth.py",
        "scripts/pipeline.py",
        "scripts/build_harness.py",
        "scripts/run_tests.py",
        "scripts/simulate_net_pie.py",
        "scripts/ue_env.py",
        "scripts/linter_baseline.json",
        ".agents/hooks.json",
    }
    protected_prefixes = ("scripts/tests/", ".agents/hooks/", ".agents/rules/")

    dirty = []
    for line in proc.stdout.splitlines():
        if len(line) < 4:
            continue
        path_part = line[3:].strip()
        if " -> " in path_part:
            path_part = path_part.split(" -> ", 1)[1].strip()
        norm = path_part.strip('"').replace("\\", "/")
        low = norm.lower()
        if low in protected_exact or any(low.startswith(p) for p in protected_prefixes):
            dirty.append(norm)
    return sorted(set(dirty))


def evaluate_stop(payload: dict) -> dict:
    # Never trap non-normal termination or re-trigger infinitely after a Stop continuation
    if payload.get("continuationActive"):
        return {"decision": "allow"}

    term_reason = payload.get("terminationReason", "NO_TOOL_CALL")
    if term_reason and term_reason != "NO_TOOL_CALL":
        return {"decision": "allow"}

    repo_root = get_workspace_root(payload)
    pipeline_script = repo_root / "Scripts" / "pipeline.py"
    if not pipeline_script.is_file():
        return {"decision": "allow"}

    failures = []

    # 1. If any harness/rule/hook files were modified, enforce harness meta-tests (including Doc Drift & Stale Baseline)
    dirty_harness = get_dirty_harness_files(repo_root)
    meta_test_script = repo_root / "Scripts" / "tests" / "test_linters.py"
    if dirty_harness and meta_test_script.is_file():
        proc = subprocess.run(
            [sys.executable, str(meta_test_script)],
            cwd=str(repo_root),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=20,
        )
        if proc.returncode != 0:
            failures.append(f"[Harness Meta-Tests ({', '.join(dirty_harness[:3])})]\n{proc.stdout.strip()}")

    # 2. If C++ files under Source/ were modified, enforce deterministic static gate
    dirty_cpp = get_dirty_cpp_files(repo_root)
    if dirty_cpp:
        if len(dirty_cpp) <= 5:
            for rel_file in dirty_cpp:
                proc = subprocess.run(
                    [sys.executable, str(pipeline_script), "static", "--file", rel_file],
                    cwd=str(repo_root),
                    stdout=subprocess.PIPE,
                    stderr=subprocess.STDOUT,
                    text=True,
                    encoding="utf-8",
                    errors="replace",
                    timeout=20,
                )
                if proc.returncode != 0:
                    failures.append(f"[{rel_file}]\n{proc.stdout.strip()}")
        else:
            proc = subprocess.run(
                [sys.executable, str(pipeline_script), "static"],
                cwd=str(repo_root),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                encoding="utf-8",
                errors="replace",
                timeout=25,
            )
            if proc.returncode != 0:
                failures.append(proc.stdout.strip())

    if failures:
        joined = "\n\n".join(failures)[:2500]
        return {
            "decision": "continue",
            "reason": (
                "Pre-Completion Gate FAILED on modified C++ or harness files.\n"
                "Fix all deterministic errors and verify with `python Scripts/pipeline.py static` / `python Scripts/tests/test_linters.py`:\n\n"
                f"{joined}"
            ),
        }

    return {"decision": "allow"}


def main() -> None:
    try:
        raw = sys.stdin.read()
        payload = json.loads(raw) if raw.strip() else {}
    except Exception:
        print(json.dumps({"decision": "allow"}))
        return

    print(json.dumps(evaluate_stop(payload)))


if __name__ == "__main__":
    main()
