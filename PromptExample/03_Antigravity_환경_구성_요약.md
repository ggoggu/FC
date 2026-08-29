# Antigravity 환경 구성 및 개발 시스템 종합 요약

본 문서는 **FC 프로젝트(Unreal Engine 5.8)**에 구축된 **Antigravity AI 페어 프로그래밍 및 자율 개발 환경**의 전체 구조와 작동 방식을 한눈에 파악할 수 있도록 정리한 시스템 요약서입니다.

---

## 1. 프로젝트 및 환경 개요

* **프로젝트명**: FC (`FC.uproject`)
* **엔진 버전**: Unreal Engine 5.8 (Target: `FCEditor`, Platform: `Win64`, C++20)
* **핵심 아키텍처 원칙**: 
  - **"Single-Player is Local Multiplayer"**: 단독 플레이도 Listen Server 세션으로 동작하여 멀티플레이와 100% 기능 동일성 보장.
  - **모듈형 도메인 설계**:
    * **일반 기능**: 경량 C++ 액터 컴포넌트(`UActorComponent`) 및 4계층 권한 모델 준수.
    * **GAS(Gameplay Ability System)**: 복잡한 상태이상, 버프/디버프, 스킬 시스템 필요 시 선택적/온디맨드 적용.
    * **Fast Array Serialization**: 동적 아이템 및 무기 목록 동기화 시 대역폭 최적화를 위해 필수 적용.

---

## 2. 전체 시스템 구조도

```
┌──────────────────────────────────────────────────────────────────────────────────────────┐
│                                ANTIGRAVITY AGENT ENGINE                                  │
│  - AI Pair Programmer (DeepMind Antigravity)                                             │
│  - Subagents: General (`self`), Research (`research`), Custom (`define_subagent`)        │
│  - MCP Client & Background Process Manager                                               │
└────────────────────────────────────────────┬─────────────────────────────────────────────┘
                                             │
                       ┌─────────────────────┴─────────────────────┐
                       ▼                                           ▼
┌──────────────────────────────────────────────┐ ┌─────────────────────────────────────────┐
│        AI Directives & Rules (.md)           │ │           MCP Server & Tools            │
├──────────────────────────────────────────────┤ ├─────────────────────────────────────────┤
│ • AGENTS.md                                  │ │ • .antigravity/mcp.json                 │
│   (최상위 권한 분리 및 파이프라인 지침)      │ │   (ue5-harness MCP 서버)                │
│ • .antigravity/rules/ (온디맨드 도메인 룰)   │ │ • Scripts/                              │
│   01-ue58-coding-standards.md                │ │   pipeline.py (통합 마스터 러너)        │
│   02-multiplayer-conventions.md              │ │   pipeline.bat / pipeline.ps1           │
│   03-loop-engineering.md                     │ │   build_harness.py (UBT 컴파일)         │
│   04-gameplay-ability-system.md              │ │   verify_gas.py (GAS 검증)              │
│   05-combat-and-inventory-networking.md      │ │   audit_bandwidth.py (대역폭 감사)      │
│ • .antigravity/skills/                       │ │   verify_replication.py (리플리케이션)  │
│   ue5-harness (자동화 스킬 팩)               │ │   simulate_ttk_balance.py (TTK 시뮬)    │
└──────────────────────────────────────────────┘ └─────────────────────────────────────────┘
```

---

## 3. 규칙 및 시스템 지침 체계

Antigravity는 프로젝트 로드 시 아래 규칙들을 참조하여 코드를 작성합니다:

| 문서 위치 | 역할 및 통제 영역 | 적용 시점 |
| :--- | :--- | :--- |
| [**`AGENTS.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/AGENTS.md) | 프로젝트 전체 최상위 지침: 4계층 권한 분리 모델 및 모듈형 파이프라인 프로토콜 | **모든 작업 기본 적용** |
| [**`01-ue58-coding-standards.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.antigravity/rules/01-ue58-coding-standards.md) | UE5.8 최신 C++ 코딩 표준 (`TObjectPtr`, IWYU, PascalCase, 스마트 포인터) | 모든 C++ 코드 작성 시 |
| [**`02-multiplayer-conventions.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.antigravity/rules/02-multiplayer-conventions.md) | 서버 권한(`HasAuthority`), Client-Prediction, UI 완벽 분리(UI는 Slate/UMG 클라이언트 전용) | 네트워크/멀티플레이 작성 시 |
| [**`03-loop-engineering.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.antigravity/rules/03-loop-engineering.md) | 자율 자가 교정(Self-Healing) 루프, 에러 파싱 트리아지 가이드 (최대 5회 자가 수정) | 빌드/검증 실행 시 |
| [**`04-gameplay-ability-system.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.antigravity/rules/04-gameplay-ability-system.md) | GAS 어트리뷰트 선언 매크로(`ATTRIBUTE_ACCESSORS`), RepNotify, Gameplay Cue 사용 규칙 | **GAS 도입 기능 작성 시** |
| [**`05-combat-and-inventory-networking.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.antigravity/rules/05-combat-and-inventory-networking.md) | `FFastArraySerializer` 구현 패턴, `COND_OwnerOnly` / `COND_SkipOwner` 필터링 지침 | **동적 인벤토리/슬롯 작성 시** |

