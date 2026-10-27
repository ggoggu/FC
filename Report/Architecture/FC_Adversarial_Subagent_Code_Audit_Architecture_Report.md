# 07. 독립 서브에이전트 기반 적대적 코드 심사 및 5대 최적화 검증 체계 구현 보고서
**FC 프로젝트 (Unreal Engine 5.8 Dedicated Server & Google Antigravity)**

---

## Executive Summary (요약)

본 문서는 Lauren Tan(SpaceX/Cursor)의 `pstack` 멀티모델 교차 검증 패러다임(`/interrogate`, `/arena`)을 분석하고, 이를 **Unreal Engine 5.8 전용 데디케이트 서버 복제 아키텍처**와 **Google Antigravity 서브에이전트 시스템**에 특화하여 이식·구축한 **적대적 코드 심사관(Adversarial Code Auditor)** 체계의 최종 완성 보고서입니다.

단일 생성 AI 모델이 코드를 작성할 때 필연적으로 발생하는 **인지적 맹점(Blind Spot), 오버엔지니어링, 네트워크 복제 누락, 불필요한 틱 활성화** 문제를 원천 차단하기 위해, 코드 수정 권한이 배제된 독립된 **Read-Only Auditor 서브에이전트(`adversarial-auditor`)**를 배치하고, **5대 핵심 차원(간결성, 기존 구조 활용, 범위 내 적정 확장성, 네트워크 최적화, 일반 최적화)**을 기준으로 엄격하게 코드를 심사하도록 구현했습니다.

특히 초기 설계 단계를 넘어, 실전 환경 점검을 통해 **① `pipeline.py audit` CLI 모드 연계**, **② Untracked C++ 파일 자동 감지(`git add -N .`) 및 바이너리(.uasset) 배제 필터링**, **③ 온디맨드 자동 티어링(`inherit` 기본, '자동 티어링' 요청 시에만 diff 기반 `pro`/`flash` 자동 선정)**, **④ 자율 치유(Closed-Loop Auto-Remediation) 폐루프**까지 완전히 구축되었으며, 실제 코드(`FCDoorBase.cpp`)를 대상으로 7개 결함 탐지 후 10/10 LGTM 승인까지 완벽히 실증 검증되었습니다.

---

## 1. 배경 및 도입 목적

### 1.1 단일 생성 모델(Single Agent)의 한계
* **확증 편향(Confirmation Bias)**: 코드를 생성한 동일한 컨텍스트 내에서는 자신이 범한 미묘한 실수(예: 소유권 없는 액터에서의 RPC 호출, 임시 변수 남발 등)를 인지하기 어렵습니다.
* **불필요한 과설계(Over-Engineering)**: 단일 프롬프트에 답변하는 과정에서 요구되지 않은 과도한 추상 계층(Wrapper, 불필요한 Factory/Manager)을 만들어 코드베이스를 비대하게 만듭니다.

### 1.2 언리얼 엔진 5.8 멀티플레이어 환경의 특수성
* C++20 컴파일러나 일반 린터는 문법적 오류는 잡을 수 있으나, **네트워크 권한 분리(`HasAuthority`), RPC 소유권(Connection Ownership), 복제 조건(`COND_OwnerOnly`), 가비지 컬렉션(`TObjectPtr`) 수명 주기**와 같은 도메인 특화 결함을 사전에 탐지하지 못합니다.
* 따라서 코드가 병합(Commit)되기 전, 프로젝트 규칙(`AGENTS.md`)을 완벽히 숙지한 **독립된 심사 서브에이전트가 `git diff`를 교차 검증하는 메커니즘**이 필수적입니다.

---

## 2. 아키텍처 설계 및 자율 치유 폐루프 (Closed-Loop)

