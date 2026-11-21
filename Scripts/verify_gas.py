#!/usr/bin/env python3
"""
Unreal Engine 5.8 Gameplay Ability System (GAS) Static Verification Script
Statically inspects C++ headers and source files in Source/ for GAS compliance using
balanced brace and parenthesis parsing:
- Verifies UAttributeSet subclasses have ATTRIBUTE_ACCESSORS macro for every FGameplayAttributeData.
- Supports un-replicated Meta-Attributes (Rule 04 §4.5: NotReplicated, Meta category, or Damage/Incoming* names).
- Verifies replicated attributes have matching OnRep_PropertyName callbacks using GAMEPLAYATTRIBUTE_REPNOTIFY.
- Verifies registration in GetLifetimeReplicatedProps with DOREPLIFETIME_CONDITION_NOTIFY(..., REPNOTIFY_Always).
- Verifies PreAttributeChange sanitizes NaN/Inf.
- Flags direct UI/HUD/Widget/ViewModel references in GAS headers/sources.
"""

import sys
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
    r"MVVMViewModelBase\.h",
]

META_ATTRIBUTE_NAMES = {"Damage", "Healing", "IncomingDamage", "IncomingHealing", "IncomingXP"}


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


class GASVerifier:
    def __init__(self, source_dir: Path, target_file: str | None = None):
        self.source_dir = source_dir
        self.target_file = target_file
        self.violations = []

    def add_violation(
        self,
        rule_id: str,
        file_path: Path,
        line_number: int,
        severity: str,
        detail: str,
        fix: str = "",
        ref: str = ".agents/rules/04-gameplay-ability-system.md",
    ):
        try:
            rel_path = (
                file_path.relative_to(self.source_dir.parent).as_posix()
                if self.source_dir.parent in file_path.parents
                else file_path.as_posix()
            )
        except Exception:
            rel_path = file_path.as_posix()
        entry = {
            "rule": rule_id,
            "file": rel_path,
            "line": line_number,
            "severity": severity,
            "detail": detail,
        }
        if fix:
            entry["fix"] = fix
        if ref:
            entry["ref"] = ref
        self.violations.append(entry)

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

    def is_meta_attribute(self, attr_name: str, prop_specs: str) -> bool:
        """Recognizes un-replicated GAS meta-attributes (Rule 04 §4.5)."""
        if re.search(r"\bReplicated(?:Using)?\b", prop_specs, re.IGNORECASE):
            return False
        if re.search(r"\bNotReplicated\b", prop_specs, re.IGNORECASE):
            return True
        if re.search(r'Category\s*=\s*"[^"]*Meta[^"]*"', prop_specs, re.IGNORECASE):
            return True
        if attr_name in META_ATTRIBUTE_NAMES or attr_name.startswith("Incoming"):
            return True
        return False

    def scan_all(self):
        header_files, cpp_files = resolve_target_files(self.source_dir, self.target_file)

        attribute_set_headers = {}
        for h_file in header_files:
            attrs = self.verify_header(h_file)
            if attrs:
                attribute_set_headers[h_file.stem] = (h_file, attrs)

        for cpp_file in cpp_files:
            self.verify_cpp(cpp_file, attribute_set_headers)

        self.verify_gameplay_tags(header_files, cpp_files)

    def verify_gameplay_tags(self, header_files: list[Path], cpp_files: list[Path]):
        """Enforces GameplayTag SSOT: bans raw RequestGameplayTag() and Native/INI duplicates."""
        raw_req_pat = re.compile(r"\bRequestGameplayTag\s*\(")
        for src_file in [*header_files, *cpp_files]:
            try:
                content = src_file.read_text(encoding="utf-8", errors="replace")
            except Exception:
                continue
            clean_content = self.strip_comments(content)
            for m in raw_req_pat.finditer(clean_content):
                line_no = content[: m.start()].count("\n") + 1
                self.add_violation(
                    rule_id="TAG_RAW_REQUEST_CALL",
                    file_path=src_file,
                    line_number=line_no,
                    severity="ERROR",
                    detail="Raw FGameplayTag::RequestGameplayTag() string lookup detected.",
                    fix="Use a native tag symbol from FCGameplayTags:: or UE_DEFINE_GAMEPLAY_TAG_STATIC instead.",
                )

        if self.target_file and not any(Path(self.target_file).suffix == ext for ext in (".cpp", ".ini")):
            return

        project_root = self.source_dir.parent
        config_dir = project_root / "Config"
        if not config_dir.exists():
            return

        ini_files = []
        default_ini = config_dir / "DefaultGameplayTags.ini"
        if default_ini.exists():
            ini_files.append(default_ini)
        tags_dir = config_dir / "Tags"
        if tags_dir.exists():
            ini_files.extend(sorted(tags_dir.glob("*.ini")))
        if not ini_files:
            return

        native_tag_pat = re.compile(
            r'UE_DEFINE_GAMEPLAY_TAG(?:_COMMENT|_STATIC)?\s*\(\s*\w+\s*,\s*"([^"]+)"'
        )
        native_tags: dict[str, Path] = {}
        for cpp_file in self.source_dir.rglob("*.cpp"):
            try:
                content = cpp_file.read_text(encoding="utf-8", errors="replace")
            except Exception:
                continue
            clean_content = self.strip_comments(content)
            for m in native_tag_pat.finditer(clean_content):
                native_tags.setdefault(m.group(1), cpp_file)

        if not native_tags:
            return

        ini_tag_pat = re.compile(r'^\s*\+?GameplayTagList\s*=\s*\(.*?\bTag\s*=\s*"([^"]+)"')
        for ini_file in ini_files:
            try:
                lines = ini_file.read_text(encoding="utf-8", errors="replace").splitlines()
            except Exception:
                continue
            for idx, line in enumerate(lines, start=1):
                stripped = line.strip()
                if not stripped or stripped.startswith(";"):
                    continue
                m = ini_tag_pat.match(line)
                if not m:
                    continue
                tag_name = m.group(1)
                if tag_name in native_tags:
                    native_rel = native_tags[tag_name].name
                    self.add_violation(
                        rule_id="TAG_DUPLICATE_NATIVE_INI",
                        file_path=ini_file,
                        line_number=idx,
                        severity="ERROR",
                        detail=f"GameplayTag '{tag_name}' is duplicated in INI and C++ Native ({native_rel}).",
                        fix="Remove the duplicate entry from INI (keep Native C++ SSOT) or move non-C++ tags exclusively to Config/Tags/<Feature>.ini.",
                    )

    def verify_header(self, h_file: Path) -> dict[str, list[dict]]:
        try:
            content = h_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return {}

        clean_content = self.strip_comments(content)
        lines = content.splitlines()

        is_ability_header = bool(
            re.search(r"\bclass\s+(?:\w+_API\s+)?([A-Z]\w+)\s*:\s*public\s+UGameplayAbility\b", clean_content)
        )
        if is_ability_header:
            for idx, line in enumerate(lines, start=1):
                clean_line = line.split("//")[0]
                for ui_hdr in FORBIDDEN_GAS_UI_HEADERS:
                    if re.search(rf'#include\s*["<]{ui_hdr}[">]', clean_line):
                        self.add_violation(
                            rule_id="GAS_UI_HEADER_IN_ABILITY",
                            file_path=h_file,
                            line_number=idx,
                            severity="ERROR",
                            detail=f"Forbidden UI/Widget header '{line.strip()}' included in UGameplayAbility header.",
                            fix="Remove UI includes from GameplayAbility; bind widgets to ASC attribute/tag delegates on the client.",
                        )

        attr_class_pat = re.compile(
            r"\bclass\s+(?:\w+_API\s+)?(?P<classname>[A-Z]\w+)(?:\s+final)?\s*:\s*public\s+UAttributeSet[^{;]*\{"
        )
        discovered_sets = {}

        pos = 0
        while pos < len(clean_content):
            match = attr_class_pat.search(clean_content, pos)
            if not match:
                break
            class_name = match.group("classname")
            open_brace_pos = match.end() - 1
            class_body, end_pos = self.extract_brace_block(clean_content, open_brace_pos)
            body_start = open_brace_pos + 1
            class_line_no = content[: match.start()].count("\n") + 1
            pos = end_pos

            if not re.search(r"#define\s+ATTRIBUTE_ACCESSORS\b", content):
                self.add_violation(
                    rule_id="GAS_MISSING_ACCESSOR_MACRO_DEF",
                    file_path=h_file,
                    line_number=class_line_no,
                    severity="WARNING",
                    detail=f"Header declaring UAttributeSet '{class_name}' does not define ATTRIBUTE_ACCESSORS macro.",
                    fix="Define `#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)` before the UAttributeSet class.",
                )

            discovered_attrs = []
            for uprop_kw in re.finditer(r"\bUPROPERTY\s*\(", class_body):
                prop_specs, after_paren = self.extract_paren_block(class_body, uprop_kw.end() - 1)
                decl_match = re.match(
                    r"\s*(?:mutable\s+)?FGameplayAttributeData\s+(?P<attrname>\w+)\s*;",
                    class_body[after_paren:],
                    re.DOTALL,
                )
                if not decl_match:
                    continue

                attr_name = decl_match.group("attrname")
                attr_pos = body_start + uprop_kw.start()
                attr_line_no = content[:attr_pos].count("\n") + 1
                is_meta = self.is_meta_attribute(attr_name, prop_specs)

                discovered_attrs.append(
                    {
                        "name": attr_name,
                        "line": attr_line_no,
                        "class": class_name,
                        "is_meta": is_meta,
                    }
                )

                # Check 1: ATTRIBUTE_ACCESSORS required for ALL attributes (including meta-attributes)
                accessor_pattern = rf"ATTRIBUTE_ACCESSORS\s*\(\s*{class_name}\s*,\s*{attr_name}\s*\)"
                if not re.search(accessor_pattern, class_body):
                    self.add_violation(
                        rule_id="GAS_MISSING_ATTRIBUTE_ACCESSORS",
                        file_path=h_file,
                        line_number=attr_line_no,
                        severity="ERROR",
                        detail=f"Attribute '{attr_name}' in '{class_name}' is missing ATTRIBUTE_ACCESSORS({class_name}, {attr_name}).",
                        fix=f"Add `ATTRIBUTE_ACCESSORS({class_name}, {attr_name})` right below the FGameplayAttributeData declaration.",
                    )

                if is_meta:
                    continue

                # Check 2: ReplicatedUsing = OnRep_PropertyName for non-meta attributes
                rep_using_match = re.search(r"ReplicatedUsing\s*=\s*(?P<repfunc>\w+)", prop_specs)
                if not rep_using_match:
                    self.add_violation(
                        rule_id="GAS_ATTRIBUTE_MISSING_REPLICATED_USING",
                        file_path=h_file,
                        line_number=attr_line_no,
                        severity="ERROR",
                        detail=f"Attribute '{attr_name}' in '{class_name}' must be marked with ReplicatedUsing = OnRep_{attr_name} (or NotReplicated / Category = \"...|Meta\" if it is a server-only meta-attribute).",
                        fix=f"Add `ReplicatedUsing = OnRep_{attr_name}` or mark as `NotReplicated` if server-only meta-attribute.",
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
                            detail=f"Attribute '{attr_name}' uses RepNotify callback '{rep_func}' instead of '{expected_rep}'.",
                            fix=f"Rename RepNotify callback to '{expected_rep}'.",
                        )

                    onrep_decl_pat = rf"void\s+{rep_func}\s*\(\s*const\s+FGameplayAttributeData\s*&\s*\w*\s*\)"
                    if not re.search(onrep_decl_pat, class_body):
                        self.add_violation(
                            rule_id="GAS_MISSING_ONREP_DECL",
                            file_path=h_file,
                            line_number=attr_line_no,
                            severity="ERROR",
                            detail=f"Missing callback declaration 'void {rep_func}(const FGameplayAttributeData& Old{attr_name})' in '{class_name}'.",
                            fix=f"Declare `UFUNCTION() virtual void {rep_func}(const FGameplayAttributeData& Old{attr_name});`.",
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
        lines = content.splitlines()

        if any(tag in cpp_file.stem for tag in ["AttributeSet", "GameplayAbility", "GA_"]):
            for idx, line in enumerate(lines, start=1):
                clean_line = line.split("//")[0]
                for ui_hdr in FORBIDDEN_GAS_UI_HEADERS:
                    if re.search(rf'#include\s*["<]{ui_hdr}[">]', clean_line):
                        self.add_violation(
                            rule_id="GAS_UI_HEADER_IN_ABILITY",
                            file_path=cpp_file,
                            line_number=idx,
                            severity="ERROR",
                            detail=f"Forbidden UI/ViewModel header '{line.strip()}' included in GAS source file.",
                            fix="Remove UI includes from GAS .cpp files; observe attributes via delegates on the client.",
                        )

        for _, (_, classes_dict) in attribute_sets.items():
            for class_name, attrs in classes_dict.items():
                if f"{class_name}::" not in clean_content:
                    continue

                pre_attr_match = re.search(rf"\b{class_name}::PreAttributeChange\s*\([^)]*\)\s*\{{", clean_content)
                if pre_attr_match:
                    pre_body, _ = self.extract_brace_block(clean_content, pre_attr_match.end() - 1)
                    if not re.search(r"\b(IsFinite|IsNaN)\s*\(", pre_body):
                        line_no = content[: pre_attr_match.start()].count("\n") + 1
                        self.add_violation(
                            rule_id="GAS_PREATTRIBUTE_MISSING_NAN_CHECK",
                            file_path=cpp_file,
                            line_number=line_no,
                            severity="ERROR",
                            detail=f"'{class_name}::PreAttributeChange' does not check FMath::IsFinite(NewValue) or FMath::IsNaN(NewValue).",
                            fix="Add `if (!FMath::IsFinite(NewValue)) { NewValue = 0.f; return; }` at the start of PreAttributeChange.",
                        )

                replicated_attrs = [a for a in attrs if not a.get("is_meta")]

                for attr in replicated_attrs:
                    attr_name = attr["name"]
                    onrep_match = re.search(rf"\b{class_name}::OnRep_{attr_name}\s*\([^)]*\)\s*\{{", clean_content)
                    if onrep_match:
                        func_body, _ = self.extract_brace_block(clean_content, onrep_match.end() - 1)
                        if "GAMEPLAYATTRIBUTE_REPNOTIFY" not in func_body:
                            line_no = content[: onrep_match.start()].count("\n") + 1
                            self.add_violation(
                                rule_id="GAS_MISSING_GAMEPLAYATTRIBUTE_REPNOTIFY",
                                file_path=cpp_file,
                                line_number=line_no,
                                severity="ERROR",
                                detail=f"Function '{class_name}::OnRep_{attr_name}' does not invoke GAMEPLAYATTRIBUTE_REPNOTIFY.",
                                fix=f"Add `GAMEPLAYATTRIBUTE_REPNOTIFY({class_name}, {attr_name}, Old{attr_name});`.",
                            )

                getlifetime_match = re.search(
                    rf"\b{class_name}::GetLifetimeReplicatedProps\s*\([^)]*\)\s*(?:const)?\s*\{{",
                    clean_content,
                )
                if getlifetime_match:
                    gl_body, _ = self.extract_brace_block(clean_content, getlifetime_match.end() - 1)
                    for attr in replicated_attrs:
                        attr_name = attr["name"]
                        dorep_pat = (
                            rf"DOREPLIFETIME_CONDITION_NOTIFY\s*\(\s*{class_name}\s*,\s*{attr_name}\s*,\s*COND_\w+\s*,\s*REPNOTIFY_Always\s*\)"
                        )
                        if not re.search(dorep_pat, gl_body):
                            line_no = content[: getlifetime_match.start()].count("\n") + 1
                            self.add_violation(
                                rule_id="GAS_MISSING_DOREPLIFETIME_NOTIFY",
                                file_path=cpp_file,
                                line_number=line_no,
                                severity="ERROR",
                                detail=f"Attribute '{class_name}::{attr_name}' is not registered via DOREPLIFETIME_CONDITION_NOTIFY(..., REPNOTIFY_Always).",
                                fix=f"Add `DOREPLIFETIME_CONDITION_NOTIFY({class_name}, {attr_name}, COND_None, REPNOTIFY_Always);` (or COND_OwnerOnly).",
                            )


def run_gas_verification(source_path: Path, target_file: str | None = None) -> dict:
    verifier = GASVerifier(source_path, target_file=target_file)
    verifier.scan_all()

    error_count = sum(1 for v in verifier.violations if v["severity"] == "ERROR")
    warning_count = sum(1 for v in verifier.violations if v["severity"] == "WARNING")
    status = "PASS" if error_count == 0 else "FAIL"

    return {
        "status": status,
        "scoped_file": target_file,
        "total_violations": len(verifier.violations),
        "error_count": error_count,
        "warning_count": warning_count,
        "violations": verifier.violations,
        "summary": f"GAS Static Verification {status.lower()}ed: {error_count} error(s), {warning_count} warning(s).",
    }


def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 GAS Static Verification")
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
            "total_violations": 1,
            "error_count": 1,
            "warning_count": 0,
            "violations": [
                {
                    "rule": "SOURCE_DIR_NOT_FOUND",
                    "file": str(source_dir),
                    "line": 0,
                    "severity": "ERROR",
                    "detail": f"Source directory '{source_dir}' does not exist.",
                }
            ],
            "summary": "Failed: Source directory not found.",
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    result = run_gas_verification(source_dir, target_file=args.file)
    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "PASS" else 1)


if __name__ == "__main__":
    main()
