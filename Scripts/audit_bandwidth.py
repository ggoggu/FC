#!/usr/bin/env python3
"""
Unreal Engine 5.8 Bandwidth & Network Performance Static Audit Script
Scans C++ headers and source files in Source/ using balanced brace & paren parsing:
- Flags UPROPERTY(Replicated) on raw TArray<...> without FastArraySerializer wrappers (ERROR).
- Flags raw unconditional DOREPLIFETIME(Class, Prop) without DOREPLIFETIME_CONDITION (ERROR).
- Flags heavy strings/maps/sets marked Replicated without delta serialization (WARN).
"""

import sys
import re
import json
import argparse
from pathlib import Path

HEAVY_UNREPLICATED_TYPES = [
    r"\bFString\b",
    r"\bTMap\s*<",
    r"\bTSet\s*<",
]


def resolve_target_files(source_dir: Path, target_file: str | None) -> tuple[list[Path], list[Path]]:
    """Resolves header and cpp files to scan, pairing .h and .cpp when --file is specified."""
    if not target_file:
        return list(source_dir.rglob("*.h")), list(source_dir.rglob("*.cpp"))

    p = Path(target_file)
    if not p.is_absolute():
        p = Path.cwd() / p
    p = p.resolve()

    stem = p.stem
    header_files = [h for h in source_dir.rglob(f"{stem}.h")]
    cpp_files = [c for c in source_dir.rglob(f"{stem}.cpp")]

    if p.exists():
        if p.suffix == ".h" and p not in header_files:
            header_files.append(p)
        elif p.suffix == ".cpp" and p not in cpp_files:
            cpp_files.append(p)

    return header_files, cpp_files


