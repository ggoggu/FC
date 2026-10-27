# 07. 독립 서브에이전트 기반 적대적 코드 심사 및 5대 최적화 검증 체계 구현 보고서
**FC 프로젝트 (Unreal Engine 5.8 Dedicated Server & Google Antigravity)**

---

## Executive Summary (요약)

본 문서는 Lauren Tan(SpaceX/Cursor)의 `pstack` 멀티모델 교차 검증 패러다임(`/interrogate`, `/arena`)을 분석하고, 이를 **Unreal Engine 5.8 전용 데디케이트 서버 복제 아키텍처**와 **Google Antigravity 서브에이전트 시스템**에 특화하여 이식·구축한 **적대적 코드 심사관(Adversarial Code Auditor)** 체계의 기술 보고서입니다.

단일 생성 AI 모델이 코드를 작성할 때 필연적으로 발생하는 **인지적 맹점(Blind Spot), 오버엔지니어링, 네트워크 복제 누락, 불필요한 틱 활성화** 문제를 원천 차단하기 위해, 코드 수정 권한이 배제된 독립된 **Read-Only Auditor 서브에이전트**를 배치하고, **5대 핵심 차원(간결성, 기존 구조 활용, 범위 내 적정 확장성, 네트워크 최적화, 일반 최적화)**을 기준으로 엄격하게 코드를 심사하도록 구현했습니다.

---

## 1. 배경 및 도입 목적

### 1.1 단일 생성 모델(Single Agent)의 한계
* **확증 편향(Confirmation Bias)**: 코드를 생성한 동일한 컨텍스트 내에서는 자신이 범한 미묘한 실수(예: 소유권 없는 액터에서의 RPC 호출, 임시 변수 남발 등)를 인지하기 어렵습니다.
* **불필요한 과설계(Over-Engineering)**: 단일 프롬프트에 답변하는 과정에서 요구되지 않은 과도한 추상 계층(Wrapper, 불필요한 Factory/Manager)을 만들어 코드베이스를 비대하게 만듭니다.

### 1.2 언리얼 엔진 5.8 멀티플레이어 환경의 특수성
* C++20 컴파일러나 일반 린터는 문법적 오류는 잡을 수 있으나, **네트워크 권한 분리(`HasAuthority`), RPC 소유권(Connection Ownership), 복제 조건(`COND_OwnerOnly`), 가비지 컬렉션(`TObjectPtr`) 수명 주기**와 같은 도메인 특화 결함을 사전에 탐지하지 못합니다.
* 따라서 코드가 병합(Commit)되기 전, 프로젝트 규칙(`AGENTS.md`)을 완벽히 숙지한 **독립된 심사 서브에이전트가 `git diff`를 교차 검증하는 메커니즘**이 필수적입니다.

---

## 2. 아키텍처 설계

```
+-----------------------------------------------------------------------------------+
|                           MAIN AGENT (Lead Developer)                             |
|  - Feature Implementation & C++20 Refactoring                                     |
|  - Has Write/Command Execution Tools                                              |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 1. Git Diff & Intent Delivery
                                         |    (invoke_subagent / /interrogate)
                                         v
+-----------------------------------------------------------------------------------+
|                      ADVERSARIAL AUDITOR (Read-Only Subagent)                     |
|  - Model Tier: Pro / Inherited (Deep Reasoning)                                   |
|  - Write Tools: NONE (Strictly Forbidden from modifying files)                    |
|  - Rubric: 5 Core Audit Dimensions (Rules 07 & AGENTS.md)                         |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 2. Structured Critique & Findings Matrix
                                         |    (APPROVED vs CHANGES REQUESTED)
                                         v
+-----------------------------------------------------------------------------------+
|                               TRIAGE & REFINEMENT                                 |
|  - Resolve BLOCKING Network / Architecture Violations                             |
|  - Verify Compilation via `python Scripts/pipeline.py static / build`             |
+-----------------------------------------------------------------------------------+
```

