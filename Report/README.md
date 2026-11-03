# FC Project Technical Reports & Architecture Documentation

본 디렉터리(`Report/`)는 **FC (Fantasy Card)** 프로젝트(Unreal Engine 5.8 / Listen & Dedicated Server Architecture)의 전체 시스템 아키텍처, 게임플레이 도메인 심층 분석, 성능 실측 벤치마크, 그리고 Antigravity AI 엔지니어링 생태계에 대한 기술 문서 및 다이어그램을 체계적으로 분류·보관하는 중앙 아카이브입니다.

---

## 📁 디렉터리 분류 체계 (Directory Taxonomy)

```
Report/
├── 01_Project_Architecture/    # FC 프로젝트 종합 기술 분석 백서 시리즈 (Vol 1 ~ 3)
├── 02_Gameplay_Systems/        # 도메인별 심층 분석 보고서 및 소스 코드 정밀 해설서
│   ├── Dungeon_and_Rooms/      # 절차적 던전 생성, 위상수학 알고리즘, 룸 시스템 C++ 분석
│   ├── Cards_and_Deck/         # 카드 데이터 하이브리드 아키텍처 및 로딩 성능 벤치마크
│   ├── Combat_and_Networking/  # 오브젝트 풀링, NetMulticast 네트워크 최적화, DB 연동
│   └── UI_and_MVVM/            # UMG MVVM 프레젠테이션 계층 및 위젯 부하 테스트
├── 03_AI_and_Antigravity/      # Antigravity 백서 3부작, 서브에이전트 심사 체계, 툴링 벤치마크
├── 04_Diagrams/                # 핵심 아키텍처 모델 및 시퀀스 고화질 다이어그램
└── README.md                   # 기술 보고서 종합 색인 및 구조 안내서 (본 문서)
```

---

## 📑 상세 문서 목록 및 내용 요약

### 01. Project Architecture (종합 기술 분석 시리즈)
프로젝트 전반의 멀티플레이어 권한 분리 모델부터 핵심 메커니즘을 총망라한 3대 기술 백서입니다.

| 파일명 | 분량 | 핵심 내용 및 아키텍처 범위 |
| :--- | :---: | :--- |
| [FC_Project_Analysis_Vol1_System_Architecture_and_Dungeon.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/01_Project_Architecture/FC_Project_Analysis_Vol1_System_Architecture_and_Dungeon.pdf) | 8p | **제1권**: 단일/로컬 멀티플레이어 아키텍처, 4-계층 권한 분리 모델, 리플리케이션 채널 및 스켈레톤 성장 기반 절차적 던전 생성 엔진 분석 |
| [FC_Project_Analysis_Vol2_Card_Deck_and_GAS_Combat.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/01_Project_Architecture/FC_Project_Analysis_Vol2_Card_Deck_and_GAS_Combat.pdf) | 8p | **제2권**: 카드 덱 네트워크 동기화, `FFastArraySerializer` 기반 대역폭 최적화, GAS 원소 속성 연계 및 투사체 물리 시뮬레이션 분석 |
| [FC_Project_Analysis_Vol3_AI_MVVM_and_Pipeline.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/01_Project_Architecture/FC_Project_Analysis_Vol3_AI_MVVM_and_Pipeline.pdf) | 8p | **제3권**: AI 몹 비헤이비어 트리 & EQS, 오브젝트 풀링 기반 스포너, UMG MVVM 프레젠테이션 분리 및 자동화 파이프라인 검증 체계 |

---

### 02. Gameplay Systems (핵심 게임플레이 시스템 심층 분석)

#### 02-1. Dungeon & Rooms (절차적 던전 및 방 시스템)
절차적 던전 맵 생성 알고리즘, 위상수학적 그래프 이론 및 C++ 핵심 클래스 정밀 해설 문서입니다.

