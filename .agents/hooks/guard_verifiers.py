#!/usr/bin/env python3
"""
Antigravity PreToolUse Hook: Anti-Reward-Hacking Verifier Guard
Requires explicit user confirmation (force_ask) before an autonomous agent modifies
static verification scripts, baseline allowlists, linter meta-tests, or hook configs.
"""

import json
import sys
from pathlib import Path

PROTECTED_EXACT = {
    "scripts/verify_replication.py",
    "scripts/verify_gas.py",
    "scripts/audit_bandwidth.py",
    "scripts/pipeline.py",
    "scripts/run_tests.py",
    "scripts/simulate_net_pie.py",
    "scripts/linter_baseline.json",
    ".agents/hooks.json",
}

PROTECTED_PREFIXES = (
    "scripts/tests/",
    ".agents/hooks/",
)


def normalize_relative(path_str: str, workspace_roots: list[str]) -> str:
    if not path_str:
        return ""
    raw = Path(path_str)
    for root in workspace_roots:
        try:
            root_path = Path(root).resolve()
            resolved = raw.resolve() if raw.is_absolute() else (root_path / raw).resolve()
            rel = resolved.relative_to(root_path).as_posix().lower()
            return rel
        except Exception:
            continue
    return path_str.replace("\\", "/").lstrip("./").lower()


def evaluate_pre_tool_use(payload: dict) -> dict:
    tool_call = payload.get("toolCall") or {}
    args = tool_call.get("args") or {}
    target_path = args.get("Path") or args.get("path") or args.get("TargetFile") or ""

    roots = list(payload.get("workspacePaths") or [])
    if payload.get("cwd"):
        roots.insert(0, payload["cwd"])

    rel = normalize_relative(str(target_path), roots)
    if rel in PROTECTED_EXACT or any(rel.startswith(prefix) for prefix in PROTECTED_PREFIXES):
        return {
            "decision": "force_ask",
            "reason": (
                f"Anti-Reward-Hacking Guard: '{rel}' is a protected harness verifier, "
                "baseline, or hook file. Confirm explicitly before allowing modifications."
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

    print(json.dumps(evaluate_pre_tool_use(payload)))


if __name__ == "__main__":
    main()