### 핵심 보안 및 격리 원칙:
1. **Read-Only 강제 (`EnableWriteTools: false`)**:
   - 감사관 서브에이전트는 파일 생성, 수정, 명령 실행 권한이 없습니다. 오직 코드와 설계를 읽고 비판하는 일에만 집중하여 환각으로 인한 코드 오염을 원천 차단합니다.
2. **의도와 구현의 분리 (Separation of Intent & Implementation)**:
   - 메인 에이전트가 구현 의도(Stated Intent)를 명시하고, 서브에이전트는 "그 의도를 가장 간결하고 안전하게 달성했는가"만을 판정합니다.

---

## 3. 5대 핵심 심사 차원 (The 5 Core Audit Dimensions)

서브에이전트는 다음 5개 축을 기준으로 점수 및 결함을 도출합니다:

### ① 코드 간결성 (Code Conciseness)
* **최소 수정 원칙 (Minimal Surface Area)**: 기능 구현 또는 버그 수정에 꼭 필요한 코드 라인만 추가/수정되었는가?
* **과설계 배제**: 1회성 로직을 위해 불필요한 래퍼 함수나 인터페이스를 남발하지 않고, 명료한 최신 C++20 관용구를 사용하는가?

### ② 기존 구조 및 코드 활용 (Maximal Architecture Reuse)
* **기존 자산 활용**: `Source/FC/` 내에 이미 존재하는 게임플레이 서브시스템, 유틸리티 스탯틱스(`FCCombatStatics` 등), 컴포넌트, 데이터 구조를 확인하고 재사용했는가?
* **중복 구조 차단**: 기존에 구축된 데이터 파이프라인이나 이벤트 디스패처가 있음에도 새로운 평행 구조(Parallel Workflow)를 생성하지 않았는가?
* **엔진 에셋 SSOT 준수**: 게임플레이 밸런스/설정 데이터가 loose file(`*.csv`) I/O가 아닌 `UDataTable` / `UPrimaryDataAsset`을 통해 주입되는가?

