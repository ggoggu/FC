# FC (Project FC) - Unreal Engine AI Utilization & Autonomous Workflow Testbed

> **프로젝트 시작 목적**  
> 본 프로젝트는 **언리얼 엔진(Unreal Engine 5.8)** 환경에서 **AI 코딩 에이전트 및 자율 워크플로우의 실전 개발 역량을 검증하고 테스트하기 위해 시작**되었습니다.

---

## 📌 1. 개요 (Overview)

단순한 코드 생성 테스트를 넘어, 상용 수준의 언리얼 엔진 멀티플레이어 아키텍처(GAS, MVVM, 서버 권위형 복제) 하에서 AI 에이전트가:
1. 복잡한 엔진 아키텍처 규칙과 네트워크 불변성을 준수할 수 있는가?
2. Live Editor Reflection(MCP) 및 에셋 분석 툴을 활용해 바이너리 에셋의 진실을 정확히 파악하고 코드를 작성하는가?
3. 엄격한 정적 린터, 빌드 파이프라인, 자동화 단위 테스트, 적대적 보안 감사를 스스로 통과하며 점진적 발전을 이뤄낼 수 있는가?

를 실증하고 테스트하기 위한 테스트베드 프로젝트입니다.

---

## 🎯 2. 핵심 테스트 및 검증 영역

### 1) AI 에이전트 자율 루프 & 가드레일 (Anti-Slop Harness)
- **Live MCP 에디터 연동**: `ModelContextProtocol`, `AllToolsets`, `MVVMToolset` 등을 통해 실행 중인 에디터 내부 에셋 및 그래프를 AI가 직접 검사.
- **결정론적 검증 파이프라인**: C++ 코드 수정 시 즉각적인 AST/정적 분석(`Scripts/pipeline.py static`), UBT 컴파일, 헤드리스 자동화 테스트를 기계적으로 거치도록 강제.
- **적대적 코드 감사 (Interrogate)**: 리플리케이션 누수, 대역폭 과부하, GAS 스레드 안전성, 보안 취약점을 에이전트가 교차 검증.

### 2) 상용 멀티플레이어 아키텍처 실증
- **Single-Player is Local Multiplayer**: 싱글 플레이어 지름길을 배제하고, 모든 로직을 로컬 리슨 서버 및 데디케이트 서버 표준으로 구축.
- **엄격한 계층 분리**:
  - **Server Authority**: 게임 모드, AI 컨트롤러, 상태 변이, 인벤토리/스폰 통제
  - **Replicated State**: `DOREPLIFETIME_CONDITION`, `FFastArraySerializer`, `OnRep_*`
  - **Autonomous Client**: Enhanced Input 기반 Server RPC 의도 전달
  - **Presentation & UI**: Slate/UMG 종속성이 분리된 순수 MVVM(`UMVVMViewModelBase`, `FieldNotify`)

### 3) 핵심 게임플레이 시스템
- **Gameplay Ability System (GAS)**: AttributeSet, GameplayEffect, GameplayAbility, GameplayCue 체계.
- **Card & Deck System**: 런타임 덱 빌딩 및 카드 사용 네트워크 동기화.
- **Combat & Element Mechanics**: 원소 스택 컨테이너, 투사체 및 환경 상호작용.
- **Dungeon & Mob Spawner**: 절차적 방 생성 및 몬스터 AI 스폰/전투 루프.

---

## 🛠️ 3. 기술 스택 (Tech Stack)

| 구분 | 기술 / 도구 |
| :--- | :--- |
| **Engine** | Unreal Engine 5.8 |
| **Language** | C++20, Python (파이프라인 및 린터) |
| **Gameplay** | Gameplay Ability System (GAS), Enhanced Input |
| **UI** | UMG, Model-View-ViewModel (MVVM) |
| **AI Integration** | Model Context Protocol (MCP), Antigravity Agent Harness |
| **Verification** | Automation Spec (23개 자동화 테스트), 커스텀 AST 정적 분석기 |

---

## 📁 4. 프로젝트 구조 (Repository Structure)

```text
FC/
├── Source/FC/               # 언리얼 C++ 메인 게임플레이 & 시스템 모듈
│   ├── Public/
│   │   ├── AbilitySystem/   # GAS Attributes, Abilities, Effects
│   │   ├── Card/            # 카드 덱 컴포넌트 및 데이터에셋
│   │   ├── Character/       # 플레이어 및 몬스터 베이스 클래스
│   │   ├── Combat/          # 원소 상호작용, 투사체, 전투 유틸
│   │   ├── Controller/      # 플레이어 및 AI 컨트롤러
│   │   ├── Map/             # 던전 매니저, 룸, 도어 시스템
│   │   └── UI/              # MVVM 뷰모델 및 프레젠터
│   └── Private/Tests/       # 23개 엔진 자동화 사양 (FC.* Automation Specs)
├── Scripts/                 # AI 검증 및 CI/CD 파이프라인 스크립트
│   ├── pipeline.py          # 빌드, 테스트, 정적 분석 통합 실행기
│   ├── verify_replication.py# 네트워크 복제 규칙 검증
│   ├── verify_gas.py        # GAS 속성 및 메타어트리뷰트 검증
│   └── audit_bandwidth.py   # 네트워크 페이로드/대역폭 감사
├── .agents/                 # AI 코딩 에이전트 룰셋 및 스킬
│   ├── rules/               # UE5.8 코딩 표준, 네트워크, GAS 컨벤션
│   └── skills/              # 에셋 인스펙터, 적대적 코드 리뷰 스킬
└── Content/                 # 언리얼 엔진 에셋 (Map, Blueprints, UI, Data)
```

---

## 🚀 5. 파이프라인 명령어 (Verification Commands)

프로젝트에 탑재된 검증 도구는 터미널을 통해 간편하게 실행할 수 있습니다:

```powershell
# 1. 증분/전체 C++ 정적 린터 검증
python Scripts/pipeline.py static

# 2. UnrealBuildTool (UBT) C++ 컴파일
python Scripts/pipeline.py build

# 3. 언리얼 헤드리스 자동화 테스트 실행 (23개 스펙)
python Scripts/pipeline.py test

# 4. 네트워크 대역폭 및 가상 레이턴시 성능 검증
python Scripts/pipeline.py net

# 5. 독립 에이전트 적대적 코드/보안 감사
python Scripts/pipeline.py audit
```
