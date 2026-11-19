# Antigravity 환경 구성 및 AI 하네스 종합 요약

본 문서는 **FC 프로젝트(Unreal Engine 5.8)**에 구축된 **Antigravity AI 하네스(규칙 · 스킬 · 훅 · 정적/동적 검증 파이프라인)**의 실제 구성을 정리한 기준 문서입니다.

---

## 1. 프로젝트 및 아키텍처 핵심

- **프로젝트 / 엔진**: `FC` (`FC.uproject`), Unreal Engine 5.8, Target `FCEditor` (Win64, C++20)
- **핵심 원칙**: **"Single-Player is Local Multiplayer"** (단독 플레이 = 로컬 Listen Server)
- **4계층 권한 분리**:
  1. Server Authority (`AGameModeBase`, `AAIController`, Subsystems) — `HasAuthority()`, Cooked Assets only
  2. Replicated State (`AGameStateBase`, `APlayerState`, `UActorComponent`) — `DOREPLIFETIME_CONDITION`, `FFastArraySerializer`
  3. Autonomous Client (`APlayerController`, `ACharacter`, Enhanced Input) — Connection-owned `Server_*` RPC (`WithValidation`)
  4. Presentation & UI (`AHUD`, `UUserWidget`, `UMVVMViewModelBase`) — Client-Only, `FieldNotify`, `TSoftObjectPtr<UTexture2D>`

---

## 2. 전체 하네스 구조 (Feedforward Guides + Computational/Inferential Sensors)

```
+------------------------------------------------------------------------------------+
|                           ANTIGRAVITY AGENT RUNTIME                                |
|  - Top-Level Map: AGENTS.md (~70 lines, Architecture & SSOT Map + Hard Constraints)|
|  - Auto-Discovered Skills (.agents/skills/): interrogate, ue-asset-inspector       |
|  - Native Lifecycle Hooks (.agents/hooks.json): PreToolUse Guard + Stop Gate       |
|  - Live Editor MCP: wildcard-mcp (FCTools, UMGToolSet, MVVMToolset, BlueprintTools)|
+-----------------------------------------+------------------------------------------+
                                          |
                 +------------------------+------------------------+
                 v                                                 v
+----------------------------------------+     +-------------------------------------+
|   On-Demand Domain Rules               |     |  Deterministic Pipeline & Sensors   |
|   (.antigravity/rules/01~08.md)        |     |  (Scripts/ + .git/hooks/pre-commit) |
+----------------------------------------+     +-------------------------------------+
| 01-ue58-coding-standards.md            |     | pipeline.py (Unified Runner + CB)   |
| 02-multiplayer-conventions.md          |     | verify_replication.py (+ baseline)  |
| 03-loop-engineering.md                 |     | verify_gas.py (Meta-Attr aware)     |
| 04-gameplay-ability-system.md          |     | audit_bandwidth.py (Balanced parse) |
| 05-combat-and-inventory-networking.md  |     | build_harness.py / run_tests.py     |
| 06-umg-mvvm-standards.md               |     | simulate_net_pie.py / dump_asset... |
| 07-adversarial-audit-standards.md      |     | tests/test_linters.py (17 tests)    |
| 08-sky-atmosphere-and-level-lighting.md|     | linter_baseline.json (19 frozen)    |
+----------------------------------------+     +-------------------------------------+
```

---

## 3. 자동 탐지 커스터마이징 (`.agents/`) & Git 훅

| 위치 | 유형 | 역할 |
| :--- | :--- | :--- |
| [AGENTS.md](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/AGENTS.md) | Always-On Map | 4계층 경계, 단일 검증 루프, Anti-Slop Hard Constraints, `Source/FC/Public/` 모듈·SSOT 지도 |
| [.agents/skills/interrogate/SKILL.md](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.agents/skills/interrogate/SKILL.md) | Auto-Discovered Skill | 읽기 전용 `adversarial-auditor` 서브에이전트 호출 (Proof-Carrying Audit) |
| [.agents/skills/ue-asset-inspector/SKILL.md](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.agents/skills/ue-asset-inspector/SKILL.md) | Auto-Discovered Skill | Live `wildcard-mcp` 우선 + 오프라인 `dump_asset_metadata.py` 폴백 `.uasset` 검사 |
| [.agents/hooks.json](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.agents/hooks.json) | Lifecycle Hooks | `PreToolUse` (`guard_verifiers.py`: 검증기·베이스라인 수정 시 `force_ask`) + `Stop` (`stop_static_gate.py`: 미검증 C++ 오류 시 종료 차단) |
| `.git/hooks/pre-commit` | Git Hook | 스테이징된 C++ 정적 검증 + 하네스 파일 변경 시 `Scripts/tests/test_linters.py` 자동 실행 |

---

## 4. 정적/동적 검증 스크립트 및 회귀 보호 (`Scripts/`)

- **정적 검증기 (Balanced Brace/Paren + Multi-line)**:
  - [verify_replication.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/verify_replication.py): 복제, RPC 소유권(`AAIController` 제외), 보안 검증, `CODE_UNSAFE_RAW_THIS_LAMBDA`, `CODE_ESCAPE_HATCH_COMMENT`, `CODE_HARDCODED_ASSET_PATH` (기존 19건은 [linter_baseline.json](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/linter_baseline.json) 동결, 신규 0건 강제)
  - [verify_gas.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/verify_gas.py): `ATTRIBUTE_ACCESSORS`, `OnRep_*`, `DOREPLIFETIME_CONDITION_NOTIFY`, `PreAttributeChange` NaN 체크, 비복제 Meta-Attribute(`NotReplicated` / `Meta` / `Damage`) 지원
  - [audit_bandwidth.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/audit_bandwidth.py): `UPROPERTY(..., meta=(...))` 포함 `TArray` 복제 차단 및 raw `DOREPLIFETIME` 차단
- **런타임 & 빌드 러너**:
  - [pipeline.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/pipeline.py) / [pipeline.ps1](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/pipeline.ps1) / [pipeline.bat](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/pipeline.bat): 단일 진입점 + 라인 번호 무관 SHA-256 서킷 브레이커
  - [build_harness.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/build_harness.py), [run_tests.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/run_tests.py) (0-Test PASS 금지), [simulate_net_pie.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/simulate_net_pie.py) (맵 실존 체크, Timeout=FAIL)
- **하네스 자가 회귀 테스트**:
  - [test_linters.py](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/tests/test_linters.py): `python Scripts/tests/test_linters.py` (17개 테스트, ~0.12s)
