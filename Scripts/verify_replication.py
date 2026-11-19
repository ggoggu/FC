#!/usr/bin/env python3
"""
Unreal Engine 5.8 Multiplayer, Replication & Anti-Slop Static Verification Script
Scans C++ headers and source files in Source/ using balanced block/paren parsing and
multi-line pattern analysis to eliminate regex truncation and false-negative escapes.
Supports full-repository scans, incremental single-file ACI checks (--file), and
explicit frozen legacy baseline tracking (Scripts/linter_baseline.json).
"""

import sys
import re
import json
import argparse
from pathlib import Path

FORBIDDEN_UI_HEADERS = [
    r"Blueprint/UserWidget\.h",
    r"Blueprint/WidgetTree\.h",
    r"UMG\.h",
    r"Components/Widget\.h",
    r"SlateBasics\.h",
    r"SlateCore\.h",
    r"SlateOptMacros\.h",
    r"MVVMViewModelBase\.h",
]

DEFAULT_BASELINE_PATH = Path(__file__).resolve().parent / "linter_baseline.json"

ESCAPE_HATCH_PATTERN = re.compile(
    r"//\s*(TODO|FIXME|HACK|XXX|WORKAROUND|FOR\s+NOW)\b",
    re.IGNORECASE,
)


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


def load_baseline(baseline_path: Path | None) -> dict[str, set[tuple[str, str]]]:
    """Loads frozen legacy baseline keyed by rule_id -> {(rel_path, symbol)}."""
    result: dict[str, set[tuple[str, str]]] = {}
    if not baseline_path or not baseline_path.is_file():
        return result
    try:
        raw = json.loads(baseline_path.read_text(encoding="utf-8"))
        for rule_id, entries in raw.items():
            if rule_id.startswith("_") or not isinstance(entries, list):
                continue
            rule_set = set()
            for item in entries:
                if isinstance(item, dict) and item.get("file") and item.get("symbol"):
                    rule_set.add((item["file"].replace("\\", "/"), item["symbol"]))
            result[rule_id] = rule_set
    except Exception:
        return {}
    return result


