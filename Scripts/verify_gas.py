#!/usr/bin/env python3
"""
Unreal Engine 5.8 Gameplay Ability System (GAS) Static Verification Script
Statically inspects C++ headers and source files in Source/ for GAS compliance:
- Verifies UAttributeSet subclasses have ATTRIBUTE_ACCESSORS macro for every FGameplayAttributeData.
- Verifies matching OnRep_PropertyName callbacks and definitions using GAMEPLAYATTRIBUTE_REPNOTIFY.
- Verifies registration in GetLifetimeReplicatedProps with DOREPLIFETIME_CONDITION_NOTIFY(..., REPNOTIFY_Always).
- Flags direct UI/HUD/Widget references in UGameplayAbility subclasses.
Emits structured JSON diagnostics for autonomous agent consumption.
"""

import sys
import os
import re
import json
import argparse
from pathlib import Path

FORBIDDEN_GAS_UI_HEADERS = [
    r"Blueprint/UserWidget\.h",
    r"Blueprint/WidgetTree\.h",
    r"UMG\.h",
    r"Components/Widget\.h",
    r"SlateBasics\.h",
    r"SlateCore\.h",
]

class GASVerifier:
    def __init__(self, source_dir: Path):
        self.source_dir = source_dir
        self.violations = []

    def add_violation(self, rule_id: str, file_path: Path, line_number: int, severity: str, detail: str):
        try:
            rel_path = file_path.relative_to(self.source_dir.parent).as_posix() if self.source_dir.parent in file_path.parents else file_path.as_posix()
        except Exception:
            rel_path = file_path.as_posix()
        self.violations.append({
            "rule": rule_id,
            "file": rel_path,
            "line": line_number,
            "severity": severity,
            "detail": detail
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

        attribute_set_headers = {}
        for h_file in header_files:
            attrs = self.verify_header(h_file)
            if attrs:
                attribute_set_headers[h_file.stem] = (h_file, attrs)

        for cpp_file in cpp_files:
            self.verify_cpp(cpp_file, attribute_set_headers)

    def verify_header(self, h_file: Path) -> dict[str, list[dict]]:
        try:
            content = h_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return {}

        clean_content = self.strip_comments(content)
        lines = content.splitlines()

        # Check UGameplayAbility UI header leak
        is_ability_header = bool(re.search(r'class\s+\w+_API\s+([A-Z]\w+)\s*:\s*public\s+UGameplayAbility', clean_content))
        if is_ability_header:
            for idx, line in enumerate(lines, start=1):
                clean_line = line.split('//')[0]
                for ui_hdr in FORBIDDEN_GAS_UI_HEADERS:
                    if re.search(rf'#include\s*["<]{ui_hdr}[">]', clean_line):
                        self.add_violation(
                            rule_id="GAS_UI_HEADER_IN_ABILITY",
                            file_path=h_file,
                            line_number=idx,
                            severity="ERROR",
                            detail=f"Forbidden UI/Widget header '{line.strip()}' included in UGameplayAbility header. Gameplay abilities must be decoupled from Slate/UMG."
                        )

        # Detect UAttributeSet subclasses
        attr_class_pattern = re.compile(r'class\s+\w+_API\s+(?P<classname>[A-Z]\w+)\s*:\s*public\s+UAttributeSet[^{]*\{(?P<body>.*?)\};', re.DOTALL)
        discovered_sets = {}

        for match in attr_class_pattern.finditer(clean_content):
            class_name = match.group("classname")
            class_body = match.group("body")
            class_start = match.start()
            class_line_no = content[:class_start].count('\n') + 1

            # Check macro definition presence in file
            has_accessor_macro_def = bool(re.search(r'#define\s+ATTRIBUTE_ACCESSORS', content))
            if not has_accessor_macro_def:
                self.add_violation(
                    rule_id="GAS_MISSING_ACCESSOR_MACRO_DEF",
                    file_path=h_file,
                    line_number=class_line_no,
                    severity="WARNING",
                    detail=f"Header declaring UAttributeSet '{class_name}' does not define ATTRIBUTE_ACCESSORS macro."
                )

            # Find all FGameplayAttributeData properties
            attr_pattern = re.compile(
                r'UPROPERTY\s*\((?P<propspecs>[^)]*)\)\s*(?:mutable\s+)?FGameplayAttributeData\s+(?P<attrname>\w+)\s*;',
                re.DOTALL
            )

            discovered_attrs = []
            for attr_match in attr_pattern.finditer(class_body):
                attr_name = attr_match.group("attrname")
                prop_specs = attr_match.group("propspecs")
                attr_pos = class_start + attr_match.start()
                attr_line_no = content[:attr_pos].count('\n') + 1

                discovered_attrs.append({
                    "name": attr_name,
                    "line": attr_line_no,
                    "class": class_name
                })

                # Check 1: ATTRIBUTE_ACCESSORS(ClassName, PropertyName) presence
                accessor_pattern = rf'ATTRIBUTE_ACCESSORS\s*\(\s*{class_name}\s*,\s*{attr_name}\s*\)'
                if not re.search(accessor_pattern, class_body):
                    self.add_violation(
                        rule_id="GAS_MISSING_ATTRIBUTE_ACCESSORS",
                        file_path=h_file,
                        line_number=attr_line_no,
                        severity="ERROR",
                        detail=f"Attribute '{attr_name}' in '{class_name}' is missing ATTRIBUTE_ACCESSORS({class_name}, {attr_name}) macro declaration."
                    )

                # Check 2: ReplicatedUsing = OnRep_PropertyName
                rep_using_match = re.search(r'ReplicatedUsing\s*=\s*(?P<repfunc>\w+)', prop_specs)
                if not rep_using_match:
                    self.add_violation(
                        rule_id="GAS_ATTRIBUTE_MISSING_REPLICATED_USING",
                        file_path=h_file,
                        line_number=attr_line_no,
                        severity="ERROR",
                        detail=f"Attribute '{attr_name}' in '{class_name}' must be marked with ReplicatedUsing = OnRep_{attr_name}."
                    )
                else:
                    rep_func = rep_using_match.group("repfunc")
                    expected_rep = f"OnRep_{attr_name}"
                    if rep_func != expected_rep:
                        self.add_violation(
                            rule_id="GAS_INVALID_REPNOTIFY_NAME",
                            file_path=h_file,
                            line_number=attr_line_no,
                            severity="WARNING",
                            detail=f"Attribute '{attr_name}' uses RepNotify callback '{rep_func}' instead of expected '{expected_rep}'."
                        )

                    # Check 3: Declaration of OnRep_PropertyName(const FGameplayAttributeData& OldValue)
                    onrep_decl_pat = rf'void\s+{rep_func}\s*\(\s*const\s+FGameplayAttributeData\s*&\s*\w*\s*\)'
                    if not re.search(onrep_decl_pat, class_body):
                        self.add_violation(
                            rule_id="GAS_MISSING_ONREP_DECL",
                            file_path=h_file,
                            line_number=attr_line_no,
                            severity="ERROR",
                            detail=f"Missing callback declaration 'void {rep_func}(const FGameplayAttributeData& Old{attr_name})' in '{class_name}'."
                        )

            if discovered_attrs:
                discovered_sets[class_name] = discovered_attrs

        return discovered_sets

    def verify_cpp(self, cpp_file: Path, attribute_sets: dict):
        try:
            content = cpp_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        clean_content = self.strip_comments(content)

        # Match OnRep implementations and GetLifetimeReplicatedProps
        for file_stem, (h_path, classes_dict) in attribute_sets.items():
            for class_name, attrs in classes_dict.items():
                if class_name not in clean_content:
                    continue

                for attr in attrs:
                    attr_name = attr["name"]
                # Verify GAMEPLAYATTRIBUTE_REPNOTIFY inside OnRep
                onrep_func_pat = rf'{class_name}::OnRep_{attr_name}\s*\([^)]*\)\s*\{{(?P<funcbody>.*?)\}}'
                onrep_match = re.search(onrep_func_pat, clean_content, re.DOTALL)
                if onrep_match:
                    func_body = onrep_match.group("funcbody")
                    if "GAMEPLAYATTRIBUTE_REPNOTIFY" not in func_body:
                        start_pos = onrep_match.start()
                        line_no = content[:start_pos].count('\n') + 1
                        self.add_violation(
                            rule_id="GAS_MISSING_GAMEPLAYATTRIBUTE_REPNOTIFY",
                            file_path=cpp_file,
                            line_number=line_no,
                            severity="ERROR",
                            detail=f"Function '{class_name}::OnRep_{attr_name}' does not invoke 'GAMEPLAYATTRIBUTE_REPNOTIFY({class_name}, {attr_name}, Old{attr_name})'."
                        )

                # Verify DOREPLIFETIME_CONDITION_NOTIFY in GetLifetimeReplicatedProps
                getlifetime_pat = rf'{class_name}::GetLifetimeReplicatedProps\s*\([^)]*\)\s*(?:const)?\s*\{{(?P<funcbody>.*?)\}}'
                getlifetime_match = re.search(getlifetime_pat, clean_content, re.DOTALL)
                if getlifetime_match:
                    gl_body = getlifetime_match.group("funcbody")
                    dorep_pat = rf'DOREPLIFETIME_CONDITION_NOTIFY\s*\(\s*{class_name}\s*,\s*{attr_name}\s*,\s*COND_\w+\s*,\s*REPNOTIFY_Always\s*\)'
                    if not re.search(dorep_pat, gl_body):
                        start_pos = getlifetime_match.start()
                        line_no = content[:start_pos].count('\n') + 1
                        self.add_violation(
                            rule_id="GAS_MISSING_DOREPLIFETIME_NOTIFY",
                            file_path=cpp_file,
                            line_number=line_no,
                            severity="ERROR",
                            detail=f"Attribute '{class_name}::{attr_name}' must be registered via DOREPLIFETIME_CONDITION_NOTIFY({class_name}, {attr_name}, COND_None, REPNOTIFY_Always) in GetLifetimeReplicatedProps."
                        )

def run_gas_verification(source_path: Path) -> dict:
    verifier = GASVerifier(source_path)
    verifier.scan_all()

    error_count = sum(1 for v in verifier.violations if v["severity"] == "ERROR")
    warning_count = sum(1 for v in verifier.violations if v["severity"] == "WARNING")
    status = "PASS" if error_count == 0 else "FAIL"

    return {
        "status": status,
        "total_violations": len(verifier.violations),
        "error_count": error_count,
        "warning_count": warning_count,
        "violations": verifier.violations,
        "summary": f"GAS Static Verification {status.lower()}ed: {error_count} error(s), {warning_count} warning(s)."
    }

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 GAS Static Verification")
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
            "total_violations": 1,
            "error_count": 1,
            "warning_count": 0,
            "violations": [{
                "rule": "SOURCE_DIR_NOT_FOUND",
                "file": str(source_dir),
                "line": 0,
                "severity": "ERROR",
                "detail": f"Source directory '{source_dir}' does not exist."
            }],
            "summary": "Failed: Source directory not found."
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    result = run_gas_verification(source_dir)
    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "PASS" else 1)

if __name__ == "__main__":
    main()