```
+-----------------------------------------------------------------------------------+
|                           MAIN AGENT (Lead Developer)                             |
|  - Feature Implementation & C++20 Refactoring                                     |
|  - Preprocessing: `git add -N .` + Filter C++ diff (`-- *.h *.cpp *.cs`)          |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 1. Git Diff & Stated Intent Delivery
                                         |    (invoke_subagent / /interrogate)
                                         v
+-----------------------------------------------------------------------------------+
|                      ADVERSARIAL AUDITOR (Read-Only Subagent)                     |
|  - Model Mode: `inherit` (기본값) / '자동 티어링' 요청 시에만 diff 기반 자동 선정         |
|  - Write Tools: NONE (Strictly Forbidden from modifying files)                    |
|  - Rubric: 5 Core Audit Dimensions (Rules 07 & AGENTS.md)                         |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 2. Structured Findings Matrix
                                         |    (BLOCKING / WARNING / SUGGESTION)
                                         v
+-----------------------------------------------------------------------------------+
|                       CLOSED-LOOP AUTONOMOUS REMEDIATION                          |
|  - Auto-patch BLOCKING issues (HasAuthority, BlueprintSetter, COND_InitialOnly)   |
|  - Verify Compilation via `python Scripts/pipeline.py static` & `pipeline.py build`|
|  - Re-Audit Pass -> Achieve 10/10 APPROVED (LGTM)                                |
+-----------------------------------------------------------------------------------+
```

### 핵심 보안 및 격리 원칙:
1. **Read-Only 강제 (`EnableWriteTools: false`)**:
   - 감사관 서브에이전트는 파일 생성, 수정, 명령 실행 권한이 없습니다. 오직 코드와 설계를 읽고 비판하는 일에만 집중하여 환각으로 인한 코드 오염을 원천 차단합니다.
2. **의도와 구현의 분리 (Separation of Intent & Implementation)**:
   - 메인 에이전트가 구현 의도(Stated Intent)를 명시하고, 서브에이전트는 "그 의도를 가장 간결하고 안전하게 달성했는가"만을 판정합니다.
3. **토큰 절약 및 필터링 원칙**:
   - `.uasset`, `.umap` 등 바이너리 파일 diff를 철저히 배제하고 순수 C++ 텍스트 diff만 전달하여 컨텍스트 낭비를 방지합니다.

---

## 3. 5대 핵심 심사 차원 (The 5 Core Audit Dimensions)

서브에이전트는 다음 5개 축을 기준으로 점수 및 결함을 도출합니다:

### ① 코드 간결성 (Code Conciseness)
* **최소 수정 원칙 (Minimal Surface Area)**: 기능 구현 또는 버그 수정에 꼭 필요한 코드 라인만 추가/수정되었는가?
* **과설계 배제**: 1회성 로직을 위해 불필요한 래퍼 함수나 인터페이스를 남발하지 않고, 헤더 초기화 등 명료한 최신 C++20 관용구를 사용하는가?

### ② 기존 구조 및 코드 활용 (Maximal Architecture Reuse)
* **기존 자산 활용**: `Source/FC/` 내에 이미 존재하는 게임플레이 서브시스템, 유틸리티 스탯틱스(`FCCombatStatics` 등), 컴포넌트, 데이터 구조를 확인하고 재사용했는가?
* **중복 구조 차단**: 기존에 구축된 데이터 파이프라인이나 이벤트 디스패처가 있음에도 새로운 평행 구조(Parallel Workflow)를 생성하지 않았는가?
* **엔진 에셋 SSOT 준수**: 게임플레이 밸런스/설정 데이터가 loose file(`*.csv`) I/O가 아닌 `UDataTable` / `UPrimaryDataAsset`을 통해 주입되는가?