---

## 4. 통합 파이프라인 및 도구 체계 (`Scripts/`)

검증 시 다중 툴 호출로 인한 토큰 낭비를 없애기 위해 **단일 진입점 러너([`pipeline.py`](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/pipeline.py))**를 구축하였습니다.

```
                                  [ python Scripts/pipeline.py <mode> ]
                                                    │
         ┌───────────────┬──────────────────────────┼──────────────────────────┬───────────────┐
         ▼               ▼                          ▼                          ▼               ▼
      [static]        [build]                   [balance]                    [test]          [full]
   • verify_gas    • build_harness           • simulate_ttk             • run_tests     • static
   • audit_bw        (UBT Compile)             (TTK & Effective DPS)      (Headless)    • build
   • verify_repl   • JSON Self-Healing                                  • sim_net_pie   • test
   (~0.4s 초고속)    (최대 5회 루프)             (~0.2s 초고속)                           • [Auto-Commit]
```

### 각 도구별 상세 스펙

1. **[`pipeline.py`](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Scripts/pipeline.py)**:
   - `python Scripts/pipeline.py static` : 정적 룰 검증 (약 0.4초 소요, 초저비용)
   - `python Scripts/pipeline.py build` : UBT 컴파일 및 JSON 진단 에러 출력
   - `python Scripts/pipeline.py balance` : 무기 아키타입별 TTK/DPS 시뮬레이션
   - `python Scripts/pipeline.py full --auto-commit -m "..."` : 정적 $\rightarrow$ 빌드 $\rightarrow$ 테스트 $\rightarrow$ Git 자동 커밋
2. **[`pipeline.bat`](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/pipeline.bat) / [`pipeline.ps1`](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/pipeline.ps1)**:
   - 윈도우/PowerShell 터미널에서 즉시 실행 가능한 단축 래퍼.
3. **[`.antigravity/mcp.json`](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/.antigravity/mcp.json)**:
   - Antigravity의 MCP(Model Context Protocol) 툴로 `ue5_build`, `ue5_verify_gas`, `ue5_audit_bandwidth` 등을 직접 노출.

---

## 5. 가이드라인 및 프롬프트 예시 문서 맵

| 문서 파일 | 주요 내용 |
| :--- | :--- |
| [**`01_파이프라인_사용_가이드라인.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/PromptExample/01_%ED%8C%8C%EC%9D%B4%ED%94%84%EB%9D%BC%EC%9D%B8_%EC%82%AC%EC%9A%A9_%EA%B0%80%EC%9D%B4%EB%93%9C%EB%9D%BC%EC%9D%B8.md) | Fast Track(스크립트 미실행) vs On-Demand 검증 모드 활용법 및 토큰 절약 팁 |
| [**`02_기능구현_프롬프트_예시.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/PromptExample/02_%EA%B8%B0%EB%8A%A5%EA%B5%AC%ED%98%84_%ED%94%84%EB%A1%AC%ED%94%84%ED%8A%B8_%EC%98%88%EC%8B%9C.md) | Non-GAS 일반 C++ 컴포넌트, GAS 어빌리티, FastArray 인벤토리, 상호작용 등 실전 프롬프트 템플릿 |
| [**`03_Antigravity_환경_구성_요약.md`**](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/PromptExample/03_Antigravity_%ED%99%98%EA%B2%BD_%EA%B5%AC%EC%84%B1_%EC%9A%94%EC%95%BD.md) | *(본 문서)* 전체 환경 및 아키텍처/규칙/도구 구조 종합 요약 |
