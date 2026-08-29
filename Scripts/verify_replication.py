#!/usr/bin/env python3
"""
Unreal Engine 5.8 Multiplayer & Replication Static Verification Script
Scans C++ headers and source files in Source/ to identify multiplayer/replication anti-patterns and violations.
Emits structured JSON diagnostics for autonomous agent consumption.
"""

import sys
import os
import re
import json
import argparse
from pathlib import Path

# UI headers forbidden in server/gameplay headers
FORBIDDEN_UI_HEADERS = [
    r"Blueprint/UserWidget\.h",
    r"Blueprint/WidgetTree\.h",
    r"UMG\.h",
    r"Components/Widget\.h",
    r"SlateBasics\.h",
    r"SlateCore\.h",
    r"SlateOptMacros\.h",
]

class ReplicationVerifier:
    def __init__(self, source_dir: Path):
        self.source_dir = source_dir
        self.violations = []

    def add_violation(self, rule_id: str, file_path: Path, line_number: int, severity: str, message: str):
        rel_path = file_path.relative_to(self.source_dir.parent).as_posix() if self.source_dir.parent in file_path.parents else file_path.as_posix()
        self.violations.append({
            "rule": rule_id,
            "file": rel_path,
            "line": line_number,
            "severity": severity,
            "message": message
        })

    def strip_comments(self, text: str) -> str:
        # Replace block comments with spaces to preserve line count
        def replace_block(match):
            newlines = match.group(0).count('\n')
            return '\n' * newlines
        text = re.sub(r'/\*.*?\*/', replace_block, text, flags=re.DOTALL)
        # Replace line comments
        text = re.sub(r'//.*$', '', text, flags=re.MULTILINE)
        return text

    def scan_all(self):
        header_files = list(self.source_dir.rglob("*.h"))
        cpp_files = list(self.source_dir.rglob("*.cpp"))

        # Map classes to their headers and cpp implementations
        for h_file in header_files:
            self.verify_header_file(h_file)

        for cpp_file in cpp_files:
            self.verify_cpp_file(cpp_file)

    def verify_header_file(self, h_file: Path):
        try:
            content = h_file.read_text(encoding="utf-8", errors="replace")
        except Exception as ex:
            return

        lines = content.splitlines()
        clean_content = self.strip_comments(content)

        # Check 1: Forbidden UI Headers in Actor/Gameplay Headers
        is_actor_or_gameplay_header = bool(re.search(r'class\s+\w+_API\s+([A-Z]\w+)\s*:\s*public\s+(AActor|ACharacter|APawn|AGameMode|AGameModeBase|AGameState|AGameStateBase|APlayerState|AAIController|UActorComponent)', clean_content))
        
        if is_actor_or_gameplay_header:
            for idx, line in enumerate(lines, start=1):
                clean_line = line.split('//')[0]
                for ui_hdr_pat in FORBIDDEN_UI_HEADERS:
                    if re.search(rf'#include\s*["<]{ui_hdr_pat}[">]', clean_line):
                        self.add_violation(
                            rule_id="REPL_UI_HEADER_IN_SERVER_ACTOR",
                            file_path=h_file,
                            line_number=idx,
                            severity="ERROR",
                            message=f"Forbidden UI/Widget header included in gameplay actor header: '{line.strip()}'. UI headers should only be in UI components or included in .cpp."
                        )

        # Check 2: Server RPC missing WithValidation
        # Regex to find UFUNCTION(...) declarations
        ufunc_pattern = re.compile(r'UFUNCTION\s*\(([^)]*)\)', re.DOTALL)
        for match in ufunc_pattern.finditer(clean_content):
            specifiers = match.group(1)
            # Find line number
            start_pos = match.start()
            line_no = content[:start_pos].count('\n') + 1

            if re.search(r'\bServer\b', specifiers, re.IGNORECASE):
                if not re.search(r'\bWithValidation\b', specifiers, re.IGNORECASE):
                    self.add_violation(
                        rule_id="REPL_SERVER_RPC_MISSING_VALIDATION",
                        file_path=h_file,
                        line_number=line_no,
                        severity="ERROR",
                        message="Server RPC declared without 'WithValidation'. All Server RPCs must declare WithValidation and implement _Validate()."
                    )

        # Check 3: RepNotify naming conventions & Replicated without GetLifetimeReplicatedProps
        # Match class definitions
        class_pattern = re.compile(r'class\s+\w+_API\s+(?P<classname>[A-Z]\w+)[^{]*\{(?P<body>.*?)\};', re.DOTALL)
        for class_match in class_pattern.finditer(clean_content):
            class_name = class_match.group("classname")
            class_body = class_match.group("body")
            class_start_pos = class_match.start()
            class_line_no = content[:class_start_pos].count('\n') + 1

            has_replicated_props = False
            
            # Search for UPROPERTY with Replicated or ReplicatedUsing
            uprop_pattern = re.compile(r'UPROPERTY\s*\(([^)]*)\)\s*(?:[A-Za-z0-9_<>\s*&]+?)\s+(?P<varname>\w+)\s*;', re.DOTALL)
            for prop_match in uprop_pattern.finditer(class_body):
                prop_specs = prop_match.group(1)
                var_name = prop_match.group("varname")
                prop_pos = class_start_pos + prop_match.start()
                prop_line_no = content[:prop_pos].count('\n') + 1

                is_replicated = bool(re.search(r'\bReplicated\b', prop_specs, re.IGNORECASE))
                rep_using_match = re.search(r'ReplicatedUsing\s*=\s*(?P<funcname>\w+)', prop_specs, re.IGNORECASE)

                if is_replicated or rep_using_match:
                    has_replicated_props = True

                if rep_using_match:
                    func_name = rep_using_match.group("funcname")
                    expected_name = f"OnRep_{var_name}"
                    if func_name != expected_name:
                        self.add_violation(
                            rule_id="REPL_INVALID_REPNOTIFY_NAME",
                            file_path=h_file,
                            line_number=prop_line_no,
                            severity="WARNING",
                            message=f"Replication callback '{func_name}' for variable '{var_name}' does not match standard convention '{expected_name}'."
                        )

            # If class has replicated properties, check if GetLifetimeReplicatedProps is declared in header
            if has_replicated_props:
                has_getlifetime_decl = bool(re.search(r'\bGetLifetimeReplicatedProps\b', class_body))
                if not has_getlifetime_decl:
                    self.add_violation(
                        rule_id="REPL_MISSING_LIFETIME_PROPS_DECL",
                        file_path=h_file,
                        line_number=class_line_no,
                        severity="ERROR",
                        message=f"Class '{class_name}' defines replicated properties but does not declare 'virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;'."
                    )

    def verify_cpp_file(self, cpp_file: Path):
        try:
            content = cpp_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        clean_content = self.strip_comments(content)
        lines = content.splitlines()

        has_doreplifetime = bool(re.search(r'\bDOREPLIFETIME\b|\bDOREPLIFETIME_CONDITION\b', clean_content))
        has_getlifetime_def = bool(re.search(r'::GetLifetimeReplicatedProps\s*\(', clean_content))

        # Check 4: Missing #include "Net/UnrealNetwork.h" in .cpp
        if has_doreplifetime or has_getlifetime_def:
            has_net_include = bool(re.search(r'#include\s*["<]Net/UnrealNetwork\.h[">]', clean_content))
            if not has_net_include:
                self.add_violation(
                    rule_id="REPL_MISSING_NET_INCLUDE",
                    file_path=cpp_file,
                    line_number=1,
                    severity="ERROR",
                    message="Source file uses DOREPLIFETIME or implements GetLifetimeReplicatedProps but is missing '#include \"Net/UnrealNetwork.h\"'."
                )

        # Check 5: Calling Super::GetLifetimeReplicatedProps
        if has_getlifetime_def:
            func_body_match = re.search(r'::GetLifetimeReplicatedProps\s*\([^)]*\)\s*(?:const)?\s*\{(.*?)\}', clean_content, re.DOTALL)
            if func_body_match:
                body = func_body_match.group(1)
                if not re.search(r'Super::GetLifetimeReplicatedProps\s*\(', body):
                    start_pos = func_body_match.start()
                    line_no = content[:start_pos].count('\n') + 1
                    self.add_violation(
                        rule_id="REPL_MISSING_SUPER_LIFETIME_CALL",
                        file_path=cpp_file,
                        line_number=line_no,
                        severity="ERROR",
                        message="GetLifetimeReplicatedProps implementation does not call 'Super::GetLifetimeReplicatedProps(OutLifetimeProps)'."
                    )

        # Check 6: Actor / Component Constructor bReplicates Check
        # Check if constructor enables replication when DOREPLIFETIME is used in the same class
        constructor_matches = re.finditer(r'([A-Z]\w+)::\1\s*\([^)]*\)\s*(?::\s*[^\{]+)?\{(.*?)\}', clean_content, re.DOTALL)
        for ctor in constructor_matches:
            class_name = ctor.group(1)
            ctor_body = ctor.group(2)
            ctor_pos = ctor.start()
            ctor_line_no = content[:ctor_pos].count('\n') + 1

            # If class has replicated props or RPCs, check if replication is enabled
            class_has_replication = bool(re.search(rf'{class_name}::GetLifetimeReplicatedProps', clean_content))
            if class_has_replication:
                if class_name.startswith('A'): # Actor
                    has_reps_flag = bool(re.search(r'bReplicates\s*=\s*true|SetReplicates\s*\(\s*true\s*\)', ctor_body))
                    if not has_reps_flag:
                        self.add_violation(
                            rule_id="REPL_ACTOR_CTOR_MISSING_BREPLICATES",
                            file_path=cpp_file,
                            line_number=ctor_line_no,
                            severity="WARNING",
                            message=f"Actor constructor '{class_name}::{class_name}' does not explicitly set 'bReplicates = true;' or 'SetReplicates(true)' despite replicating properties."
                        )
                elif class_name.startswith('U') and class_name.endswith('Component'): # Actor Component
                    has_comp_rep = bool(re.search(r'SetIsReplicatedByDefault\s*\(\s*true\s*\)|bReplicates\s*=\s*true', ctor_body))
                    if not has_comp_rep:
                        self.add_violation(
                            rule_id="REPL_COMPONENT_CTOR_MISSING_REPLICATION",
                            file_path=cpp_file,
                            line_number=ctor_line_no,
                            severity="WARNING",
                            message=f"Component constructor '{class_name}::{class_name}' does not set 'SetIsReplicatedByDefault(true)' despite replicating properties."
                        )