### ③ 범위 내 적정 확장성 (Scoped Extensibility)
* **스펙 맞춤 설계**: 새로운 구조나 클래스를 도입할 때, **현재 기획서/스펙에서 요구하는 범위에 최적화된 유연성**을 가졌는가?
* **YAGNI(You Aren't Gonna Need It) 원칙**: 현재 요구되지 않는 가상의 미래 요구사항을 대비한다는 명목으로 추상화 계층을 덧씌우지 않았는가?
* **모듈형 잎새 확장**: 향후 예정된 변형(예: 새 무기, 새 몬스터 카드) 추가 시 엔진 코어를 건드리지 않고 잎새 서브클래스나 데이터 추가만으로 작동할 수 있는 클린 인터페이스를 제공하는가?

### ④ 네트워크 및 복제 최적화 (Network & Replication)
* **권한 분리(Authority)**: 모든 전투 연산, 대미지 처리, 인벤토리/체력 변조가 `HasAuthority()` 또는 `ROLE_Authority`로 엄격히 보호되는가?
* **RPC 소유권(Ownership)**: `Server_*` RPC가 클라이언트의 커넥션 소유 액터(`APlayerController`, `APlayerState`, 빙의된 `APawn`)에만 선언되었는가? (월드 내 배치된 미소유 상호작용 액터의 Server RPC 원천 차단)
* **복제 대역폭 최적화**: 모든 복제 프로퍼티에 `DOREPLIFETIME_CONDITION` (`COND_OwnerOnly`, `COND_SimulatedOnly`)이 적용되었는가?
* **RepNotify 우선**: 틱 단위 고빈도 RPC를 지양하고 `ReplicatedUsing = OnRep_*` 상태 동기화 패턴을 채택했는가? 동적 배열에 `FFastArraySerializer`를 적용했는가?

### ⑤ 일반 성능 및 메모리 최적화 (General Optimization)
* **Tick 위생**: 모든 액터 및 컴포넌트에서 `PrimaryActorTick.bCanEverTick = false`를 기본값으로 유지하고, 프레임 연속 시뮬레이션이 필수적인 경우에만 제한적으로 활성화했는가?
* **핫패스 힙 할당 차단**: `Tick()`, 애니메이션 노티파이, 충돌 루프 내에서 동적 메모리 할당(`TArray::Add` без `Reserve`, 문자열 포맷팅, 람다 복사 캡처)이 발생하는가?
* **메모리 및 GC 안전성**: 모든 UObject 멤버 변수에 `TObjectPtr<T>`를 사용하고, 순환 참조를 방지하기 위해 비소유 캐시 참조에는 `TWeakObjectPtr<T>`를 적용했는가?
* **IWYU 준수**: 헤더 파일의 include를 최소화하고 전방 선언(Forward Declaration)을 올바르게 사용했는가?

---

## 4. 구현 파일 및 디렉토리 구조

프로젝트 루트를 기준으로 다음과 같이 설정 및 규칙 파일이 배치되었습니다:

```text
FC/
├── .agents/
│   └── skills/
│       └── interrogate/
│           └── SKILL.md            <-- /interrogate 스킬 정의 및 자동 실행 워크플로우
├── .antigravity/
│   └── rules/
│       └── 07-adversarial-audit-standards.md  <-- 5대 심사 기준 공식 도메인 룰
├── AGENTS.md                       <-- 파이프라인 모드 및 표준 개발 지침 연계
└── Report/
    └── Architecture/
        ├── FC_Adversarial_Subagent_Code_Audit_Architecture_Report.md   <-- 본 보고서 (Markdown)
        └── FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf  <-- 발행된 정식 PDF 보고서
```

---

## 5. 실무 운영 프로토콜 (Operational Protocol)

### 5.1 호출 방법
작업 완료 후 언제든지 대화창에 다음 명령 중 하나를 입력합니다:
* `/interrogate`
* `"코드 검사해줘"`
* `"서브에이전트 리뷰 돌려줘"`
* `"최적화 및 네트워크 검사해줘"`

### 5.2 출력 리포트 표준 규격
서브에이전트는 다음과 같이 구조화된 판정 리포트를 즉시 반환합니다:

```markdown
# 🛡️ Adversarial Audit Report

**Verdict**: [APPROVED (LGTM)] 또는 [CHANGES REQUESTED (BLOCKING)]

### 1. Executive Summary
- 2~3문장으로 전반적인 설계 건전성 및 핵심 피드백 요약.

### 2. Findings Matrix
| Severity | Dimension | Location | Issue & Root Cause | Actionable Fix |
| :--- | :--- | :--- | :--- | :--- |
| **BLOCKING** | Network | `Source/FC/...:42` | Server RPC on unowned Actor | APlayerController로 이전 |
| **WARNING** | Reuse | `Source/FC/...:88` | Re-implemented math helper | 기존 `FCCombatStatics` 사용 |
| **SUGGESTION**| Conciseness | `Source/FC/...:120`| Redundant temporary wrapper | 인라인화 처리 |

### 3. Dimension Scoring
- Conciseness: [Score /10]
- Architecture Reuse: [Score /10]
- Scoped Extensibility: [Score /10]
- Network Optimization: [Score /10]
- General Optimization: [Score /10]

### 4. Required Adjustments before Approval
- 승인을 위해 반드시 수정되어야 할 항목 목록.
```

---

## 6. 결론 및 기대 효과

1. **무결점 멀티플레이어 코드 보장**:
   - 컴파일 에러는 잡히지 않지만 런타임에서 패킷 드롭이나 Desync를 유발하는 RPC 소유권 위반, Authority 체크 누락을 사전에 차단합니다.
2. **코드베이스 부채 최소화**:
   - 필요 이상의 과도한 추상화를 억제하고 기존 구조를 최대한 재사용하도록 강제하여, 코드 비대화(Bloat)를 방지합니다.
3. **토큰 및 개발 속도 최적화**:
   - 서브에이전트가 엄격한 루브릭에 맞춰 단 한 번의 실행으로 정밀한 리포트를 제공하므로, 반복적인 수정 루프를 최소화합니다.
