#!/usr/bin/env python3
"""
Unreal Engine 5.8 Bandwidth & Network Performance Static Audit Script
Scans C++ headers and source files in Source/ to identify networking performance anti-patterns:
- Flags UPROPERTY(Replicated) on raw TArray<...> without FastArraySerializer wrappers.
- Flags replicated primitive types lacking COND_* replication conditions (or raw DOREPLIFETIME usage).
- Flags heavy structs, strings, or sets/maps marked Replicated without delta serialization.
Emits structured JSON audit reports for autonomous agent consumption.
"""

import sys
import os
import re
import json
import argparse
from pathlib import Path

# Known heavy types that should not be replicated naively
HEAVY_UNREPLICATED_TYPES = [
    r"FString",
    r"TMap\s*<",
    r"TSet\s*<",
]

class BandwidthAuditor:
    def __init__(self, source_dir: Path):
        self.source_dir = source_dir
        self.issues = []

    def add_issue(self, file_path: Path, line_number: int, severity: str, description: str, recommendation: str):
        try:
            rel_path = file_path.relative_to(self.source_dir.parent).as_posix() if self.source_dir.parent in file_path.parents else file_path.as_posix()
        except Exception:
            rel_path = file_path.as_posix()
        self.issues.append({
            "file": rel_path,
            "line": line_number,
            "severity": severity,
            "description": description,
            "recommendation": recommendation
        })

    def strip_comments(self, text: str) -> str:
        def replace_block(match):
            newlines = match.group(0).count('\n')
            return '\n' * newlines
        text = re.sub(r'/\*.*?\*/', replace_block, text, flags=re.DOTALL)
        text = re.sub(r'//.*$', '', text, flags=re.MULTILINE)
        return text

    def scan_all(self):
        header_files = list(self.source_dir.rglob("*.h"))
        cpp_files = list(self.source_dir.rglob("*.cpp"))

        for h_file in header_files:
            self.audit_header(h_file)

        for cpp_file in cpp_files:
            self.audit_cpp(cpp_file)

    def audit_header(self, h_file: Path):
        try:
            content = h_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        clean_content = self.strip_comments(content)

        # Find classes and their replicated UPROPERTY variables
        class_pattern = re.compile(r'class\s+\w+_API\s+(?P<classname>[A-Z]\w+)[^{]*\{(?P<body>.*?)\};', re.DOTALL)
        for class_match in class_pattern.finditer(clean_content):
            class_body = class_match.group("body")
            class_start = class_match.start()

            # Find all UPROPERTY declarations
            prop_pattern = re.compile(
                r'UPROPERTY\s*\((?P<specs>[^)]*)\)\s*(?P<type>[A-Za-z0-9_<>\s*&:,]+?)\s+(?P<varname>\w+)\s*;',
                re.DOTALL
            )

            for prop_match in prop_pattern.finditer(class_body):
                specs = prop_match.group("specs")
                prop_type = prop_match.group("type").strip()
                var_name = prop_match.group("varname")
                prop_pos = class_start + prop_match.start()
                line_no = content[:prop_pos].count('\n') + 1

                is_replicated = bool(re.search(r'\b(Replicated|ReplicatedUsing)\b', specs, re.IGNORECASE))
                if not is_replicated:
                    continue

                # Check 1: Raw TArray Replication without FastArraySerializer
                if re.search(r'\bTArray\s*<', prop_type):
                    self.add_issue(
                        file_path=h_file,
                        line_number=line_no,
                        severity="ERROR",
                        description=f"Raw dynamic array '{prop_type} {var_name}' is marked Replicated. Raw TArray replication resends the entire array upon modification.",
                        recommendation="Wrap the array in a struct inheriting from FFastArraySerializer / FFastArraySerializerItem with NetDeltaSerialize."
                    )

                # Check 2: Heavy types (FString, TMap, TSet)
                for heavy_pat in HEAVY_UNREPLICATED_TYPES:
                    if re.search(heavy_pat, prop_type):
                        self.add_issue(
                            file_path=h_file,
                            line_number=line_no,
                            severity="WARN",
                            description=f"Heavy type '{prop_type} {var_name}' marked for network replication. Replicating strings/maps/sets consumes high bandwidth.",
                            recommendation="Consider using FName/uint8 identifiers, Gameplay Tags, or Fast Array Serialization instead of raw dynamic containers."
                        )

    def audit_cpp(self, cpp_file: Path):
        try:
            content = cpp_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        clean_content = self.strip_comments(content)
        lines = content.splitlines()

        # Check 3: Raw DOREPLIFETIME instead of DOREPLIFETIME_CONDITION
        # Find raw DOREPLIFETIME(Class, Property) calls (not DOREPLIFETIME_CONDITION or DOREPLIFETIME_CONDITION_NOTIFY)
        raw_dorep_pattern = re.compile(r'\bDOREPLIFETIME\s*\(\s*(?P<class>\w+)\s*,\s*(?P<prop>\w+)\s*\)')
        for match in raw_dorep_pattern.finditer(clean_content):
            start_pos = match.start()
            line_no = content[:start_pos].count('\n') + 1
            class_name = match.group("class")
            prop_name = match.group("prop")

            self.add_issue(
                file_path=cpp_file,
                line_number=line_no,
                severity="WARN",
                description=f"Unconditional replication used: 'DOREPLIFETIME({class_name}, {prop_name})'. Properties replicated to all connections without conditional filtering waste network bandwidth.",
                recommendation=f"Use DOREPLIFETIME_CONDITION({class_name}, {prop_name}, COND_OwnerOnly/COND_SkipOwner/COND_SimulatedOnly) to restrict replication scope."
            )

def run_bandwidth_audit(source_path: Path) -> dict:
    auditor = BandwidthAuditor(source_path)
    auditor.scan_all()

    error_count = sum(1 for i in auditor.issues if i["severity"] == "ERROR")
    warn_count = sum(1 for i in auditor.issues if i["severity"] == "WARN")

    if error_count > 0:
        status = "FAIL"
    elif warn_count > 0:
        status = "WARN"
    else:
        status = "PASS"

    return {
        "status": status,
        "total_issues": len(auditor.issues),
        "error_count": error_count,
        "warning_count": warn_count,
        "issues": auditor.issues,
        "summary": f"Bandwidth audit {status.lower()}: {error_count} critical issue(s), {warn_count} warning(s)."
    }

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Bandwidth & Network Performance Static Audit")
    parser.add_argument("--source", type=str, default="Source", help="Path to Source directory (default: Source)")
    parser.add_argument("--json", action="store_true", default=True, help="Emit output as JSON (default: True)")
    parser.add_argument("--pretty", action="store_true", help="Format JSON with indentation")

    args = parser.parse_args()

    source_dir = Path(args.source)
    if not source_dir.is_absolute():
        source_dir = Path.cwd() / source_dir

    if not source_dir.exists():
        result = {
            "status": "FAIL",
            "total_issues": 1,
            "error_count": 1,
            "warning_count": 0,
            "issues": [{
                "file": str(source_dir),
                "line": 0,
                "severity": "ERROR",
                "description": f"Source directory '{source_dir}' does not exist.",
                "recommendation": "Check the specified source path."
            }],
            "summary": "Failed: Source directory not found."
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    result = run_bandwidth_audit(source_dir)
    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] in ["PASS", "WARN"] else 1)

if __name__ == "__main__":
    main()