class ReplicationVerifier:
    def __init__(
        self,
        source_dir: Path,
        target_file: str | None = None,
        baseline_path: Path | None = DEFAULT_BASELINE_PATH,
        ignore_baseline: bool = False,
    ):
        self.source_dir = source_dir
        self.target_file = target_file
        self.baseline = {} if ignore_baseline else load_baseline(baseline_path)
        self.baseline_suppressed_count = 0
        self.violations = []
        self.math_payload_structs = set()
        self.container_payload_structs = set()
        self.server_rpcs_by_class = {}
        self.authority_only_methods_by_class = {}

    def _rel_path(self, file_path: Path) -> str:
        try:
            if self.source_dir.parent in file_path.parents:
                return file_path.relative_to(self.source_dir.parent).as_posix()
        except Exception:
            pass
        return file_path.as_posix()

    def is_baselined(self, rule_id: str, file_path: Path, symbol: str) -> bool:
        entries = self.baseline.get(rule_id)
        if not entries:
            return False
        key = (self._rel_path(file_path), symbol)
        if key in entries:
            self.baseline_suppressed_count += 1
            return True
        return False

    def add_violation(
        self,
        rule_id: str,
        file_path: Path,
        line_number: int,
        severity: str,
        message: str,
        fix: str = "",
        ref: str = "",
    ):
        entry = {
            "rule": rule_id,
            "file": self._rel_path(file_path),
            "line": line_number,
            "severity": severity,
            "message": message,
        }
        if fix:
            entry["fix"] = fix
        if ref:
            entry["ref"] = ref
        self.violations.append(entry)

    def strip_comments(self, text: str) -> str:
        """Replaces C/C++ comments with equal-length spaces/newlines to preserve exact offsets & line numbers."""
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
        """Extracts balanced delimited body starting at open_pos, ignoring string/char literals."""
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

    def extract_brace_block(self, text: str, open_brace_pos: int) -> tuple[str, int]:
        return self.extract_delimited_block(text, open_brace_pos, "{", "}")

    def extract_paren_block(self, text: str, open_paren_pos: int) -> tuple[str, int]:
        return self.extract_delimited_block(text, open_paren_pos, "(", ")")

    def iter_class_blocks(self, clean_content: str):
        """Yields (class_name, parent_name, body, header_start, body_start) using balanced brace parsing."""
        cls_header_pat = re.compile(
            r"\bclass\s+(?:\w+_API\s+)?(?P<classname>[A-Z]\w+)(?:\s+final)?\s*(?::\s*(?:public|protected|private)\s+(?P<parent>[A-Za-z_]\w*))?[^{;]*\{"
        )
        pos = 0
        n = len(clean_content)
        while pos < n:
            m = cls_header_pat.search(clean_content, pos)
            if not m:
                break
            open_brace_pos = m.end() - 1
            body, end_pos = self.extract_brace_block(clean_content, open_brace_pos)
            yield (
                m.group("classname"),
                m.group("parent") or "",
                body,
                m.start(),
                open_brace_pos + 1,
            )
            pos = end_pos

    def collect_payload_structs(self):
        """Scans all headers in Source/ for USTRUCTs containing floating-point/vector or dynamic container fields."""
        self.math_payload_structs = set()
        self.container_payload_structs = set()
        math_field_pat = re.compile(
            r"\b(FVector(?:_NetQuantize\d*)?|FVector2D|FRotator|FTransform|float|double)\b"
        )
        container_field_pat = re.compile(r"\b(TArray|TSet|TMap)\s*<|\bFString\b")
        struct_header_pat = re.compile(
            r"USTRUCT\s*\([^)]*\)\s*struct\s+(?:\w+_API\s+)?(?P<sname>[A-Z]\w+)[^{;]*\{"
        )
        struct_bodies = {}
        for h_file in self.source_dir.rglob("*.h"):
            try:
                clean = self.strip_comments(h_file.read_text(encoding="utf-8", errors="replace"))
            except Exception:
                continue
            for m in struct_header_pat.finditer(clean):
                sname = m.group("sname")
                body, _ = self.extract_brace_block(clean, m.end() - 1)
                struct_bodies[sname] = body
                if math_field_pat.search(body):
                    self.math_payload_structs.add(sname)
                if container_field_pat.search(body):
                    self.container_payload_structs.add(sname)

        changed = True
        while changed:
            changed = False
            for sname, body in struct_bodies.items():
                if sname not in self.math_payload_structs and any(
                    re.search(rf"\b{m}\b", body) for m in self.math_payload_structs
                ):
                    self.math_payload_structs.add(sname)
                    changed = True
                if sname not in self.container_payload_structs and any(
                    re.search(rf"\b{c}\b", body) for c in self.container_payload_structs
                ):
                    self.container_payload_structs.add(sname)
                    changed = True

    def scan_all(self):
        self.collect_payload_structs()
        self.server_rpcs_by_class = {}
        self.authority_only_methods_by_class = {}

        header_files, cpp_files = resolve_target_files(self.source_dir, self.target_file)

        for h_file in header_files:
            self.verify_header_file(h_file)

        for cpp_file in cpp_files:
            self.verify_cpp_file(cpp_file)

    def _is_connection_owned_parent(self, parent_name: str) -> bool:
        if not parent_name:
            return False
        if "AIController" in parent_name:
            return False
        return any(
            base in parent_name
            for base in [
                "PlayerController",
                "Pawn",
                "Character",
                "PlayerState",
                "ActorComponent",
                "Component",
            ]
        )

    def verify_header_file(self, h_file: Path):
        try:
            content = h_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        lines = content.splitlines()
        clean_content = self.strip_comments(content)

        # Check 0: Escape-Hatch Comments (Hard Constraint -> ERROR)
        for idx, line in enumerate(lines, start=1):
            if ESCAPE_HATCH_PATTERN.search(line):
                self.add_violation(
                    rule_id="CODE_ESCAPE_HATCH_COMMENT",
                    file_path=h_file,
                    line_number=idx,
                    severity="ERROR",
                    message=f"Escape-hatch comment detected ('{line.strip()}').",
                    fix="Implement the complete logic or remove the TODO/FIXME/HACK/Workaround/For now comment.",
                    ref=".antigravity/rules/01-ue58-coding-standards.md#7",
                )

        # Check 1: Forbidden UI Headers in Actor/Gameplay Headers & GameMode in UI/VM Headers
        is_actor_or_gameplay_header = bool(
            re.search(
                r"\bclass\s+(?:\w+_API\s+)?([A-Z]\w+)\s*:\s*public\s+(AActor|ACharacter|APawn|AGameMode|AGameModeBase|AGameState|AGameStateBase|APlayerState|AAIController|UActorComponent)\b",
                clean_content,
            )
        )
        is_ui_or_vm_header = bool(
            re.search(
                r"\bclass\s+(?:\w+_API\s+)?([A-Z]\w+)\s*:\s*public\s+(UUserWidget|UMVVMViewModelBase)\b",
                clean_content,
            )
        )

        if is_ui_or_vm_header:
            for idx, line in enumerate(lines, start=1):
                clean_line = line.split("//")[0]
                if re.search(r'#include\s*["<][^">]*GameMode[^">]*[">]', clean_line):
                    self.add_violation(
                        rule_id="UI_GAMEMODE_DEPENDENCY",
                        file_path=h_file,
                        line_number=idx,
                        severity="ERROR",
                        message=f"Client-only UI/ViewModel header includes server GameMode header: '{line.strip()}'.",
                        fix="Observe replicated GameState/PlayerState or ViewModel FieldNotify instead of including GameMode.",
                        ref=".antigravity/rules/06-umg-mvvm-standards.md#1",
                    )

        if is_actor_or_gameplay_header:
            for idx, line in enumerate(lines, start=1):
                clean_line = line.split("//")[0]
                for ui_hdr_pat in FORBIDDEN_UI_HEADERS:
                    if re.search(rf'#include\s*["<]{ui_hdr_pat}[">]', clean_line):
                        self.add_violation(
                            rule_id="REPL_UI_HEADER_IN_SERVER_ACTOR",
                            file_path=h_file,
                            line_number=idx,
                            severity="ERROR",
                            message=f"Forbidden UI/Widget header included in gameplay actor header: '{line.strip()}'.",
                            fix="Remove UI includes from gameplay headers; bind UI reactively on the client via OnRep_* or FieldNotify.",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#4",
                        )

        # Check 2 & 3: Balanced Class Body Inspection (RPCs, UPROPERTY, MVVM, Replication)
        for class_name, parent_name, class_body, class_start_pos, body_start_pos in self.iter_class_blocks(clean_content):
            class_line_no = content[:class_start_pos].count("\n") + 1
            is_viewmodel_class = parent_name == "UMVVMViewModelBase" or class_name.endswith("ViewModel")
            is_connection_owned = self._is_connection_owned_parent(parent_name)

            # Inspect UFUNCTION declarations using balanced paren extraction
            for ufunc_kw in re.finditer(r"\bUFUNCTION\s*\(", class_body):
                specifiers, after_specs = self.extract_paren_block(class_body, ufunc_kw.end() - 1)
                sig_match = re.match(
                    r"\s*(?:virtual\s+)?(?:[\w:<>*&\s]+?)\s+(?P<funcname>\w+)\s*\(",
                    class_body[after_specs:],
                    re.DOTALL,
                )
                if not sig_match:
                    continue
                func_name = sig_match.group("funcname")
                params_open = after_specs + sig_match.end() - 1
                params, after_params = self.extract_paren_block(class_body, params_open)
                tail_match = re.match(r"(?P<qualifiers>[^;{]*)(?P<term>;|\{)", class_body[after_params:], re.DOTALL)
                if not tail_match:
                    continue
                qualifiers = tail_match.group("qualifiers")
                term = tail_match.group("term")
                func_pos = body_start_pos + ufunc_kw.start()
                line_no = content[:func_pos].count("\n") + 1

                if re.search(r"\bServer\b", specifiers, re.IGNORECASE):
                    if not re.search(r"\bWithValidation\b", specifiers, re.IGNORECASE):
                        self.add_violation(
                            rule_id="REPL_SERVER_RPC_MISSING_VALIDATION",
                            file_path=h_file,
                            line_number=line_no,
                            severity="ERROR",
                            message=f"Server RPC '{class_name}::{func_name}' declared without 'WithValidation'.",
                            fix=f"Add WithValidation to UFUNCTION(Server, ..., WithValidation) and implement {func_name}_Validate().",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#2",
                        )

                    if not is_connection_owned:
                        self.add_violation(
                            rule_id="REPL_SERVER_RPC_UNOWNED_ACTOR",
                            file_path=h_file,
                            line_number=line_no,
                            severity="ERROR",
                            message=(
                                f"Server RPC '{func_name}' declared on '{class_name}' (parent '{parent_name}'). "
                                "Clients can ONLY invoke Server RPCs on connection-owned actors."
                            ),
                            fix="Move the Server RPC to APlayerController, APlayerState, possessed APawn/ACharacter, or their UActorComponent.",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#2",
                        )

                    self.server_rpcs_by_class.setdefault(class_name, []).append(
                        {
                            "name": func_name,
                            "params": params.strip(),
                            "line": line_no,
                            "header": h_file,
                        }
                    )

                if re.search(r"\bNetMulticast\b", specifiers, re.IGNORECASE) and re.search(
                    r"\bReliable\b", specifiers, re.IGNORECASE
                ):
                    self.add_violation(
                        rule_id="NET_SEC_RELIABLE_MULTICAST",
                        file_path=h_file,
                        line_number=line_no,
                        severity="ERROR",
                        message=f"NetMulticast RPC '{class_name}::{func_name}' is marked 'Reliable'.",
                        fix="Change Reliable -> Unreliable for transient cosmetics, or use ReplicatedUsing = OnRep_* for persistent state.",
                        ref=".antigravity/rules/02-multiplayer-conventions.md#6",
                    )

                if re.search(r"\bBlueprintAuthorityOnly\b", specifiers, re.IGNORECASE):
                    is_const = bool(re.search(r"\bconst\b", qualifiers))
                    is_pure_getter = bool(re.match(r"^(Get|Is|Has|Can)[A-Z]", func_name))
                    if not is_const and not is_pure_getter and not re.search(r"\bServer\b", specifiers, re.IGNORECASE):
                        if term == "{":
                            brace_pos = after_params + tail_match.end() - 1
                            inline_body, _ = self.extract_brace_block(class_body, brace_pos)
                            if not re.search(
                                r"\bHasAuthority\s*\(|\bROLE_Authority\b|\bGetAuthGameMode\s*\(|\bNM_Client\b|\bIsRunningDedicatedServer\s*\(",
                                inline_body,
                            ):
                                self.add_violation(
                                    rule_id="NET_SEC_AUTHORITY_ONLY_MISSING_CPP_GUARD",
                                    file_path=h_file,
                                    line_number=line_no,
                                    severity="ERROR",
                                    message=f"Inline BlueprintAuthorityOnly method '{class_name}::{func_name}' lacks an internal HasAuthority() guard.",
                                    fix="Add `if (!HasAuthority()) { return; }` inside the C++ body.",
                                    ref=".antigravity/rules/02-multiplayer-conventions.md#6",
                                )
                        else:
                            self.authority_only_methods_by_class.setdefault(class_name, []).append(
                                {
                                    "name": func_name,
                                    "line": line_no,
                                    "header": h_file,
                                }
                            )

            # Inspect UPROPERTY declarations using balanced paren extraction
            has_replicated_props = False
            for uprop_kw in re.finditer(r"\bUPROPERTY\s*\(", class_body):
                prop_specs, after_paren = self.extract_paren_block(class_body, uprop_kw.end() - 1)
                decl_match = re.match(
                    r"\s*(?P<proptype>[A-Za-z0-9_<>\s*&:,]+?)\s+(?P<varname>\w+)\s*(?:=\s*[^;]+)?;",
                    class_body[after_paren:],
                    re.DOTALL,
                )
                if not decl_match:
                    continue
                prop_type = re.sub(r"\s+", " ", decl_match.group("proptype").strip())
                var_name = decl_match.group("varname")
                prop_pos = body_start_pos + uprop_kw.start()
                prop_line_no = content[:prop_pos].count("\n") + 1

                if prop_type.endswith("*") and re.match(r"^(?:const\s+)?[UA][A-Z]\w*\s*\*$", prop_type):
                    inner_type = prop_type[:-1].strip()
                    self.add_violation(
                        rule_id="UE58_RAW_POINTER_UPROPERTY",
                        file_path=h_file,
                        line_number=prop_line_no,
                        severity="ERROR",
                        message=f"Raw UObject pointer '{prop_type} {var_name}' in UPROPERTY.",
                        fix=f"Replace '{prop_type}' with 'TObjectPtr<{inner_type}>'.",
                        ref=".antigravity/rules/01-ue58-coding-standards.md#2",
                    )

                is_replicated = bool(re.search(r"\bReplicated\b", prop_specs, re.IGNORECASE))
                rep_using_match = re.search(r"ReplicatedUsing\s*=\s*(?P<funcname>\w+)", prop_specs, re.IGNORECASE)

                if is_viewmodel_class:
                    if is_replicated or rep_using_match:
                        self.add_violation(
                            rule_id="MVVM_REPLICATED_FORBIDDEN",
                            file_path=h_file,
                            line_number=prop_line_no,
                            severity="ERROR",
                            message=f"ViewModel '{class_name}' property '{var_name}' is marked Replicated.",
                            fix="Remove Replicated/ReplicatedUsing; push replicated model changes into ViewModel on the client via FieldNotify.",
                            ref=".antigravity/rules/06-umg-mvvm-standards.md#1",
                        )
                    if re.search(r"\bTObjectPtr\s*<\s*UTexture2D\s*>|\bUTexture2D\s*\*", prop_type):
                        self.add_violation(
                            rule_id="MVVM_HARD_TEXTURE_REF",
                            file_path=h_file,
                            line_number=prop_line_no,
                            severity="ERROR",
                            message=f"ViewModel '{class_name}' property '{var_name}' uses hard texture reference '{prop_type}'.",
                            fix="Replace with 'TSoftObjectPtr<UTexture2D>'.",
                            ref=".antigravity/rules/06-umg-mvvm-standards.md#5",
                        )

                if is_replicated or rep_using_match:
                    has_replicated_props = True

                    if re.search(r"\bBlueprintReadWrite\b", prop_specs) and not re.search(
                        r"\bBlueprintSetter\s*=", prop_specs
                    ):
                        clean_var = (
                            var_name[1:]
                            if (var_name.startswith("b") and len(var_name) > 1 and var_name[1].isupper())
                            else var_name
                        )
                        setter_name = f"Set{clean_var}"
                        if re.search(rf"\bvoid\s+{setter_name}\s*\([^)]*\)\s*;", class_body):
                            self.add_violation(
                                rule_id="REPL_BYPASSED_AUTHORITY_SETTER",
                                file_path=h_file,
                                line_number=prop_line_no,
                                severity="ERROR",
                                message=f"Replicated property '{var_name}' is 'BlueprintReadWrite' without 'BlueprintSetter = {setter_name}'.",
                                fix=f"Add `BlueprintSetter = {setter_name}` to UPROPERTY or switch to `BlueprintReadOnly`.",
                                ref=".antigravity/rules/03-loop-engineering.md#4",
                            )

                if rep_using_match:
                    func_name = rep_using_match.group("funcname")
                    clean_var = (
                        var_name[1:]
                        if (var_name.startswith("b") and len(var_name) > 1 and var_name[1].isupper())
                        else var_name
                    )
                    valid_names = {f"OnRep_{var_name}", f"OnRep_{clean_var}"}
                    is_declared_in_class = bool(re.search(rf"\bvoid\s+{func_name}\s*\(", class_body))
                    if func_name not in valid_names and not (
                        func_name.startswith("OnRep_") and is_declared_in_class
                    ):
                        self.add_violation(
                            rule_id="REPL_INVALID_REPNOTIFY_NAME",
                            file_path=h_file,
                            line_number=prop_line_no,
                            severity="WARNING",
                            message=f"Replication callback '{func_name}' for '{var_name}' does not match 'OnRep_{clean_var}' or is missing declaration.",
                            fix=f"Rename callback to OnRep_{clean_var} and declare `UFUNCTION() void OnRep_{clean_var}();`.",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#1",
                        )

            if has_replicated_props and not re.search(r"\bGetLifetimeReplicatedProps\b", class_body):
                self.add_violation(
                    rule_id="REPL_MISSING_LIFETIME_PROPS_DECL",
                    file_path=h_file,
                    line_number=class_line_no,
                    severity="ERROR",
                    message=f"Class '{class_name}' defines replicated properties without declaring GetLifetimeReplicatedProps.",
                    fix="Add `virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;`.",
                    ref=".antigravity/rules/02-multiplayer-conventions.md#1",
                )

    def verify_cpp_file(self, cpp_file: Path):
        try:
            content = cpp_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        clean_content = self.strip_comments(content)
        lines = content.splitlines()

        has_doreplifetime = bool(re.search(r"\bDOREPLIFETIME\b|\bDOREPLIFETIME_CONDITION\b", clean_content))
        has_getlifetime_def = bool(re.search(r"::GetLifetimeReplicatedProps\s*\(", clean_content))

        # Check 4: Missing #include "Net/UnrealNetwork.h"
        if has_doreplifetime or has_getlifetime_def:
            if not re.search(r'#include\s*["<]Net/UnrealNetwork\.h[">]', clean_content):
                self.add_violation(
                    rule_id="REPL_MISSING_NET_INCLUDE",
                    file_path=cpp_file,
                    line_number=1,
                    severity="ERROR",
                    message="Source file uses DOREPLIFETIME / GetLifetimeReplicatedProps without '#include \"Net/UnrealNetwork.h\"'.",
                    fix='Add `#include "Net/UnrealNetwork.h"` near the top of the .cpp file.',
                    ref=".antigravity/rules/02-multiplayer-conventions.md#1",
                )

        # Check 5: Super::GetLifetimeReplicatedProps using balanced brace block
        for gl_match in re.finditer(r"::GetLifetimeReplicatedProps\s*\([^)]*\)\s*(?:const)?\s*\{", clean_content):
            body, _ = self.extract_brace_block(clean_content, gl_match.end() - 1)
            if not re.search(r"Super::GetLifetimeReplicatedProps\s*\(", body):
                line_no = content[: gl_match.start()].count("\n") + 1
                self.add_violation(
                    rule_id="REPL_MISSING_SUPER_LIFETIME_CALL",
                    file_path=cpp_file,
                    line_number=line_no,
                    severity="ERROR",
                    message="GetLifetimeReplicatedProps does not call Super::GetLifetimeReplicatedProps(OutLifetimeProps).",
                    fix="Add `Super::GetLifetimeReplicatedProps(OutLifetimeProps);` as the first line.",
                    ref=".antigravity/rules/02-multiplayer-conventions.md#1",
                )

        # Check 6: Actor / Component Constructor bReplicates Check (balanced brace block)
        for ctor in re.finditer(r"\b([A-Z]\w+)::\1\s*\([^)]*\)\s*(?::\s*[^\{]+)?\{", clean_content, re.DOTALL):
            class_name = ctor.group(1)
            ctor_body, _ = self.extract_brace_block(clean_content, ctor.end() - 1)
            ctor_line_no = content[: ctor.start()].count("\n") + 1

            if re.search(rf"\b{class_name}::GetLifetimeReplicatedProps\b", clean_content):
                if class_name.startswith("A"):
                    if not re.search(r"bReplicates\s*=\s*true|SetReplicates\s*\(\s*true\s*\)", ctor_body):
                        self.add_violation(
                            rule_id="REPL_ACTOR_CTOR_MISSING_BREPLICATES",
                            file_path=cpp_file,
                            line_number=ctor_line_no,
                            severity="ERROR",
                            message=f"Actor constructor '{class_name}::{class_name}' replicates properties without setting bReplicates = true.",
                            fix="Add `bReplicates = true;` inside the constructor.",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#3",
                        )
                elif class_name.startswith("U") and class_name.endswith("Component"):
                    if not re.search(r"SetIsReplicatedByDefault\s*\(\s*true\s*\)|bReplicates\s*=\s*true", ctor_body):
                        self.add_violation(
                            rule_id="REPL_COMPONENT_CTOR_MISSING_REPLICATION",
                            file_path=cpp_file,
                            line_number=ctor_line_no,
                            severity="ERROR",
                            message=f"Component constructor '{class_name}::{class_name}' replicates properties without SetIsReplicatedByDefault(true).",
                            fix="Add `SetIsReplicatedByDefault(true);` inside the constructor.",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#3",
                        )

        # Check 7: OpenLevel in GameMode
        if "GameMode" in cpp_file.stem:
            for m in re.finditer(r"UGameplayStatics::OpenLevel\b", clean_content):
                line_no = content[: m.start()].count("\n") + 1
                self.add_violation(
                    rule_id="REPL_OPENLEVEL_IN_GAMEMODE",
                    file_path=cpp_file,
                    line_number=line_no,
                    severity="ERROR",
                    message="UGameplayStatics::OpenLevel called in GameMode (disconnects remote clients).",
                    fix="Use `GetWorld()->ServerTravel(MapURL, false)` instead.",
                    ref=".antigravity/rules/02-multiplayer-conventions.md#5",
                )

        # Check 8: Multi-line Loose CSV file reads
        for m in re.finditer(r"FPaths::ProjectContentDir\s*\(\s*\)\s*/\s*TEXT\s*\([^)]*\.csv", clean_content, re.DOTALL):
            line_no = content[: m.start()].count("\n") + 1
            self.add_violation(
                rule_id="DATA_RUNTIME_CSV_READ",
                file_path=cpp_file,
                line_number=line_no,
                severity="ERROR",
                message="Direct runtime disk CSV access detected (breaks packaged/cooked builds).",
                fix="Reference a cooked UDataTable or UPrimaryDataAsset property instead of reading loose CSV files.",
                ref="AGENTS.md#1",
            )

        # Check 9: Multi-line ConstructorHelpers::FObjectFinder / FClassFinder (Hard ERROR with Frozen Baseline)
        ctor_finder_pat = re.compile(
            r"ConstructorHelpers::(?P<kind>FObjectFinder|FClassFinder)\s*<[^>]+>\s+(?P<sym>\w+)\s*\(",
            re.DOTALL,
        )
        for m in ctor_finder_pat.finditer(clean_content):
            sym = m.group("sym")
            if self.is_baselined("CODE_HARDCODED_ASSET_PATH", cpp_file, sym):
                continue
            line_no = content[: m.start()].count("\n") + 1
            self.add_violation(
                rule_id="CODE_HARDCODED_ASSET_PATH",
                file_path=cpp_file,
                line_number=line_no,
                severity="ERROR",
                message=f"Banned ConstructorHelpers::{m.group('kind')} ('{sym}') in C++ constructor.",
                fix="Configure asset references via TSoftObjectPtr, UPrimaryDataAsset, or Blueprint Class Defaults (BP_*).",
                ref=".antigravity/rules/01-ue58-coding-standards.md#8",
            )

        # Check 9a: Escape-Hatch Comments in .cpp (Hard ERROR)
        for idx, line in enumerate(lines, start=1):
            if ESCAPE_HATCH_PATTERN.search(line):
                self.add_violation(
                    rule_id="CODE_ESCAPE_HATCH_COMMENT",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="ERROR",
                    message=f"Escape-hatch comment detected ('{line.strip()}').",
                    fix="Implement the complete logic or remove the TODO/FIXME/HACK/Workaround/For now comment.",
                    ref=".antigravity/rules/01-ue58-coding-standards.md#7",
                )

        # Check 9b: Multi-line Banned Footgun APIs (Index-0 Player getters & Raw 'this' AddLambda/BindLambda)
        for m in re.finditer(
            r"UGameplayStatics::(GetPlayerController|GetPlayerPawn|GetPlayerCharacter)\s*\([^,]+,\s*0\s*\)",
            clean_content,
            re.DOTALL,
        ):
            line_no = content[: m.start()].count("\n") + 1
            self.add_violation(
                rule_id="CODE_BANNED_INDEX0_PLAYER_API",
                file_path=cpp_file,
                line_number=line_no,
                severity="ERROR",
                message=f"Banned single-player index-0 API (UGameplayStatics::{m.group(1)}(..., 0)).",
                fix="Resolve ownership explicitly via GetController(), GetOwningPlayer(), or Server RPC caller context.",
                ref=".antigravity/rules/01-ue58-coding-standards.md#8",
            )

        for m in re.finditer(r"\.(AddLambda|BindLambda)\s*\(\s*\[[^\]]*\bthis\b", clean_content, re.DOTALL):
            line_no = content[: m.start()].count("\n") + 1
            safe_api = "AddWeakLambda" if m.group(1) == "AddLambda" else "BindWeakLambda"
            self.add_violation(
                rule_id="CODE_UNSAFE_RAW_THIS_LAMBDA",
                file_path=cpp_file,
                line_number=line_no,
                severity="ERROR",
                message=f"Unsafe '.{m.group(1)}([this, ...])' delegate binding detected (dangling UObject crash risk).",
                fix=f"Replace with '.{safe_api}(this, [...])' so Unreal automatically guards UObject lifetime.",
                ref=".antigravity/rules/01-ue58-coding-standards.md#8",
            )

        # Check 9c: Server/Gameplay .cpp UI header leaks & UI/VM .cpp GameMode leaks
        is_server_only_cpp = any(
            tag in cpp_file.stem for tag in ["GameMode", "AIController", "GameState", "AttributeSet", "AbilitySystem"]
        )
        is_ui_or_vm_cpp = any(tag in cpp_file.stem for tag in ["Widget", "ViewModel"])
        for idx, line in enumerate(lines, start=1):
            clean_line = line.split("//")[0]
            if is_server_only_cpp:
                for ui_hdr_pat in FORBIDDEN_UI_HEADERS:
                    if re.search(rf'#include\s*["<]{ui_hdr_pat}[">]', clean_line):
                        self.add_violation(
                            rule_id="REPL_UI_HEADER_IN_SERVER_CPP",
                            file_path=cpp_file,
                            line_number=idx,
                            severity="ERROR",
                            message=f"Forbidden UI/ViewModel header included in server/gameplay source file: '{line.strip()}'.",
                            fix="Decouple UI presentation from server/GAS code; emit replicated state or delegates instead.",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#4",
                        )
            if is_ui_or_vm_cpp and re.search(r'#include\s*["<][^">]*GameMode[^">]*[">]', clean_line):
                self.add_violation(
                    rule_id="UI_GAMEMODE_DEPENDENCY",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="ERROR",
                    message=f"Client-only UI/ViewModel source file includes server GameMode header: '{line.strip()}'.",
                    fix="Read state from GameState/PlayerState/ViewModel rather than server-only GameMode.",
                    ref=".antigravity/rules/06-umg-mvvm-standards.md#1",
                )

        # Check 10: Trivial Server RPC _Validate (Hard Constraint -> ERROR)
        validate_pat = re.compile(r"\b(?P<cls>[A-Z]\w+)::(?P<func>\w+)_Validate\s*\((?P<params>[^)]*)\)\s*\{")
        for val_match in validate_pat.finditer(clean_content):
            params_str = val_match.group("params").strip()
            if params_str and params_str != "void":
                body, _ = self.extract_brace_block(clean_content, val_match.end() - 1)
                has_conditional_or_expr = bool(re.search(r"\bif\b|\bswitch\b|\?|\breturn\s+(?!true\s*;)", body))
                if not has_conditional_or_expr:
                    line_no = content[: val_match.start()].count("\n") + 1
                    self.add_violation(
                        rule_id="NET_SEC_TRIVIAL_RPC_VALIDATE",
                        file_path=cpp_file,
                        line_number=line_no,
                        severity="ERROR",
                        message=f"Server RPC '{val_match.group('cls')}::{val_match.group('func')}_Validate' accepts client parameters but unconditionally returns 'true'.",
                        fix="Validate bounds, NaN/Inf (!ContainsNaN()/FMath::IsFinite()), enum ranges, or container max size.",
                        ref=".antigravity/rules/02-multiplayer-conventions.md#6",
                    )

        # Check 11 & 12: Server RPC float/vector NaN sanitization & container bounds checks
        math_type_pat = re.compile(r"\b(FVector(?:_NetQuantize\d*)?|FVector2D|FRotator|FTransform|float|double)\b")
        container_type_pat = re.compile(r"\b(TArray|TSet|TMap)\s*<|\bFString\b")
        for cls_name, rpc_list in self.server_rpcs_by_class.items():
            if f"{cls_name}::" not in clean_content:
                continue
            for rpc in rpc_list:
                rpc_name = rpc["name"]
                rpc_params = rpc["params"]

                val_m = re.search(rf"\b{cls_name}::{rpc_name}_Validate\s*\([^)]*\)\s*\{{", clean_content)
                impl_m = re.search(rf"\b{cls_name}::{rpc_name}_Implementation\s*\([^)]*\)\s*\{{", clean_content)
                val_body = self.extract_brace_block(clean_content, val_m.end() - 1)[0] if val_m else ""
                impl_body = self.extract_brace_block(clean_content, impl_m.end() - 1)[0] if impl_m else ""
                if not val_m and not impl_m:
                    continue

                report_pos = val_m.start() if val_m else impl_m.start()
                report_line = content[:report_pos].count("\n") + 1

                has_math_param = bool(math_type_pat.search(rpc_params))
                if not has_math_param and self.math_payload_structs:
                    has_math_param = any(re.search(rf"\b{sname}\b", rpc_params) for sname in self.math_payload_structs)

                if has_math_param:
                    combined = val_body + "\n" + impl_body
                    if not re.search(r"\b(ContainsNaN|IsNaN|IsFinite)\s*\(", combined):
                        self.add_violation(
                            rule_id="NET_SEC_UNSANITIZED_FLOAT_OR_VECTOR_RPC",
                            file_path=cpp_file,
                            line_number=report_line,
                            severity="ERROR",
                            message=f"Server RPC '{cls_name}::{rpc_name}' accepts floating-point/vector payload without NaN/Inf sanitization.",
                            fix="Check `!Param.ContainsNaN()` or `FMath::IsFinite(Param)` inside _Validate() / _Implementation().",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#6",
                        )

                has_container_param = bool(container_type_pat.search(rpc_params))
                if not has_container_param and self.container_payload_structs:
                    has_container_param = any(
                        re.search(rf"\b{sname}\b", rpc_params) for sname in self.container_payload_structs
                    )
                if has_container_param:
                    combined_bounds = val_body + "\n" + impl_body
                    if not re.search(r"\.(Num|Len)\s*\(", combined_bounds):
                        self.add_violation(
                            rule_id="NET_SEC_UNBOUNDED_RPC_CONTAINER",
                            file_path=cpp_file,
                            line_number=report_line,
                            severity="ERROR",
                            message=f"Server RPC '{cls_name}::{rpc_name}' accepts dynamic container/FString without size/length bounds check.",
                            fix="Validate `Param.Num() <= MaxAllowed` or `Param.Len() <= MaxAllowed` inside _Validate().",
                            ref=".antigravity/rules/02-multiplayer-conventions.md#6",
                        )

        # Check 13: BlueprintAuthorityOnly non-const methods missing C++ HasAuthority() guard
        auth_guard_pat = re.compile(
            r"\bHasAuthority\s*\(|\bROLE_Authority\b|\bGetAuthGameMode\s*\(|\bNM_Client\b|\bIsRunningDedicatedServer\s*\("
        )
        for cls_name, method_list in self.authority_only_methods_by_class.items():
            if f"{cls_name}::" not in clean_content:
                continue
            for meth in method_list:
                m_name = meth["name"]
                def_m = re.search(rf"\b{cls_name}::{m_name}\s*\([^)]*\)\s*\{{", clean_content)
                if not def_m:
                    continue
                body, _ = self.extract_brace_block(clean_content, def_m.end() - 1)
                if not auth_guard_pat.search(body):
                    line_no = content[: def_m.start()].count("\n") + 1
                    self.add_violation(
                        rule_id="NET_SEC_AUTHORITY_ONLY_MISSING_CPP_GUARD",
                        file_path=cpp_file,
                        line_number=line_no,
                        severity="ERROR",
                        message=f"BlueprintAuthorityOnly method '{cls_name}::{m_name}' lacks a C++ HasAuthority() guard.",
                        fix="Add `if (!HasAuthority()) { return; }` (or owner/GameMode authority check) at the start of the function.",
                        ref=".antigravity/rules/02-multiplayer-conventions.md#6",
                    )