class BandwidthAuditor:
    def __init__(self, source_dir: Path, target_file: str | None = None):
        self.source_dir = source_dir
        self.target_file = target_file
        self.issues = []

    def add_issue(
        self,
        rule_id: str,
        file_path: Path,
        line_number: int,
        severity: str,
        description: str,
        recommendation: str,
        ref: str = ".antigravity/rules/05-combat-and-inventory-networking.md",
    ):
        try:
            rel_path = (
                file_path.relative_to(self.source_dir.parent).as_posix()
                if self.source_dir.parent in file_path.parents
                else file_path.as_posix()
            )
        except Exception:
            rel_path = file_path.as_posix()
        self.issues.append(
            {
                "rule": rule_id,
                "file": rel_path,
                "line": line_number,
                "severity": severity,
                "description": description,
                "recommendation": recommendation,
                "ref": ref,
            }
        )

    def strip_comments(self, text: str) -> str:
        def replace_comment(match: re.Match) -> str:
            s = match.group(0)
            if s.startswith('"') or s.startswith("'"):
                return s
            return "".join("\n" if ch == "\n" else " " for ch in s)

        pattern = re.compile(
            r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*',
            re.DOTALL,
        )
        return pattern.sub(replace_comment, text)

    def extract_delimited_block(self, text: str, open_pos: int, open_ch: str, close_ch: str) -> tuple[str, int]:
        if open_pos < 0 or open_pos >= len(text) or text[open_pos] != open_ch:
            return "", open_pos
        depth = 0
        i = open_pos
        n = len(text)
        in_string = False
        in_char = False
        while i < n:
            ch = text[i]
            if in_string:
                if ch == "\\":
                    i += 2
                    continue
                if ch == '"':
                    in_string = False
            elif in_char:
                if ch == "\\":
                    i += 2
                    continue
                if ch == "'":
                    in_char = False
            else:
                if ch == '"':
                    in_string = True
                elif ch == "'":
                    in_char = True
                elif ch == open_ch:
                    depth += 1
                elif ch == close_ch:
                    depth -= 1
                    if depth == 0:
                        return text[open_pos + 1 : i], i + 1
            i += 1
        return text[open_pos + 1 :], n

    def extract_brace_block(self, text: str, open_pos: int) -> tuple[str, int]:
        return self.extract_delimited_block(text, open_pos, "{", "}")

    def extract_paren_block(self, text: str, open_pos: int) -> tuple[str, int]:
        return self.extract_delimited_block(text, open_pos, "(", ")")

    def scan_all(self):
        header_files, cpp_files = resolve_target_files(self.source_dir, self.target_file)

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
        cls_header_pat = re.compile(r"\bclass\s+(?:\w+_API\s+)?(?P<classname>[A-Z]\w+)(?:\s+final)?[^{;]*\{")

        pos = 0
        while pos < len(clean_content):
            class_match = cls_header_pat.search(clean_content, pos)
            if not class_match:
                break
            open_brace_pos = class_match.end() - 1
            class_body, end_pos = self.extract_brace_block(clean_content, open_brace_pos)
            body_start = open_brace_pos + 1
            pos = end_pos

            for uprop_kw in re.finditer(r"\bUPROPERTY\s*\(", class_body):
                specs, after_paren = self.extract_paren_block(class_body, uprop_kw.end() - 1)
                decl_match = re.match(
                    r"\s*(?P<type>[A-Za-z0-9_<>\s*&:,]+?)\s+(?P<varname>\w+)\s*(?:=\s*[^;]+)?;",
                    class_body[after_paren:],
                    re.DOTALL,
                )
                if not decl_match:
                    continue

                prop_type = re.sub(r"\s+", " ", decl_match.group("type").strip())
                var_name = decl_match.group("varname")
                prop_pos = body_start + uprop_kw.start()
                line_no = content[:prop_pos].count("\n") + 1

                if not re.search(r"\b(Replicated|ReplicatedUsing)\b", specs, re.IGNORECASE):
                    continue

                if re.search(r"\bTArray\s*<", prop_type):
                    self.add_issue(
                        rule_id="BW_RAW_TARRAY_REPLICATED",
                        file_path=h_file,
                        line_number=line_no,
                        severity="ERROR",
                        description=f"Raw dynamic array '{prop_type} {var_name}' is marked Replicated (resends entire array on every mutation).",
                        recommendation="Wrap the array in a struct inheriting from FFastArraySerializer / FFastArraySerializerItem with NetDeltaSerialize.",
                        ref=".antigravity/rules/05-combat-and-inventory-networking.md#1",
                    )

                for heavy_pat in HEAVY_UNREPLICATED_TYPES:
                    if re.search(heavy_pat, prop_type):
                        self.add_issue(
                            rule_id="BW_HEAVY_TYPE_REPLICATED",
                            file_path=h_file,
                            line_number=line_no,
                            severity="WARN",
                            description=f"Heavy type '{prop_type} {var_name}' marked for network replication.",
                            recommendation="Prefer FName/uint8 identifiers, GameplayTags, or FastArraySerializer over replicated FString/TMap/TSet.",
                            ref=".antigravity/rules/05-combat-and-inventory-networking.md#1",
                        )

    def audit_cpp(self, cpp_file: Path):
        try:
            content = cpp_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        clean_content = self.strip_comments(content)

        raw_dorep_pattern = re.compile(r"\bDOREPLIFETIME\s*\(\s*(?P<class>\w+)\s*,\s*(?P<prop>\w+)\s*\)")
        for match in raw_dorep_pattern.finditer(clean_content):
            line_no = content[: match.start()].count("\n") + 1
            class_name = match.group("class")
            prop_name = match.group("prop")
            self.add_issue(
                rule_id="BW_RAW_DOREPLIFETIME",
                file_path=cpp_file,
                line_number=line_no,
                severity="ERROR",
                description=f"Unconditional 'DOREPLIFETIME({class_name}, {prop_name})' used without explicit replication condition.",
                recommendation=f"Use DOREPLIFETIME_CONDITION({class_name}, {prop_name}, COND_OwnerOnly/COND_SimulatedOnly/COND_InitialOnly/COND_None).",
                ref=".antigravity/rules/02-multiplayer-conventions.md#1",
            )


def run_bandwidth_audit(source_path: Path, target_file: str | None = None) -> dict:
    auditor = BandwidthAuditor(source_path, target_file=target_file)
    auditor.scan_all()

    error_count = sum(1 for i in auditor.issues if i["severity"] == "ERROR")
    warn_count = sum(1 for i in auditor.issues if i["severity"] in ("WARN", "WARNING"))

    if error_count > 0:
        status = "FAIL"
    elif warn_count > 0:
        status = "WARN"
    else:
        status = "PASS"

    return {
        "status": status,
        "scoped_file": target_file,
        "total_issues": len(auditor.issues),
        "error_count": error_count,
        "warning_count": warn_count,
        "issues": auditor.issues,
        "summary": f"Bandwidth audit {status.lower()}: {error_count} critical issue(s), {warn_count} warning(s).",
    }


def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Bandwidth & Network Performance Static Audit")
    parser.add_argument("--source", type=str, default="Source", help="Path to Source directory (default: Source)")
    parser.add_argument("--file", "-f", type=str, default=None, help="Optional single file path for incremental ACI check")
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
            "issues": [
                {
                    "rule": "SOURCE_DIR_NOT_FOUND",
                    "file": str(source_dir),
                    "line": 0,
                    "severity": "ERROR",
                    "description": f"Source directory '{source_dir}' does not exist.",
                    "recommendation": "Check the specified source path.",
                }
            ],
            "summary": "Failed: Source directory not found.",
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    result = run_bandwidth_audit(source_dir, target_file=args.file)
    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] in ["PASS", "WARN"] else 1)


if __name__ == "__main__":
    main()