| 파일명 | 분량 | 핵심 내용 |
| :--- | :---: | :--- |
| [Dungeon_Map_Generation_Architecture_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Dungeon_and_Rooms/Dungeon_Map_Generation_Architecture_Analysis_Report.pdf) | 10p | 스켈레톤 성장 알고리즘, 이산 그래프 위상수학(Discrete Graph Topology), BFS 측지 거리 및 아이작(The Binding of Isaac) 공식 분석 |
| [Dungeon_Map_Generation_Code_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Dungeon_and_Rooms/Dungeon_Map_Generation_Code_Analysis_Report.pdf) | 12p | 던전 맵 생성 C++ 핵심 소스 코드(`AFCDungeonGenerator`, `UFCDungeonLayoutAlgorithm`), 단계별 알고리즘 흐름 및 네트워크 시드 동기화 분석 |
| [FC_Room_Code_Explanation.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Dungeon_and_Rooms/FC_Room_Code_Explanation.pdf) | 12p | `FCRoomBase`, `FCDungeonTypes` 및 연계 모듈러 맵/도어/스포너 시스템의 소스 레벨 정밀 해설 (Python IDLE 뷰어 포맷) |

#### 02-2. Cards & Deck (카드 및 덱 데이터 시스템)
대규모 카드 데이터 확장성과 메모리 스트리밍을 위한 아키텍처 의사결정 및 실측 벤치마크입니다.

| 파일명 | 분량 | 핵심 내용 |
| :--- | :---: | :--- |
| [Card_Data_Architecture_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Cards_and_Deck/Card_Data_Architecture_Analysis_Report.pdf) | 4p | 카드 데이터 시스템 아키텍처 분석 및 방안 B(DataTable + Soft Reference) 선정 사유, 확장성 및 하이브리드 스트리밍 설계 검증 |
| [Card_Data_Architecture_Benchmark_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Cards_and_Deck/Card_Data_Architecture_Benchmark_Report.pdf) | 4p | 레거시 Direct DataAsset vs 방안 B(DataTable + TSoftObjectPtr)의 메모리 점유, 로딩 속도, 캐시 효율 실측 벤치마크 평가 |

#### 02-3. Combat & Networking (전투, 네트워크 복제 및 풀링)
서버 권한 기반 네트워크 동기화 최적화 및 액터 생명주기 관리 기술 문서입니다.

| 파일명 | 분량 | 핵심 내용 |
| :--- | :---: | :--- |
| [FC_NetMulticast_Optimization_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Combat_and_Networking/FC_NetMulticast_Optimization_Report.pdf) | 7p | 무분별한 `NetMulticast`를 지양하고 Server Authority + Replicated State + OnRep Presentation 패턴으로 전면 개편하여 대역폭 절감 및 신뢰성 확보 |
| [Chest_Object_Pooling_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Combat_and_Networking/Chest_Object_Pooling_Analysis_Report.pdf) | 7p | 상자(`AFCChestActor`) 오브젝트 풀링 타당성, 액터 수명 주기, GAS 어빌리티 상태 무결성 및 복제 채널 영향 분석 |
| [Mob_Object_Pooling_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Combat_and_Networking/Mob_Object_Pooling_Analysis_Report.pdf) | 4p | 몬스터(Mob) 액터의 오브젝트 풀링 아키텍처 타당성 검토 및 AI Controller 분리/재연결 시 생명주기 안전성 평가 |
| [Database_Integration_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/Combat_and_Networking/Database_Integration_Analysis_Report.pdf) | 7p | UE 5.8 기반 데이터 영속화 체계, 로컬 멀티플레이어 환경에서의 DB 연동 타당성 및 UObject 에셋 무결성 분석 |

#### 02-4. UI & MVVM (프레젠테이션 계층)
UMG Model-View-ViewModel 패턴 적용 및 UI 렌더링 부하 테스트 결과입니다.

| 파일명 | 분량 | 핵심 내용 |
| :--- | :---: | :--- |
| [Card_Hand_MVVM_Architecture_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/UI_and_MVVM/Card_Hand_MVVM_Architecture_Analysis_Report.pdf) | 3p | Card & Hand UI 아키텍처 분석, MVVM 플러그인 도입 대비 기존 UMG 성능 비교, 위젯 풀링 및 부하 테스트 실측 수치 |
| [ElementStack_Architecture_Analysis_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/02_Gameplay_Systems/UI_and_MVVM/ElementStack_Architecture_Analysis_Report.pdf) | 2p | 원소 중첩 UI(`ElementStackItem`)의 빈번한 갱신 상황에서 슬레이트 레이아웃 리빌드 억제 및 부하 테스트 보고서 |

---

### 03. AI & Antigravity (AI 생태계, 거버넌스 및 자동화 도구)
Google Antigravity 플랫폼 활용, 서브에이전트 거버넌스 및 개발 툴링 관련 문서입니다.