def run_verification(source_path: Path) -> dict:
    verifier = ReplicationVerifier(source_path)
    verifier.scan_all()
    
    error_count = sum(1 for v in verifier.violations if v["severity"] == "ERROR")
    warning_count = sum(1 for v in verifier.violations if v["severity"] == "WARNING")
    
    status = "PASSED" if error_count == 0 else "FAILED"
    
    return {
        "status": status,
        "total_violations": len(verifier.violations),
        "error_count": error_count,
        "warning_count": warning_count,
        "violations": verifier.violations,
        "summary": f"Replication analysis {status.lower()}: {error_count} error(s), {warning_count} warning(s)."
    }

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Static Replication Verifier")
    parser.add_argument("--source", type=str, default="Source", help="Path to Source directory (default: Source)")
    parser.add_argument("--json", action="store_true", default=True, help="Emit output as JSON (default: True)")
    parser.add_argument("--pretty", action="store_true", help="Pretty print JSON")

    args = parser.parse_args()

    source_dir = Path(args.source)
    if not source_dir.is_absolute():
        source_dir = Path.cwd() / source_dir

    if not source_dir.exists():
        result = {
            "status": "FAILED",
            "total_violations": 1,
            "error_count": 1,
            "warning_count": 0,
            "violations": [{
                "rule": "SOURCE_DIR_NOT_FOUND",
                "file": str(source_dir),
                "line": 0,
                "severity": "ERROR",
                "message": f"Source directory '{source_dir}' does not exist."
            }],
            "summary": "Failed: Source directory not found."
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    result = run_verification(source_dir)
    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "PASSED" else 1)

if __name__ == "__main__":
    main()
