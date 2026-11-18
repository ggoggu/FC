#!/usr/bin/env python3
"""
Unreal Engine 5.8 Multiplayer & Replication Static Verification Script
Scans C++ headers and source files in Source/ to identify multiplayer/replication anti-patterns and violations.
Supports both full-repository scans and incremental single-file ACI checks (--file).
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
    r"MVVMViewModelBase\.h",
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

class ReplicationVerifier:
    def __init__(self, source_dir: Path, target_file: str | None = None):
        self.source_dir = source_dir
        self.target_file = target_file
        self.violations = []
        self.math_payload_structs = set()
        self.server_rpcs_by_class = {}
        self.authority_only_methods_by_class = {}

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
                if ch == '\\':
                    i += 2
                    continue
                if ch == '"':
                    in_string = False
            elif in_char:
                if ch == '\\':
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
                        return text[open_pos + 1:i], i + 1
            i += 1
        return text[open_pos + 1:], n

    def extract_brace_block(self, text: str, open_brace_pos: int) -> tuple[str, int]:
        return self.extract_delimited_block(text, open_brace_pos, '{', '}')

    def extract_paren_block(self, text: str, open_paren_pos: int) -> tuple[str, int]:
        return self.extract_delimited_block(text, open_paren_pos, '(', ')')

    def collect_payload_structs(self):
        """Scans all headers in Source/ for USTRUCTs containing floating-point/vector or dynamic container fields (transitive)."""
        self.math_payload_structs = set()
        self.container_payload_structs = set()
        math_field_pat = re.compile(
            r'\b(FVector(?:_NetQuantize\d*)?|FVector2D|FRotator|FTransform|float|double)\b'
        )
        container_field_pat = re.compile(r'\b(TArray|TSet|TMap)\s*<|\bFString\b')
        struct_header_pat = re.compile(r'USTRUCT\s*\([^)]*\)\s*struct\s+(?:\w+_API\s+)?(?P<sname>[A-Z]\w+)[^{;]*\{')
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

        # Transitive closure for nested USTRUCT payloads
        changed = True
        while changed:
            changed = False
            for sname, body in struct_bodies.items():
                if sname not in self.math_payload_structs and any(re.search(rf'\b{m}\b', body) for m in self.math_payload_structs):
                    self.math_payload_structs.add(sname)
                    changed = True
                if sname not in self.container_payload_structs and any(re.search(rf'\b{c}\b', body) for c in self.container_payload_structs):
                    self.container_payload_structs.add(sname)
                    changed = True

    def scan_all(self):
        self.collect_payload_structs()
        self.server_rpcs_by_class = {}
        self.authority_only_methods_by_class = {}

        header_files, cpp_files = resolve_target_files(self.source_dir, self.target_file)

        # Map classes to their headers and cpp implementations
        for h_file in header_files:
            self.verify_header_file(h_file)

        for cpp_file in cpp_files:
            self.verify_cpp_file(cpp_file)

    def verify_header_file(self, h_file: Path):
        try:
            content = h_file.read_text(encoding="utf-8", errors="replace")
        except Exception:
            return

        lines = content.splitlines()
        clean_content = self.strip_comments(content)

        # Check 0: Escape-Hatch Comments (// TODO, // FIXME, // HACK, // XXX)
        for idx, line in enumerate(lines, start=1):
            if re.search(r'//\s*(TODO|FIXME|HACK|XXX)\b', line, re.IGNORECASE):
                self.add_violation(
                    rule_id="CODE_ESCAPE_HATCH_COMMENT",
                    file_path=h_file,
                    line_number=idx,
                    severity="WARNING",
                    message=f"Escape-hatch comment detected ('{line.strip()}'). Resolve incomplete logic or promote constraints instead of leaving TODO/FIXME/HACK comments."
                )

        # Check 1: Forbidden UI Headers in Actor/Gameplay Headers
        is_actor_or_gameplay_header = bool(re.search(r'class\s+\w+_API\s+([A-Z]\w+)\s*:\s*public\s+(AActor|ACharacter|APawn|AGameMode|AGameModeBase|AGameState|AGameStateBase|APlayerState|AAIController|UActorComponent)', clean_content))
        is_ui_or_vm_header = bool(re.search(r'class\s+\w+_API\s+([A-Z]\w+)\s*:\s*public\s+(UUserWidget|UMVVMViewModelBase)', clean_content))

        if is_ui_or_vm_header:
            for idx, line in enumerate(lines, start=1):
                clean_line = line.split('//')[0]
                if re.search(r'#include\s*["<][^">]*GameMode[^">]*[">]', clean_line):
                    self.add_violation(
                        rule_id="UI_GAMEMODE_DEPENDENCY",
                        file_path=h_file,
                        line_number=idx,
                        severity="ERROR",
                        message=f"Client-only UI/ViewModel header includes server GameMode header: '{line.strip()}'."
                    )
        
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

        # Check 2: Server RPC missing WithValidation & Reliable NetMulticast check
        # Regex to find UFUNCTION(...) declarations and the following method signature
        ufunc_pattern = re.compile(
            r'UFUNCTION\s*\((?P<specs>[^)]*)\)\s*(?:virtual\s+)?(?:[\w:<>*&\s]+?)\s+(?P<funcname>\w+)\s*\((?P<params>[^)]*)\)\s*(?P<qualifiers>[^;{]*)(?P<term>;|\{)',
            re.DOTALL
        )
        for match in ufunc_pattern.finditer(clean_content):
            specifiers = match.group("specs")
            func_name = match.group("funcname")
            params = match.group("params").strip()
            qualifiers = match.group("qualifiers")
            term = match.group("term")
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
                # Find enclosing class name (closest preceding class declaration)
                preceding = clean_content[:start_pos]
                cls_matches = list(re.finditer(r'class\s+\w+_API\s+(?P<classname>[A-Z]\w+)', preceding))
                if cls_matches:
                    owner_class = cls_matches[-1].group("classname")
                    self.server_rpcs_by_class.setdefault(owner_class, []).append({
                        "name": func_name,
                        "params": params,
                        "line": line_no,
                        "header": h_file
                    })

            if re.search(r'\bNetMulticast\b', specifiers, re.IGNORECASE) and re.search(r'\bReliable\b', specifiers, re.IGNORECASE):
                self.add_violation(
                    rule_id="NET_SEC_RELIABLE_MULTICAST",
                    file_path=h_file,
                    line_number=line_no,
                    severity="WARNING",
                    message=f"NetMulticast RPC '{func_name}' is marked 'Reliable'. Transient cosmetics should be 'Unreliable' to prevent replication queue stalls; use ReplicatedUsing for persistent state."
                )

            if re.search(r'\bBlueprintAuthorityOnly\b', specifiers, re.IGNORECASE):
                is_const = bool(re.search(r'\bconst\b', qualifiers))
                is_pure_getter = bool(re.match(r'^(Get|Is|Has|Can)[A-Z]', func_name))
                if not is_const and not is_pure_getter and not re.search(r'\bServer\b', specifiers, re.IGNORECASE):
                    preceding = clean_content[:start_pos]
                    cls_matches = list(re.finditer(r'class\s+\w+_API\s+(?P<classname>[A-Z]\w+)', preceding))
                    if cls_matches:
                        owner_class = cls_matches[-1].group("classname")
                        if term == '{':
                            inline_body, _ = self.extract_brace_block(clean_content, match.end() - 1)
                            if not re.search(r'\bHasAuthority\s*\(|\bROLE_Authority\b|\bGetAuthGameMode\s*\(|\bNM_Client\b|\bIsRunningDedicatedServer\s*\(', inline_body):
                                self.add_violation(
                                    rule_id="NET_SEC_AUTHORITY_ONLY_MISSING_CPP_GUARD",
                                    file_path=h_file,
                                    line_number=line_no,
                                    severity="WARNING",
                                    message=f"Inline BlueprintAuthorityOnly method '{owner_class}::{func_name}' lacks an internal HasAuthority() guard; C++ callers can execute it on non-authoritative clients."
                                )
                        else:
                            self.authority_only_methods_by_class.setdefault(owner_class, []).append({
                                "name": func_name,
                                "line": line_no,
                                "header": h_file
                            })

        # Check: Server RPC declared on Actor not owned by connection
        for class_match in re.finditer(r'class\s+\w+_API\s+(?P<classname>[A-Z]\w+)\s*:\s*public\s+(?P<parent>\w+)[^{]*\{(?P<body>.*?)\};', clean_content, re.DOTALL):
            c_name = class_match.group("classname")
            c_parent = class_match.group("parent")
            c_body = class_match.group("body")
            is_connection_owned = any(base in c_parent for base in ["PlayerController", "Controller", "Pawn", "Character", "PlayerState", "ActorComponent", "Component"])
            if not is_connection_owned:
                for rpc_match in re.finditer(r'UFUNCTION\s*\([^)]*\bServer\b[^)]*\)[^;]*?\s+(?P<rpcname>\w+)\s*\(', c_body, re.IGNORECASE):
                    rpc_name = rpc_match.group("rpcname")
                    self.add_violation(
                        rule_id="REPL_SERVER_RPC_UNOWNED_ACTOR",
                        file_path=h_file,
                        line_number=content[:class_match.start() + rpc_match.start()].count('\n') + 1,
                        severity="ERROR",
                        message=f"Server RPC '{rpc_name}' declared on '{c_name}' (parent '{c_parent}'). Clients can ONLY invoke Server RPCs on actors owned by their connection (PlayerController, PlayerState, Pawn, or their components)."
                    )

        # Check 3: RepNotify naming conventions, Replicated without GetLifetimeReplicatedProps,
        #          Promoted Rule 01 (Raw UObject* in UPROPERTY), Promoted Rule 06 (MVVM purity),
        #          and Promoted Rule 07 (BlueprintSetter on Replicated BlueprintReadWrite)
        class_pattern = re.compile(r'class\s+\w+_API\s+(?P<classname>[A-Z]\w+)\s*(?::\s*public\s+(?P<parent>\w+))?[^{]*\{(?P<body>.*?)\};', re.DOTALL)
        for class_match in class_pattern.finditer(clean_content):
            class_name = class_match.group("classname")
            parent_name = class_match.group("parent") or ""
            class_body = class_match.group("body")
            class_start_pos = class_match.start()
            class_line_no = content[:class_start_pos].count('\n') + 1
            is_viewmodel_class = (parent_name == "UMVVMViewModelBase" or class_name.endswith("ViewModel"))

            has_replicated_props = False
            
            # Search for UPROPERTY declarations using balanced-paren extraction (supports nested meta=(...))
            for uprop_kw in re.finditer(r'\bUPROPERTY\s*\(', class_body):
                prop_specs, after_paren = self.extract_paren_block(class_body, uprop_kw.end() - 1)
                decl_match = re.match(r'\s*(?P<proptype>[A-Za-z0-9_<>\s*&:,]+?)\s+(?P<varname>\w+)\s*(?:=\s*[^;]+)?;', class_body[after_paren:], re.DOTALL)
                if not decl_match:
                    continue
                prop_type = decl_match.group("proptype").strip()
                var_name = decl_match.group("varname")
                prop_pos = class_start_pos + uprop_kw.start()
                prop_line_no = content[:prop_pos].count('\n') + 1

                # Promoted Memory Check (Rule 01): Raw UObject/AActor pointer in UPROPERTY instead of TObjectPtr<T>
                if prop_type.endswith("*") and re.match(r'^(?:const\s+)?[UA][A-Z]\w*\s*\*$', prop_type):
                    self.add_violation(
                        rule_id="UE58_RAW_POINTER_UPROPERTY",
                        file_path=h_file,
                        line_number=prop_line_no,
                        severity="ERROR",
                        message=f"Raw UObject pointer '{prop_type} {var_name}' in UPROPERTY. Use 'TObjectPtr<{prop_type[:-1].strip()}>' per UE 5.8 C++20 coding standards."
                    )

                is_replicated = bool(re.search(r'\bReplicated\b', prop_specs, re.IGNORECASE))
                rep_using_match = re.search(r'ReplicatedUsing\s*=\s*(?P<funcname>\w+)', prop_specs, re.IGNORECASE)

                # Promoted MVVM Checks (Rule 06): No replication on ViewModels, and soft texture references only
                if is_viewmodel_class:
                    if is_replicated or rep_using_match:
                        self.add_violation(
                            rule_id="MVVM_REPLICATED_FORBIDDEN",
                            file_path=h_file,
                            line_number=prop_line_no,
                            severity="ERROR",
                            message=f"ViewModel '{class_name}' property '{var_name}' is marked Replicated. UMVVMViewModelBase is client-only presentation state and must never replicate."
                        )
                    if re.search(r'\bTObjectPtr\s*<\s*UTexture2D\s*>|\bUTexture2D\s*\*', prop_type):
                        self.add_violation(
                            rule_id="MVVM_HARD_TEXTURE_REF",
                            file_path=h_file,
                            line_number=prop_line_no,
                            severity="ERROR",
                            message=f"ViewModel '{class_name}' property '{var_name}' uses hard texture reference '{prop_type}'. Use 'TSoftObjectPtr<UTexture2D>' per Rule 06."
                        )

                if is_replicated or rep_using_match:
                    has_replicated_props = True

                    # Promoted Memory Check (Rule 07 / Adversarial Audit): Replicated + BlueprintReadWrite bypassing non-inline authority setter Set<VarName>
                    if re.search(r'\bBlueprintReadWrite\b', prop_specs) and not re.search(r'\bBlueprintSetter\s*=', prop_specs):
                        clean_var = var_name[1:] if (var_name.startswith("b") and len(var_name) > 1 and var_name[1].isupper()) else var_name
                        setter_name = f"Set{clean_var}"
                        if re.search(rf'\bvoid\s+{setter_name}\s*\([^)]*\)\s*;', class_body):
                            self.add_violation(
                                rule_id="REPL_BYPASSED_AUTHORITY_SETTER",
                                file_path=h_file,
                                line_number=prop_line_no,
                                severity="ERROR",
                                message=f"Replicated property '{var_name}' is marked 'BlueprintReadWrite' without 'BlueprintSetter = {setter_name}', allowing Blueprint graphs to bypass '{setter_name}()' authority checks."
                            )

                if rep_using_match:
                    func_name = rep_using_match.group("funcname")
                    clean_var = var_name[1:] if (var_name.startswith("b") and len(var_name) > 1 and var_name[1].isupper()) else var_name
                    valid_names = {f"OnRep_{var_name}", f"OnRep_{clean_var}"}
                    is_declared_in_class = bool(re.search(rf'\bvoid\s+{func_name}\s*\(', class_body))
                    if func_name not in valid_names and not (func_name.startswith("OnRep_") and is_declared_in_class):
                        self.add_violation(
                            rule_id="REPL_INVALID_REPNOTIFY_NAME",
                            file_path=h_file,
                            line_number=prop_line_no,
                            severity="WARNING",
                            message=f"Replication callback '{func_name}' for variable '{var_name}' does not match standard convention 'OnRep_{clean_var}' or is missing declaration."
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

        # Check 7: OpenLevel in GameMode
        if "GameMode" in cpp_file.stem:
            for idx, line in enumerate(lines, start=1):
                if re.search(r'UGameplayStatics::OpenLevel\b', line):
                    self.add_violation(
                        rule_id="REPL_OPENLEVEL_IN_GAMEMODE",
                        file_path=cpp_file,
                        line_number=idx,
                        severity="ERROR",
                        message="UGameplayStatics::OpenLevel called in GameMode. Use GetWorld()->ServerTravel(MapURL, false) to preserve multiplayer client sessions."
                    )

        # Check 8: Loose CSV file reads
        for idx, line in enumerate(lines, start=1):
            if re.search(r'FPaths::ProjectContentDir\(\)\s*/\s*TEXT\([^)]*\.csv', line):
                self.add_violation(
                    rule_id="DATA_RUNTIME_CSV_READ",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="ERROR",
                    message="Direct runtime disk CSV access detected. Cooked/packaged builds do not package loose CSVs. Reference UDataTable or UPrimaryDataAsset directly."
                )

        # Check 9: ConstructorHelpers::FObjectFinder with /Game/ in constructor
        for idx, line in enumerate(lines, start=1):
            if re.search(r'ConstructorHelpers::FObjectFinder<[^>]+>\s+\w+\s*\(\s*TEXT\(\s*"/Game/', line):
                self.add_violation(
                    rule_id="CODE_HARDCODED_ASSET_PATH",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="WARNING",
                    message="Hardcoded asset path in constructor using ConstructorHelpers. Configure assets via DataAsset, TSoftObjectPtr, or Blueprint Class Defaults."
                )
            if re.search(r'//\s*(TODO|FIXME|HACK|XXX)\b', line, re.IGNORECASE):
                self.add_violation(
                    rule_id="CODE_ESCAPE_HATCH_COMMENT",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="WARNING",
                    message=f"Escape-hatch comment detected ('{line.strip()}'). Resolve incomplete logic or promote constraints instead of leaving TODO/FIXME/HACK comments."
                )

        # Check 9b: Banned Footgun APIs (Index-0 Player getters & Raw 'this' AddLambda) and Server .cpp UI includes
        is_server_only_cpp = any(tag in cpp_file.stem for tag in ["GameMode", "AIController"])
        is_ui_or_vm_cpp = any(tag in cpp_file.stem for tag in ["Widget", "ViewModel"])
        for idx, line in enumerate(lines, start=1):
            clean_line = line.split('//')[0]
            if re.search(r'UGameplayStatics::(GetPlayerController|GetPlayerPawn|GetPlayerCharacter)\s*\([^,]+,\s*0\s*\)', clean_line):
                self.add_violation(
                    rule_id="CODE_BANNED_INDEX0_PLAYER_API",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="ERROR",
                    message="Banned single-player index-0 API (GetPlayerController/Pawn/Character(..., 0)). Use explicit ownership (GetController(), GetOwningPlayer(), or RPC context)."
                )
            if re.search(r'\.AddLambda\s*\(\s*\[[^\]]*\bthis\b', clean_line):
                self.add_violation(
                    rule_id="CODE_UNSAFE_RAW_THIS_LAMBDA",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="ERROR",
                    message="Unsafe '.AddLambda([this])' delegate binding detected. Use 'AddWeakLambda(this, ...)' to prevent dangling UObject crashes."
                )
            if is_server_only_cpp:
                for ui_hdr_pat in FORBIDDEN_UI_HEADERS:
                    if re.search(rf'#include\s*["<]{ui_hdr_pat}[">]', clean_line):
                        self.add_violation(
                            rule_id="REPL_UI_HEADER_IN_SERVER_CPP",
                            file_path=cpp_file,
                            line_number=idx,
                            severity="ERROR",
                            message=f"Forbidden UI/ViewModel header included in server-only source file: '{line.strip()}'."
                        )
            if is_ui_or_vm_cpp and re.search(r'#include\s*["<][^">]*GameMode[^">]*[">]', clean_line):
                self.add_violation(
                    rule_id="UI_GAMEMODE_DEPENDENCY",
                    file_path=cpp_file,
                    line_number=idx,
                    severity="ERROR",
                    message=f"Client-only UI/ViewModel source file includes server GameMode header: '{line.strip()}'."
                )

        # Check 10: Trivial Server RPC _Validate (accepts parameters with no conditional rejection)
        validate_pat = re.compile(r'(?P<cls>[A-Z]\w+)::(?P<func>\w+)_Validate\s*\((?P<params>[^)]*)\)\s*\{')
        for val_match in validate_pat.finditer(clean_content):
            params_str = val_match.group("params").strip()
            if params_str and params_str != "void":
                body, _ = self.extract_brace_block(clean_content, val_match.end() - 1)
                has_conditional_or_expr = bool(re.search(r'\bif\b|\bswitch\b|\?|\breturn\s+(?!true\s*;)', body))
                if not has_conditional_or_expr:
                    line_no = content[:val_match.start()].count('\n') + 1
                    self.add_violation(
                        rule_id="NET_SEC_TRIVIAL_RPC_VALIDATE",
                        file_path=cpp_file,
                        line_number=line_no,
                        severity="WARNING",
                        message=f"Server RPC '{val_match.group('cls')}::{val_match.group('func')}_Validate' accepts client parameters but unconditionally returns 'true'. Validate bounds, NaN, enum ranges, or ownership."
                    )

        # Check 11 & 12: Server RPC float/vector NaN sanitization & container bounds checks
        math_type_pat = re.compile(r'\b(FVector(?:_NetQuantize\d*)?|FVector2D|FRotator|FTransform|float|double)\b')
        container_type_pat = re.compile(r'\b(TArray|TSet|TMap)\s*<|\bFString\b')
        for cls_name, rpc_list in self.server_rpcs_by_class.items():
            if f"{cls_name}::" not in clean_content:
                continue
            for rpc in rpc_list:
                rpc_name = rpc["name"]
                rpc_params = rpc["params"]

                val_m = re.search(rf'\b{cls_name}::{rpc_name}_Validate\s*\([^)]*\)\s*\{{', clean_content)
                impl_m = re.search(rf'\b{cls_name}::{rpc_name}_Implementation\s*\([^)]*\)\s*\{{', clean_content)
                val_body = self.extract_brace_block(clean_content, val_m.end() - 1)[0] if val_m else ""
                impl_body = self.extract_brace_block(clean_content, impl_m.end() - 1)[0] if impl_m else ""
                if not val_m and not impl_m:
                    continue

                report_pos = val_m.start() if val_m else impl_m.start()
                report_line = content[:report_pos].count('\n') + 1

                # Check math/float parameters or math-containing USTRUCTs
                has_math_param = bool(math_type_pat.search(rpc_params))
                if not has_math_param and self.math_payload_structs:
                    has_math_param = any(re.search(rf'\b{sname}\b', rpc_params) for sname in self.math_payload_structs)

                if has_math_param:
                    combined = val_body + "\n" + impl_body
                    if not re.search(r'\b(ContainsNaN|IsNaN|IsFinite)\s*\(', combined):
                        self.add_violation(
                            rule_id="NET_SEC_UNSANITIZED_FLOAT_OR_VECTOR_RPC",
                            file_path=cpp_file,
                            line_number=report_line,
                            severity="ERROR",
                            message=f"Server RPC '{cls_name}::{rpc_name}' accepts floating-point/vector payload without NaN/Inf sanitization (ContainsNaN/IsNaN/IsFinite)."
                        )

                # Check dynamic container / string parameters (direct or inside USTRUCT)
                has_container_param = bool(container_type_pat.search(rpc_params))
                if not has_container_param and getattr(self, "container_payload_structs", None):
                    has_container_param = any(re.search(rf'\b{sname}\b', rpc_params) for sname in self.container_payload_structs)
                if has_container_param:
                    combined_bounds = val_body + "\n" + impl_body
                    if not re.search(r'\.(Num|Len)\s*\(', combined_bounds):
                        self.add_violation(
                            rule_id="NET_SEC_UNBOUNDED_RPC_CONTAINER",
                            file_path=cpp_file,
                            line_number=report_line,
                            severity="WARNING",
                            message=f"Server RPC '{cls_name}::{rpc_name}' accepts dynamic container/FString without size/length bounds check (.Num()/.Len())."
                        )

        # Check 13: BlueprintAuthorityOnly non-const methods missing C++ HasAuthority() guard
        auth_guard_pat = re.compile(r'\bHasAuthority\s*\(|\bROLE_Authority\b|\bGetAuthGameMode\s*\(|\bNM_Client\b|\bIsRunningDedicatedServer\s*\(')
        for cls_name, method_list in self.authority_only_methods_by_class.items():
            if f"{cls_name}::" not in clean_content:
                continue
            for meth in method_list:
                m_name = meth["name"]
                def_m = re.search(rf'\b{cls_name}::{m_name}\s*\([^)]*\)\s*\{{', clean_content)
                if not def_m:
                    continue
                body, _ = self.extract_brace_block(clean_content, def_m.end() - 1)
                if not auth_guard_pat.search(body):
                    line_no = content[:def_m.start()].count('\n') + 1
                    self.add_violation(
                        rule_id="NET_SEC_AUTHORITY_ONLY_MISSING_CPP_GUARD",
                        file_path=cpp_file,
                        line_number=line_no,
                        severity="WARNING",
                        message=f"BlueprintAuthorityOnly method '{cls_name}::{m_name}' lacks a C++ HasAuthority() guard in its implementation."
                    )

def run_verification(source_path: Path, target_file: str | None = None) -> dict:
    verifier = ReplicationVerifier(source_path, target_file=target_file)
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
        "violations": verifier.violations,
        "summary": f"Replication analysis {status.lower()}: {error_count} error(s), {warning_count} warning(s)."
    }

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Static Replication Verifier")
    parser.add_argument("--source", type=str, default="Source", help="Path to Source directory (default: Source)")
    parser.add_argument("--file", "-f", type=str, default=None, help="Optional single file path for incremental ACI check")
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

    result = run_verification(source_dir, target_file=args.file)
    print(json.dumps(result, indent=2 if args.pretty else None))
    sys.exit(0 if result["status"] == "PASSED" else 1)

if __name__ == "__main__":
    main()