| 파일명 | 형식 | 분량 | 핵심 내용 |
| :--- | :---: | :---: | :--- |
| [FC_Antigravity_Vol1_Platform_and_Core_Runtime.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_Antigravity_Vol1_Platform_and_Core_Runtime.pdf) | PDF | 7p | **안티그래비티 종합 백서 제1권**: 플랫폼 생태계, 4대 서피스(CLI/IDE/SDK/Extension), 비동기 코어 런타임 및 보안 거버넌스 |
| [FC_Antigravity_Vol2_Customization_and_Extensibility.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_Antigravity_Vol2_Customization_and_Extensibility.pdf) | PDF | 7p | **안티그래비티 종합 백서 제2권**: 커스터마이징 5대 기둥(Rules, Skills, MCP, Prompts, Agents), 점진적 공개 원칙 및 MCP 표준 |
| [FC_Antigravity_Vol3_FC_Project_Engineering_and_Loop.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_Antigravity_Vol3_FC_Project_Engineering_and_Loop.pdf) | PDF | 7p | **안티그래비티 종합 백서 제3권**: FC 프로젝트 UE 5.8 실전 엔지니어링 룰셋, 모듈러 아키텍처 및 자율 셀프 힐링 폐루프 |
| [FC_Agent_Rules_Skills_MCP_Architecture_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_Agent_Rules_Skills_MCP_Architecture_Report.pdf) | PDF | 8p | UE 5.8 복제 아키텍처 지침, 슬림 러너(`Scripts/pipeline.py`) 및 Live MCP 에디터 소켓 연동 아키텍처 보고서 |
| [FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf) | PDF | 2p | 독립 서브에이전트(`/interrogate`) 기반 적대적 코드 심사 및 5대 검증 차원(간결성/권한/복제/IWYU/GAS) 체계 구현 보고서 |
| [FC_Adversarial_Subagent_Code_Audit_Architecture_Report.md](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_Adversarial_Subagent_Code_Audit_Architecture_Report.md) | MD | - | 적대적 코드 심사 아키텍처 마크다운 원본 문서 |
| [FC_SubAgent_Addition_Rules_Skills_Impact_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_SubAgent_Addition_Rules_Skills_Impact_Report.pdf) | PDF | 3p | 서브에이전트 추가에 따른 프로젝트 5대 계층(Rule, AGENTS.md, Skill, Runner, Pipeline) 연쇄 영향도 분석 |
| [FC_Python_Remote_Execution_Speed_Benchmark_Report.pdf](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/03_AI_and_Antigravity/FC_Python_Remote_Execution_Speed_Benchmark_Report.pdf) | PDF | 3p | Python Remote Execution(라이브 에디터 소켓) vs UnrealEditor-Cmdlet 실측 속도(0.04s vs 8.5s) 및 생산성 벤치마크 |

---

### 04. Diagrams (아키텍처 및 시퀀스 다이어그램)
각종 보고서 및 발표에 활용되는 고해상도 아키텍처 다이어그램 렌더링 파일입니다.

| 파일명 | 형식 | 설명 |
| :--- | :---: | :--- |
| [FC_Overall_Architecture.png](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/04_Diagrams/FC_Overall_Architecture.png) | PNG | FC 프로젝트 전체 시스템 레이어 및 모듈 간 상호작용 전체 조감도 |
| [FC_4Tier_Authority_Model.png](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/04_Diagrams/FC_4Tier_Authority_Model.png) | PNG | Server Authority / Replicated State / Autonomous Client / Presentation 4-계층 권한 분리 모델 |
| [FC_Card_Combat_Sequence.png](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/04_Diagrams/FC_Card_Combat_Sequence.png) | PNG | 플레이어 카드 시전 요청부터 서버 권한 검증, GAS 타깃팅, 투사체 및 대미지 적용 시퀀스 다이어그램 |
| [FC_Mob_Spawn_Combat_Sequence.png](file:///C:/Users/oogg/Documents/Unreal%20Projects/FC/Report/04_Diagrams/FC_Mob_Spawn_Combat_Sequence.png) | PNG | 룸 진입 시 몹 풀링 활성화, AIController 빙의, 비헤이비어 트리 구동 및 전투 상호작용 시퀀스 |