### ③ 범위 내 적정 확장성 (Scoped Extensibility)
* **스펙 맞춤 설계**: 새로운 구조나 클래스를 도입할 때, **현재 기획서/스펙에서 요구하는 범위에 최적화된 유연성**을 가졌는가?
* **YAGNI(You Aren't Gonna Need It) 원칙**: 현재 요구되지 않는 가상의 미래 요구사항을 대비한다는 명목으로 추상화 계층을 덧씌우지 않았는가?
* **모듈형 잎새 확장**: 컴포넌트 Extent를 하드코딩으로 덮어쓰지 않고 보존하며, 가상 함수(`virtual`)를 통해 자식 클래스의 확장을 열어두었는가?

### ④ 네트워크 및 복제 최적화 (Network & Replication)
* **권한 분리(Authority)**: 모든 전투 연산, 대미지 처리, 인벤토리/체력 변조가 `HasAuthority()` 또는 `ROLE_Authority`로 엄격히 보호되는가?
* **RPC 소유권(Ownership) 및 캡슐화**: `BlueprintSetter`를 통해 블루프린트 그래프에서의 변조 시에도 C++ 권한 가드를 강제하는가?
* **복제 대역폭 최적화**: 모든 복제 프로퍼티에 `DOREPLIFETIME_CONDITION` (`COND_InitialOnly`, `COND_OwnerOnly`)이 적용되었는가?
* **RepNotify 우선**: 틱 단위 고빈도 RPC를 지양하고 `ReplicatedUsing = OnRep_*` 상태 동기화 패턴을 채택했는가?

### ⑤ 일반 성능 및 메모리 최적화 (General Optimization)
* **Tick 위생**: `PrimaryActorTick.bCanEverTick = false`를 기본값으로 유지하는가?
* **핫패스 물리/메모리 절약**: `FMath::IsNearlyEqual` 등을 사용하여 동일 값 변경 시 불필요한 브로드페이즈 물리 씬 더티 및 Overlap 갱신을 차단했는가?
* **메모리 및 GC 안전성**: 모든 UObject 멤버 변수에 `TObjectPtr<T>`를 사용하고, 순환 참조를 방지하기 위해 비소유 캐시 참조에는 `TWeakObjectPtr<T>`를 적용했는가?

---

## 4. 실전 검증 사례: FCDoorBase 결함 탐지 및 10/10 완치

실제 프로젝트 코드인 [`Source/FC/Private/Map/FCDoorBase.cpp`](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Source/FC/Private/Map/FCDoorBase.cpp)의 변경 사항을 대상으로 실전 심사를 진행하여 얻은 실증 데이터입니다:

### 4.1 1차 심사 결과 (CHANGES REQUESTED)
| Severity | Dimension | Issue & Root Cause | Actionable Fix |
| :--- | :--- | :--- | :--- |
| **BLOCKING** | Network | `SetDoorWidth`에 `HasAuthority()` 부재로 클라이언트 조작 위험 | `if (!HasAuthority()) return;` 추가 |
| **BLOCKING** | Network | `DoorWidth`가 `BlueprintReadWrite`로 노출되어 세터 우회 결함 | `BlueprintSetter = SetDoorWidth` 강제 |
| **WARNING** | Network | 정적 던전 문에 `COND_None` 사용하여 매 틱 더티 체크 낭비 | `COND_InitialOnly`로 변경 |
| **WARNING** | Extensibility | 하드코딩된 X, Z 박스 범위로 디자이너 커스텀 설정 파괴 | `GetUnscaledBoxExtent()` 보존 |
| **SUGGESTION**| Optimization | 동일 값 입력 시 불필요한 물리 씬 Overlap 연산 발생 | `FMath::IsNearlyEqual` 조기 반환 추가 |

### 4.2 자율 치유 및 재심사 결과 (APPROVED - LGTM)
* **UBT 컴파일 검증**: `python Scripts/pipeline.py build` 무오류 통과 (0 error, 0 warning in 31.38s).
* **서브에이전트 재심사 점수**:
  - Conciseness: **10/10** | Architecture Reuse: **10/10** | Scoped Extensibility: **10/10**
  - Network Optimization: **10/10** | General Optimization: **10/10**
  - **Verdict**: **APPROVED (LGTM)** 전 항목 만점 승인.

---

## 5. 최종 구현 파일 및 통합 체계

```text
FC/
├── .agents/
│   └── skills/
│       └── interrogate/
│           └── SKILL.md            <-- /interrogate 스킬 정의 (C++ 필터링 & 모델 티어링)
├── .antigravity/
│   └── rules/
│       └── 07-adversarial-audit-standards.md  <-- 5대 심사 기준 공식 도메인 룰
├── AGENTS.md                       <-- 파이프라인 모드 7번(audit) 및 표준 개발 지침 연계
├── Scripts/
│   └── pipeline.py                 <-- python Scripts/pipeline.py audit (C++ diff 패키징 모드)
└── Report/
    └── Architecture/
        ├── FC_Adversarial_Subagent_Code_Audit_Architecture_Report.md
        ├── FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf
        ├── FC_SubAgent_Addition_Rules_Skills_Impact_Report.pdf
        └── FC_Agent_Rules_Skills_MCP_Architecture_Report.pdf
```

---

## 6. 결론

FC 프로젝트의 독립 서브에이전트 감사 환경은 **이론적 설계를 넘어, 엔진 컴파일러와 정적 검사기, 독립 심사관이 유기적으로 맞물려 작동하는 자율 치유 엔지니어링 파이프라인**으로 최종 완성되었습니다.