def run_verification(
    source_path: Path,
    target_file: str | None = None,
    baseline_path: Path | None = DEFAULT_BASELINE_PATH,
    ignore_baseline: bool = False,
) -> dict:
    verifier = ReplicationVerifier(
        source_path,
        target_file=target_file,
        baseline_path=baseline_path,
        ignore_baseline=ignore_baseline,
    )
    verifier.scan_all()

    error_count = sum(1 for v in verifier.violations if v["severity"] == "ERROR")
    warning_count = sum(1 for v in verifier.violations if v["severity"] == "WARNING")
    status = "PASSED" if error_count == 0 else "FAILED"

    return {
        "status": status,
        "scoped_file": target_file,
        "total_violations": len(verifier.violations),
        "error_count": error_count,
        "warning_count": warning_count,
        "baseline_suppressed": verifier.baseline_suppressed_count,
        "violations": verifier.violations,
        "summary": (
            f"Replication analysis {status.lower()}: {error_count} error(s), {warning_count} warning(s)"
            + (
                f" ({verifier.baseline_suppressed_count} frozen legacy baseline item(s))"
                if verifier.baseline_suppressed_count
                else ""
            )
            + "."
        ),
    }


def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Static Replication & Anti-Slop Verifier")
    parser.add_argument("--source", type=str, default="Source", help="Path to Source directory (default: Source)")
    parser.add_argument("--file", "-f", type=str, default=None, help="Optional single file path for incremental ACI check")
    parser.add_argument("--baseline", type=str, default=str(DEFAULT_BASELINE_PATH), help="Path to legacy baseline JSON")
    parser.add_argument("--ignore-baseline", action="store_true", help="Report all frozen legacy baseline items too")
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
            "violations": [
                {
                    "rule": "SOURCE_DIR_NOT_FOUND",
                    "file": str(source_dir),
                    "line": 0,
                    "severity": "ERROR",
                    "message": f"Source directory '{source_dir}' does not exist.",
                }
            ],
            "summary": "Failed: Source directory not found.",
        }
        print(json.dumps(result, indent=2 if args.pretty else None))
        sys.exit(1)

    result = run_verification(
        source_dir,
        target_file=args.file,
        baseline_path=Path(args.baseline) if args.baseline else None,
        ignore_baseline=args.ignore_baseline,
    )
    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "PASSED" else 1)


if __name__ == "__main__":
    main()
