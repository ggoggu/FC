#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates an executive PDF report:
"FC 프로젝트 AI 개발 아키텍처 종합 분석 보고서 (Version 1.3)"
Covers AGENTS.md, Rules, Skills, MCP, Asset Inspection, Python Remote Execution, and Self-Healing Pipeline.
"""

import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
OUTPUT_PDF = PROJECT_ROOT / "Report" / "FC_Agent_Rules_Skills_MCP_Architecture_Report.pdf"
TEMP_HTML = PROJECT_ROOT / "Report" / "FC_Agent_Rules_Skills_MCP_Architecture_Report.html"

HTML_CONTENT = """<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<title>FC 프로젝트 AI 개발 아키텍처 종합 분석 보고서</title>
<style>
    @page {
        size: A4;
        margin: 12mm 14mm 12mm 14mm;
    }
    *, *:before, *:after {
        box-sizing: border-box;
    }
    body {
        font-family: 'Pretendard', -apple-system, BlinkMacSystemFont, 'Malgun Gothic', '맑은 고딕', 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        color: #1e293b;
        background: #ffffff;
        line-height: 1.45;
        font-size: 8.8pt;
        margin: 0;
        padding: 0;
    }
    .page-container {
        page-break-after: always;
        height: 100%;
        display: flex;
        flex-direction: column;
        justify-content: space-between;
        min-height: 268mm;
    }
    .page-container:last-child {
        page-break-after: avoid;
    }
    .page-content {
        flex: 1;
    }
    .header-bar {
        display: flex;
        justify-content: space-between;
        align-items: center;
        border-bottom: 1.5px solid #0284c7;
        padding-bottom: 4px;
        margin-bottom: 12px;
        font-size: 7.2pt;
        font-weight: 700;
        color: #64748b;
        letter-spacing: 0.5px;
    }
    .header-bar .tag {
        color: #0284c7;
    }
    .footer-bar {
        border-top: 1px solid #e2e8f0;
        padding-top: 5px;
        margin-top: 10px;
        display: flex;
        justify-content: space-between;
        font-size: 7.2pt;
        color: #94a3b8;
    }
    .cover-header {
        margin-bottom: 12px;
    }
    .badge-top {
        display: inline-block;
        background: #0284c7;
        color: #ffffff;
        font-size: 7.2pt;
        font-weight: 700;
        letter-spacing: 0.8px;
        padding: 2.5px 7px;
        border-radius: 3px;
        text-transform: uppercase;
        margin-bottom: 6px;
    }
    h1 {
        font-size: 19pt;
        font-weight: 800;
        color: #0f172a;
        margin: 0 0 4px 0;
        letter-spacing: -0.6px;
        line-height: 1.2;
    }
    .subtitle {
        font-size: 9.5pt;
        color: #64748b;
        margin: 0 0 10px 0;
    }
    .meta-box {
        display: flex;
        background: #f8fafc;
        border: 1px solid #e2e8f0;
        border-radius: 5px;
        padding: 6px 12px;
        font-size: 7.8pt;
        color: #475569;
        justify-content: space-between;
        margin-bottom: 12px;
    }
    .meta-item strong {
        color: #0f172a;
        display: block;
        font-size: 7pt;
        text-transform: uppercase;
        margin-bottom: 1px;
    }
    h2 {
        font-size: 11.5pt;
        font-weight: 800;
        color: #0f172a;
        border-left: 4px solid #0284c7;
        padding-left: 8px;
        margin: 12px 0 8px 0;
        page-break-after: avoid;
    }
    h3 {
        font-size: 9.2pt;
        font-weight: 700;
        color: #1e293b;
        margin: 8px 0 4px 0;
        page-break-after: avoid;
    }
    p {
        margin: 0 0 6px 0;
    }
    ul, ol {
        margin: 0 0 6px 0;
        padding-left: 15px;
    }
    li {
        margin-bottom: 2.5px;
    }
    table {
        width: 100%;
        border-collapse: collapse;
        margin: 6px 0 10px 0;
        font-size: 7.8pt;
    }
    th, td {
        border: 1px solid #cbd5e1;
        padding: 4.5px 6.5px;
        text-align: left;
        vertical-align: middle;
    }
    th {
        background: #f1f5f9;
        color: #0f172a;
        font-weight: 700;
    }
    tr:nth-child(even) td {
        background: #f8fafc;
    }
    .badge {
        display: inline-block;
        padding: 1px 5px;
        border-radius: 3px;
        font-size: 6.8pt;
        font-weight: 700;
        white-space: nowrap;
    }
    .badge-primary { background: #e0f2fe; color: #0284c7; }
    .badge-danger { background: #fee2e2; color: #991b1b; }
    .badge-warn { background: #fef3c7; color: #92400e; }
    .badge-success { background: #dcfce7; color: #166534; }
    .badge-purple { background: #f3e8ff; color: #7e22ce; }
    .badge-slate { background: #f1f5f9; color: #475569; }
    .code-inline {
        font-family: 'Consolas', 'Courier New', monospace;
        background: #f1f5f9;
        color: #0369a1;
        padding: 1px 4px;
        border-radius: 3px;
        font-size: 7.5pt;
    }
    .callout {
        background: #f8fafc;
        border-left: 3.5px solid #0284c7;
        padding: 8px 11px;
        margin: 7px 0 10px 0;
        border-radius: 0 4px 4px 0;
        font-size: 8.2pt;
    }
    .callout-title {
        font-weight: 700;
        color: #0284c7;
        margin-bottom: 3px;
        display: flex;
        align-items: center;
        justify-content: space-between;
    }
    .diagram-box {
        background: #0f172a;
        color: #e2e8f0;
        border-radius: 6px;
        padding: 10px;
        margin: 8px 0 10px 0;
        font-family: 'Consolas', 'Courier New', monospace;
        font-size: 7.2pt;
        line-height: 1.35;
    }
    .grid-2 {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 8px;
        margin: 6px 0 10px 0;
    }
    .grid-3 {
        display: grid;
        grid-template-columns: 1fr 1fr 1fr;
        gap: 7px;
        margin: 6px 0 9px 0;
    }
    .card {
        background: #ffffff;
        border: 1px solid #e2e8f0;
        border-radius: 5px;
        padding: 7px 9px;
    }
    .card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 4px;
        border-bottom: 1px solid #f1f5f9;
        padding-bottom: 3px;
    }
    .card-title {
        font-weight: 700;
        font-size: 8pt;
        color: #0f172a;
    }
</style>
</head>
<body>

<!-- ================= PAGE 1 ================= -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC ARCHITECTURE DIRECTIVE & AI ECOSYSTEM REPORT</span>
            <span class="tag">UE 5.8 REPLICATION, ASSET REFLECTION & HARNESS</span>
            <span>CONFIDENTIAL & PROPRIETARY</span>
        </div>

        <div class="cover-header">
            <span class="badge-top">FC System Architecture Audit</span>
            <h1>FC 프로젝트 AI 개발 아키텍처 종합 분석 보고서</h1>
            <div class="subtitle">AGENTS.md 최상위 헌장, 도메인 룰(Rules), 자율 스킬(Skills), MCP 도구, 에셋 인스펙션 및 순환 엔지니어링 체계 심층 해설</div>
            <div class="meta-box">
                <div class="meta-item">
                    <strong>Target Engine</strong>
                    Unreal Engine 5.8 (Win64 / C++20)
                </div>
                <div class="meta-item">
                    <strong>Architecture Core</strong>
                    Listen Server (SP = MP Parity)
                </div>
                <div class="meta-item">
                    <strong>AI Agent Environment</strong>
                    Antigravity Autonomous Engine
                </div>
                <div class="meta-item">
                    <strong>Report Date & Version</strong>
                    2026-10-15 • Version 1.3 (Asset Reflection Added)
                </div>
            </div>
        </div>

        <h2>1. 총괄 요약 (Executive Summary)</h2>
        <p>
            본 문서는 FC 프로젝트(Unreal Engine 5.8)에 구축된 <strong>Antigravity AI 페어 프로그래밍 및 자율 에이전트 개발 체계</strong>의 기술적 구성 요소(<span class="code-inline">AGENTS.md</span>, <span class="code-inline">Rules</span>, <span class="code-inline">Skills</span>, <span class="code-inline">MCP</span>, <span class="code-inline">Asset Inspection</span>, <span class="code-inline">Scripts</span>)를 체계적으로 분석하여 정리한 공식 아키텍처 보고서입니다. 본 시스템은 단순한 프롬프트 전달 수준을 넘어, AI 에이전트가 엄격한 멀티플레이어 네트워크 권한을 준수하고, 프로젝트 전용 툴체인(UBT, 정적 분석기, 밸런스 시뮬레이터, 자동화 테스트, 에셋 바이너리/원격 인스펙터)과 직접 연동하여 <strong>최대 5회 자가 치유(Self-Healing)</strong>를 수행할 수 있는 완결된 엔지니어링 루프를 제공합니다.
        </p>

        <div class="callout">
            <div class="callout-title">
                <span>🌟 FC 프로젝트 AI 협업 환경 4대 핵심 설계 철학</span>
                <span class="badge badge-primary">CORE TENETS</span>
            </div>
            <ul>
                <li><strong>Single-Player is Local Multiplayer:</strong> 모든 게임플레이 시스템은 네트워크 경계를 전제로 설계됩니다. 단독 플레이도 엔진 내부 Listen Server 세션으로 동작하여 싱글과 협동/멀티플레이 간 100% 기능 동일성을 보장합니다. 클라이언트 임의 상태 변이 숏컷 작성을 엄격히 금지합니다.</li>
                <li><strong>Minimal Surface Area & Structural Preservation (최소한의 수정 & 기존 코드 활용):</strong> 기존 클래스, 유틸리티, 데이터 테이블을 최대한 재활용하며, 변경 범위를 작업 요청에만 국한하여 불필요한 리팩토링이나 코드 훼손을 방지합니다.</li>
                <li><strong>Ground Truth Over Guessing (에셋 추측 금지 & 실측 메타데이터 확인):</strong> Blueprint(<span class="code-inline">BP_*</span>), Widget Blueprint(<span class="code-inline">WBP_*</span>), Animation Montage/ABP를 수정할 때 내부 슬롯이나 변수명을 임의로 추측하지 않고, 전용 인스펙터(<span class="code-inline">ue5_inspect_asset</span>)를 통해 JSON 메타데이터를 확인한 후 안전하게 코드를 작성합니다.</li>
                <li><strong>Token-Efficient Dual-Track Workflow:</strong> 일상적인 코드 작성 및 Q&A 시에는 스크립트 실행 오버헤드가 없는 Fast Track을 유지하고, 검증 요청 시에만 단일 명령어(<span class="code-inline">pipeline.py</span> / <span class="code-inline">pipeline.ps1</span> / <span class="code-inline">MCP</span>)로 통합 검증을 구동하여 컨텍스트 토큰을 극대화합니다.</li>
            </ul>
        </div>

        <div class="diagram-box">
            <div style="color: #38bdf8; font-weight: 700; margin-bottom: 4px; text-align: center;">ANTIGRAVITY AUTONOMOUS ENGINE (AI PAIR PROGRAMMER & HARNESS ECOSYSTEM)</div>
            <div style="display: flex; justify-content: space-between; border-top: 1px solid #334155; padding-top: 6px;">
                <div style="width: 48%;">
                    <div style="color: #facc15; font-weight: 700; margin-bottom: 3px;">▼ Directives & Domain Rules Injection (Rules Tier)</div>
                    <div>• <strong>AGENTS.md</strong>: 최상위 권한 분리, 파이프라인 헌장 & 에셋 검증 수칙</div>
                    <div>• <strong>01-ue58-coding-standards.md</strong>: C++20, TObjectPtr, IWYU, 코드 재활용 원칙</div>
                    <div>• <strong>02-multiplayer-conventions.md</strong>: 권한 격리, WithValidation, 조건부 복제</div>
                    <div>• <strong>03-loop-engineering.md</strong>: 5회 자가치유 루프 규격 & 에셋 인스펙션 프로토콜</div>
                    <div>• <strong>04-gameplay-ability-system.md</strong>: GAS 어트리뷰트, RepNotify & Cue</div>
                    <div>• <strong>05-combat-and-inventory-networking.md</strong>: FastArray, Sweep 판정</div>
                    <div>• <strong>06-umg-mvvm-standards.md</strong>: MVVM ViewModel, Zero-Tick FieldNotify</div>
                    <div>• <strong>.antigravity/skills/</strong>: ue5-harness 8대 커맨드 스킬 팩</div>
                </div>
                <div style="width: 48%;">
                    <div style="color: #34d399; font-weight: 700; margin-bottom: 3px;">▼ MCP Server & Headless Execution Tools (Execution Tier)</div>
                    <div>• <strong>.antigravity/mcp.json</strong>: ue5-harness 8대 MCP 도구 네이티브 노출</div>
                    <div>• <strong>Scripts/pipeline.py</strong>: 7대 모드 통합 마스터 러너 (단일 호출 진입점)</div>
                    <div>• <strong>pipeline.ps1 / pipeline.bat</strong>: 윈도우 PowerShell 초고속 CLI 래퍼</div>
                    <div>• <strong>Scripts/dump_asset_metadata.py</strong>: BP/WBP/Anim 2-Tier 메타데이터 덤퍼</div>
                    <div>• <strong>Python Remote Execution</strong>: 실행 중 에디터와 소켓 통신 (~0.05s)</div>
                    <div>• <strong>Scripts/build_harness.py</strong>: UBT C++ 컴파일 & JSON 에러 파서</div>
                    <div>• <strong>Scripts/verify_gas.py / audit_bandwidth.py</strong>: GAS 및 대역폭 정적 분석</div>
                    <div>• <strong>Scripts/verify_replication.py</strong>: 멀티플레이 리플리케이션 검증기</div>
                    <div>• <strong>Scripts/simulate_ttk_balance.py</strong>: 전투 TTK/DPS 수학적 시뮬레이터</div>
                </div>
            </div>
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project • Lead Systems & Multiplayer Architect Directives • AI System Architecture Report</span>
        <span>Page 1 of 7 • Executive Summary</span>
    </div>
</div>

<!-- ================= PAGE 2 ================= -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC ARCHITECTURE DIRECTIVE & AI ECOSYSTEM REPORT</span>
            <span class="tag">AGENTS.MD SUPREME DIRECTIVES</span>
            <span>CONFIDENTIAL & PROPRIETARY</span>
        </div>

        <h2>2. 최상위 아키텍처 헌장: AGENTS.md 심층 분석</h2>
        <p>
            <span class="code-inline">AGENTS.md</span>는 FC 프로젝트 리포지토리의 최상위 루트에 위치하며, AI 에이전트 시스템에 최우선 규칙(Supreme Rule)으로 자동 주입되는 핵심 헌장입니다. 모든 AI 코딩 작업은 이 문서에 기술된 권한 분리 원칙과 루프 엔지니어링 수칙을 절대적으로 준수해야 합니다.
        </p>

        <h3>가. 4계층 권한 및 관심사 분리 모델 (4-Tier Authority Model)</h3>
        <p>네트워크 멀티플레이어 환경에서 클라이언트 치팅 방지와 시각적 부드러움을 동시에 달성하기 위해 코드를 엄격히 4계층으로 격리합니다:</p>

        <table>
            <thead>
                <tr>
                    <th style="width: 20%;">계층 (Tier)</th>
                    <th style="width: 23%;">대표 클래스 및 주체</th>
                    <th style="width: 37%;">주요 책임 및 실행 영역</th>
                    <th style="width: 20%;">보장 수칙 및 가드</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>1. Server Authority</strong><br><span class="badge badge-danger">SERVER ONLY</span></td>
                    <td><span class="code-inline">AGameModeBase</span>,<br><span class="code-inline">AAIController</span>,<br>Authoritative Managers</td>
                    <td>전투 판정, 데미지 연산, 사망 처리, 인벤토리/재화/퀘스트 변이, 액터 스폰, Server RPC 인자 검증</td>
                    <td><span class="code-inline">if (HasAuthority())</span> 또는 <span class="code-inline">ROLE_Authority</span> 필수 가드</td>
                </tr>
                <tr>
                    <td><strong>2. Replicated State</strong><br><span class="badge badge-primary">SERVER TO CLIENT</span></td>
                    <td><span class="code-inline">AGameStateBase</span>,<br><span class="code-inline">APlayerState</span>,<br><span class="code-inline">UActorComponent</span></td>
                    <td>게임플레이 상태 동기화(체력, 마나, 버프, 팀 점수), 조건부 복제 필터링, 변경 알림 발송</td>
                    <td><span class="code-inline">DOREPLIFETIME_CONDITION</span>,<br><span class="code-inline">OnRep_*</span> 콜백 핸들러</td>
                </tr>
                <tr>
                    <td><strong>3. Autonomous Client</strong><br><span class="badge badge-warn">CLIENT PREDICTION</span></td>
                    <td><span class="code-inline">APlayerController</span>,<br><span class="code-inline">ACharacter</span> (Autonomous)</td>
                    <td>Enhanced Input 처리, 로컬 캐릭터 이동 예측, 사용자 UI 조작 의도 서버 전송</td>
                    <td><span class="code-inline">Server_*</span> RPC 필수,<br><span class="code-inline">WithValidation</span> 의무화</td>
                </tr>
                <tr>
                    <td><strong>4. Simulated Client & UI</strong><br><span class="badge badge-success">PRESENTATION TIER</span></td>
                    <td><span class="code-inline">AHUD</span>, <span class="code-inline">UUserWidget</span>,<br><span class="code-inline">UNiagaraComponent</span>, Audio</td>
                    <td>UI 렌더링, HUD 갱신, SFX/VFX 재생, 랙 보간 연출 (서버 코드 종속성 0%)</td>
                    <td>서버 코드 내 Slate/UMG 인클루드 완전 금지</td>
                </tr>
            </tbody>
        </table>

        <h3>나. 최소 수정 및 구조 보존 원칙 (Minimal Disruption Directive)</h3>
        <div class="callout">
            <div class="callout-title">
                <span>🛡️ AI 에이전트 코드 수정의 4대 안전 수칙</span>
                <span class="badge badge-warn">SAFETY RULES</span>
            </div>
            <ol>
                <li><strong>최소한의 수정 범위 (Minimal Surface Area Changes):</strong> 요청된 작업과 무관한 파일, 함수, 공백, 서식을 임의로 재작성하거나 리팩토링하지 않으며, 가장 작고 명확한 기능적 Diff를 생성합니다.</li>
                <li><strong>기존 구조 및 코드 재활용 극대화 (Maximize Reuse of Existing Architecture & Code):</strong> 새로운 추상화 계층이나 중복 함수를 임의로 만들기 전에 프로젝트 내 기존 클래스, 유틸리티 함수, 데이터 테이블, 서브시스템을 우선 조사하여 상속 및 확장합니다.</li>
                <li><strong>문서 및 의도 무결성 유지 (Preserve Context & Documentation Integrity):</strong> 기존 주석, TODO 항목, 설계 의도를 삭제하지 않고 유지합니다.</li>
                <li><strong>Ground Truth Over Guessing (에셋 추측 금지 & 메타데이터 확인):</strong> 기존 Blueprint(<span class="code-inline">BP_*</span>), Widget Blueprint(<span class="code-inline">WBP_*</span>), Animation Montage/ABP를 다룰 때 내부 위젯 이름, 노티파이, 슬롯명을 추측하지 않고 반드시 메타데이터 도구(<span class="code-inline">pipeline.py inspect</span> 또는 <span class="code-inline">ue5_inspect_asset</span>)를 통해 JSON 실측 데이터를 확보한 후 작성합니다.</li>
            </ol>
        </div>

        <h3>다. 계층형 파이프라인 프로토콜 (Tiered Pipeline Protocol)</h3>
        <p>AI 에이전트의 불필요한 스크립트 실행과 토큰 낭비를 방지하기 위해 작업 모드를 엄격히 구분합니다:</p>
        <ul>
            <li><strong>기본 모드 (Fast Track):</strong> 일반 코드 편집, 질문 답변, 간단한 리팩토링 시에는 검증 스크립트를 자동 실행하지 않고 깔끔한 C++ 코드와 설명을 즉시 제공합니다.</li>
            <li><strong>온디맨드 검증 모드 (On-Demand Single Tool Call):</strong> 사용자가 컴파일이나 검증을 요청했을 때만 통합 마스터 러너(<span class="code-inline">pipeline.py</span> / <span class="code-inline">pipeline.ps1</span>)를 단일 도구 호출로 실행하여 결과 요약만 반환합니다.</li>
        </ul>
    </div>
    <div class="footer-bar">
        <span>FC Project • Lead Systems & Multiplayer Architect Directives • AI System Architecture Report</span>
        <span>Page 2 of 7 • AGENTS.md Directives</span>
    </div>
</div>

<!-- ================= PAGE 3 ================= -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC ARCHITECTURE DIRECTIVE & AI ECOSYSTEM REPORT</span>
            <span class="tag">MODULAR DOMAIN RULES (.ANTIGRAVITY/RULES/)</span>
            <span>CONFIDENTIAL & PROPRIETARY</span>
        </div>

        <h2>3. 모듈형 도메인 규칙 체계 (.antigravity/rules/)</h2>
        <p>
            모든 규칙을 상시 프롬프트에 담을 경우 발생하는 컨텍스트 낭비와 환각(Hallucination)을 방지하기 위해, 세부 구현 표준은 <span class="code-inline">.antigravity/rules/</span> 디렉토리에 6개의 모듈형 문서로 분리되어 필요 시점에만 온디맨드(On-Demand)로 참조됩니다.
        </p>

        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">01-ue58-coding-standards.md</span>
                    <span class="badge badge-primary">C++20 & UE5.8</span>
                </div>
                <ul>
                    <li><strong>현대 C++20 표준:</strong> <span class="code-inline">/std:c++20</span> 채택, 엔진 표준 컨테이너(<span class="code-inline">TArray</span>, <span class="code-inline">TMap</span>) 사용, 컴파일 타임 <span class="code-inline">constexpr</span> 활용.</li>
                    <li><strong>스마트 포인터 위생:</strong> <span class="code-inline">UPROPERTY</span> 멤버 변수에는 생포인터(<span class="code-inline">T*</span>) 대신 반드시 <span class="code-inline">TObjectPtr&lt;T&gt;</span> 강제, 캐시 참조는 <span class="code-inline">TWeakObjectPtr&lt;T&gt;</span>.</li>
                    <li><strong>IWYU (Include What You Use):</strong> 헤더에는 전방 선언(Forward Declaration)만 사용하고, <span class="code-inline">.generated.h</span>는 헤더의 가장 마지막에 인클루드.</li>
                    <li><strong>최소 수정 및 재활용:</strong> 7장 원칙 준수, 기존 코드 최대 활용.</li>
                </ul>
            </div>

            <div class="card">
                <div class="card-header">
                    <span class="card-title">02-multiplayer-conventions.md</span>
                    <span class="badge badge-warn">NETWORKING</span>
                </div>
                <ul>
                    <li><strong>리플리케이션 수칙:</strong> <span class="code-inline">Net/UnrealNetwork.h</span> 포함, <span class="code-inline">GetLifetimeReplicatedProps</span> 내 Super 호출 필수.</li>
                    <li><strong>조건부 복제 의무화:</strong> 무조건 <span class="code-inline">DOREPLIFETIME</span> 금지. <span class="code-inline">COND_OwnerOnly</span>, <span class="code-inline">COND_SkipOwner</span>, <span class="code-inline">COND_SimulatedOnly</span> 필터링 적용.</li>
                    <li><strong>Server RPC 검증:</strong> 모든 서버 RPC는 <span class="code-inline">WithValidation</span> 지정 및 <span class="code-inline">_Validate</span> / <span class="code-inline">_Implementation</span> 구현 쌍 작성.</li>
                    <li><strong>Multicast RPC 제한:</strong> 게임플레이 상태 변이 금지, 일회성 시각/음향 연출 전용.</li>
                </ul>
            </div>
        </div>

        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">03-loop-engineering.md</span>
                    <span class="badge badge-purple">SELF-HEALING & INSPECT</span>
                </div>
                <ul>
                    <li><strong>자율 자가 치유 루프:</strong> 컴파일 에러 발생 시 사용자 개입 없이 JSON 진단 데이터를 파싱하여 즉시 수정 코드 작성.</li>
                    <li><strong>최대 5회 교정 예산:</strong> 태스크당 최대 5회 반복 시도 후 지속 실패 시 원인 가설 및 에러 상세 리포트 제출.</li>
                    <li><strong>에셋 인스펙션 프로토콜 (5장 신설):</strong> BP/WBP/애니메이션 수정 전 메타데이터 실측 검증 강제 (<span class="code-inline">detected_widgets</span>, <span class="code-inline">detected_notifies</span> 등).</li>
                    <li><strong>에러 트리아지:</strong> C2065, WithValidation 누락, LNK2019 분류.</li>
                </ul>
            </div>

            <div class="card">
                <div class="card-header">
                    <span class="card-title">04-gameplay-ability-system.md</span>
                    <span class="badge badge-danger">GAS & CUE</span>
                </div>
                <ul>
                    <li><strong>ATTRIBUTE_ACCESSORS 매크로:</strong> Getter, Setter, Init, Capture 표준 매크로 강제 적용.</li>
                    <li><strong>어트리뷰트 복제:</strong> <span class="code-inline">DOREPLIFETIME_CONDITION_NOTIFY</span>에 <span class="code-inline">REPNOTIFY_Always</span> 지정 및 <span class="code-inline">GAMEPLAYATTRIBUTE_REPNOTIFY</span> 콜백 사용.</li>
                    <li><strong>클램핑 수칙:</strong> <span class="code-inline">PreAttributeChange</span>(사전 클램프) vs <span class="code-inline">PostGameplayEffectExecute</span>(서버 권한 후처리).</li>
                    <li><strong>Gameplay Cue:</strong> 타격 이펙트/사운드에 멀티캐스트 RPC 대신 네이티브 거리 컬링을 지원하는 Cue 활용.</li>
                </ul>
            </div>
        </div>

        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">05-combat-and-inventory-networking.md</span>
                    <span class="badge badge-success">FAST ARRAY</span>
                </div>
                <ul>
                    <li><strong>FFastArraySerializer:</strong> 대규모 동적 아이템 및 무기 목록 동기화 시 <span class="code-inline">TArray</span> 단순 복제를 금지하고 델타 직렬화 패턴 강제.</li>
                    <li><strong>인벤토리 대역폭:</strong> 컨테이너 프로퍼티를 <span class="code-inline">COND_OwnerOnly</span>로 등록하여 타 플레이어 불필요 패킷 전송 차단.</li>
                    <li><strong>서버 히트 검증:</strong> 클라이언트의 데미지 수치/좌표 불신. 서버가 FOV 각도, 거리, 레이캐스트/스윕을 통해 유효성 직접 검증.</li>
                </ul>
            </div>

            <div class="card">
                <div class="card-header">
                    <span class="card-title">06-umg-mvvm-standards.md</span>
                    <span class="badge badge-slate">UMG MVVM</span>
                </div>
                <ul>
                    <li><strong>클라이언트 전용 뷰모델:</strong> <span class="code-inline">UMVVMViewModelBase</span> 상속, 뷰모델 복제 금지 (OnRep/델리게이트 수신 후 로컬 주입).</li>
                    <li><strong>Zero-Tick 바인딩:</strong> Tick 감시 제거, <span class="code-inline">FieldNotify</span> 브로드캐스트 기반 UI 이벤트 트리거.</li>
                    <li><strong>변이 매크로:</strong> 값 변경 감지 시 <span class="code-inline">UE_MVVM_SET_PROPERTY_VALUE</span>, 컬렉션 갱신 시 <span class="code-inline">UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED</span>.</li>
                    <li><strong>소프트 에셋 참조:</strong> 아이콘/텍스처는 <span class="code-inline">TSoftObjectPtr&lt;UTexture2D&gt;</span> 사용.</li>
                </ul>
            </div>
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project • Lead Systems & Multiplayer Architect Directives • AI System Architecture Report</span>
        <span>Page 3 of 7 • Modular Domain Rules</span>
    </div>
</div>

<!-- ================= PAGE 4 ================= -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC ARCHITECTURE DIRECTIVE & AI ECOSYSTEM REPORT</span>
            <span class="tag">AUTONOMOUS SKILLS FRAMEWORK (.ANTIGRAVITY/SKILLS/)</span>
            <span>CONFIDENTIAL & PROPRIETARY</span>
        </div>

        <h2>4. 자율 스킬 체계 (.antigravity/skills/)</h2>
        <p>
            Antigravity의 Skill(스킬)은 에이전트가 특정 도메인 태스크를 능동적으로 수행할 수 있도록 지침(Instructions), 실행 커맨드(Commands), 리소스(Resources)를 하나로 묶은 최신 에이전트 확장 규격입니다. FC 프로젝트는 언리얼 엔진 5.8 전용 빌드 및 검증 하네스를 <span class="code-inline">ue5-harness</span> 스킬(v1.1.0)로 패키징하여 에이전트의 자율 엔지니어링을 뒷받침합니다.
        </p>

        <div class="callout">
            <div class="callout-title">
                <span>📦 ue5-harness 워크스페이스 스킬 구성 (8개 전용 액션 완비)</span>
                <span class="badge badge-primary">WORKSPACE SKILL V1.1.0</span>
            </div>
            <div style="font-size: 7.8pt; color: #475569;">
                파일 경로: 메인 지침 <span class="code-inline">.antigravity/skills/ue5-harness/SKILL.md</span> | 커맨드 정의 <span class="code-inline">.antigravity/skills/ue5-harness-skill.json</span><br>
                에이전트는 터미널에 긴 인자열을 직접 조합하는 대신, 사전 정의된 스킬 액션을 호출하여 일관되고 격리된 환경에서 언리얼 엔진 툴체인을 구동합니다.
            </div>
        </div>

        <table>
            <thead>
                <tr>
                    <th style="width: 22%;">스킬 액션명</th>
                    <th style="width: 38%;">매핑 실행 명령어</th>
                    <th style="width: 40%;">수행 작업 및 반환 데이터</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>ue5_build</strong></td>
                    <td><span class="code-inline">python Scripts/build_harness.py --pretty</span></td>
                    <td>UBT(UnrealBuildTool) 컴파일 구동, C2xxx/LNKxxxx/UHT 에러 파싱 및 구조화된 JSON 반환</td>
                </tr>
                <tr>
                    <td><strong>ue5_verify_replication</strong></td>
                    <td><span class="code-inline">python Scripts/verify_replication.py --pretty</span></td>
                    <td>Source/ 디렉토리 정적 분석, WithValidation 누락, bReplicates 누락, UI 헤더 오염 탐지</td>
                </tr>
                <tr>
                    <td><strong>ue5_verify_gas</strong></td>
                    <td><span class="code-inline">python Scripts/verify_gas.py --pretty</span></td>
                    <td>UAttributeSet 내 ATTRIBUTE_ACCESSORS 매크로, OnRep_* 콜백 및 GAS 규격 준수 여부 검사</td>
                </tr>
                <tr>
                    <td><strong>ue5_audit_bandwidth</strong></td>
                    <td><span class="code-inline">python Scripts/audit_bandwidth.py --pretty</span></td>
                    <td>원시 TArray 복제 안티패턴, 미필터링 DOREPLIFETIME, 대용량 스트링 복제 탐지</td>
                </tr>
                <tr>
                    <td><strong>ue5_simulate_ttk</strong></td>
                    <td><span class="code-inline">python Scripts/simulate_ttk_balance.py --pretty</span></td>
                    <td>무기 아키타입별 유효 DPS, 아머 티어별 TTK(Time-to-Kill) 커브 수학적 시뮬레이션 및 벤치마크</td>
                </tr>
                <tr>
                    <td><strong>ue5_simulate_net_pie</strong></td>
                    <td><span class="code-inline">python Scripts/simulate_net_pie.py --pretty</span></td>
                    <td>헤드리스 Dedicated Server/Client 구동, 패킷 지연(lag), 손실(loss), 복제(dup) 인젝션 테스트</td>
                </tr>
                <tr>
                    <td><strong>ue5_run_tests</strong></td>
                    <td><span class="code-inline">python Scripts/run_tests.py --pretty</span></td>
                    <td>UnrealEditor-Cmd.exe 헤드리스 자동화 스펙(NullRHI 모드) 실행 및 단위 테스트 상태 리포트</td>
                </tr>
                <tr>
                    <td><strong>ue5_inspect_asset</strong><br><span class="badge badge-success">NEW V1.1.0</span></td>
                    <td><span class="code-inline">python Scripts/dump_asset_metadata.py --json</span></td>
                    <td>BP, WBP, Animation(AnimMontage, ABP) 등 .uasset 파일의 위젯, 노티파이, 슬롯, 뷰모델 구조 메타데이터 JSON 추출</td>
                </tr>
            </tbody>
        </table>

        <h3>🌐 Antigravity 글로벌 Built-in 스킬과의 역할 분담</h3>
        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">시스템 & 환경 제어 스킬</span>
                    <span class="badge badge-slate">GLOBAL</span>
                </div>
                <ul>
                    <li><strong>agy-customizations / antigravity-guide:</strong> Antigravity 커스터마이징 메커니즘, 우선순위, 규칙 로딩, IDE 환경 전반을 안내하는 시스템 레벨 스킬.</li>
                    <li><strong>automation:</strong> 백그라운드 타이머 및 주기적 cron 자동화 작업을 설계하고 스케줄링하는 범용 자동화 스킬.</li>
                </ul>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">시각화 & 도메인 하네스 스킬</span>
                    <span class="badge badge-primary">DOMAIN</span>
                </div>
                <ul>
                    <li><strong>generative_ui:</strong> 채팅 인터페이스 내에 반응형 대화식 HTML 위젯이나 다이어그램을 렌더링하는 시각화 스킬.</li>
                    <li><strong>ue5-harness (프로젝트 전용):</strong> FC 리포지토리에 종속된 특화 스킬로, UE 5.8 C++ 빌드, 에셋 검사, 멀티플레이 정적 검증 파이프라인 전담.</li>
                </ul>
            </div>
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project • Lead Systems & Multiplayer Architect Directives • AI System Architecture Report</span>
        <span>Page 4 of 7 • Skills Architecture</span>
    </div>
</div>

<!-- ================= PAGE 5 ================= -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC ARCHITECTURE DIRECTIVE & AI ECOSYSTEM REPORT</span>
            <span class="tag">MCP PROTOCOL & ASSET REFLECTION (.ANTIGRAVITY/MCP.JSON)</span>
            <span>CONFIDENTIAL & PROPRIETARY</span>
        </div>

        <h2>5. Model Context Protocol (MCP) 및 에셋 리플렉션 연동</h2>
        <p>
            Model Context Protocol (MCP)은 LLM 에이전트와 로컬 개발 환경 간의 표준화된 도구 호출 인터페이스입니다. 터미널 명령어를 직접 파싱하는 수고 없이, 에이전트가 사전 정의된 구조적 도구(Tool)를 함수 호출하듯 실행하고 그 결과를 정형 컨텍스트로 수신합니다.
        </p>

        <h3>가. .antigravity/mcp.json 서버 아키텍처 (8대 MCP 도구 체계)</h3>
        <p>
            MCP 설정 파일에 정의된 <span class="code-inline">tools</span> 블록은 에이전트에게 <strong>8개의 전용 네이티브 함수</strong>로 노출됩니다. 각 도구는 <span class="code-inline">--pretty</span> 플래그를 통해 포맷된 JSON 결과를 스트림으로 반환하여 에이전트의 에러 분석 속도를 극대화합니다.
        </p>

        <div class="diagram-box">
<span style="color: #38bdf8;">// .antigravity/mcp.json 서버 및 도구 등록 구조 (8 Tools Active)</span>
{
  "mcpServers": {
    "ue5-harness": {
      "command": "python",
      "args": ["Scripts/build_harness.py"],
      "env": { "PYTHONUNBUFFERED": "1" },
      "tools": {
        "<strong>ue5_build</strong>": { "command": "python Scripts/build_harness.py --pretty", "description": "Compile UE5.8 project with UBT..." },
        "<strong>ue5_verify_replication</strong>": { "command": "python Scripts/verify_replication.py --pretty", "description": "Verify replication rules..." },
        "<strong>ue5_verify_gas</strong>": { "command": "python Scripts/verify_gas.py --pretty", "description": "GAS compliance inspection..." },
        "<strong>ue5_audit_bandwidth</strong>": { "command": "python Scripts/audit_bandwidth.py --pretty", "description": "Scan network anti-patterns..." },
        "<strong>ue5_simulate_ttk</strong>": { "command": "python Scripts/simulate_ttk_balance.py --pretty", "description": "Execute TTK simulation..." },
        "<strong>ue5_simulate_net_pie</strong>": { "command": "python Scripts/simulate_net_pie.py --pretty", "description": "Net degradation simulation..." },
        "<strong>ue5_run_tests</strong>": { "command": "python Scripts/run_tests.py --pretty", "description": "Headless automation test specs..." },
        "<strong>ue5_inspect_asset</strong>": { "command": "python Scripts/dump_asset_metadata.py --json", "description": "Inspect BP/WBP/Anim metadata..." }
      }
    }
  }
}
        </div>

        <h3>나. MCP 도구 호출 방식과 일반 Shell 실행 방식 비교 우위</h3>
        <table>
            <thead>
                <tr>
                    <th style="width: 22%;">비교 지표</th>
                    <th style="width: 39%;">기존 임의 Shell 실행 (PowerShell/Bash)</th>
                    <th style="width: 39%;">MCP 프로토콜 기반 도구 호출</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>구문 안정성</strong></td>
                    <td>따옴표, 이스케이프 문자 오류, 윈도우 경로 역슬래시 결함 빈발</td>
                    <td>구조화된 인자 전달로 구문 에러율 0% 보장</td>
                </tr>
                <tr>
                    <td><strong>토큰 효율성</strong></td>
                    <td>수천 라인의 원시 컴파일러 로그가 컨텍스트 윈도우 잠식</td>
                    <td><span class="code-inline">--pretty</span> JSON 요약본만 수신하여 토큰 90% 이상 절감</td>
                </tr>
                <tr>
                    <td><strong>에러 트리아지</strong></td>
                    <td>에이전트가 텍스트 정규식을 직접 돌려 파일과 라인 추론</td>
                    <td><span class="code-inline">file</span>, <span class="code-inline">line</span>, <span class="code-inline">code</span>가 정형 JSON 키로 즉시 제공</td>
                </tr>
                <tr>
                    <td><strong>보안 및 격리</strong></td>
                    <td>임의 시스템 명령 실행 가능 위험성 존재</td>
                    <td>사전 승인된 화이트리스트 검증 스크립트만 격리 구동</td>
                </tr>
            </tbody>
        </table>

        <h3>다. 에디터 에셋 2-Tier 인스펙션 & Python Remote Execution 연동</h3>
        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">Tier 1: Fast Standalone AST (~0.005s)</span>
                    <span class="badge badge-primary">ZERO ENGINE OVERHEAD</span>
                </div>
                <div style="font-size: 7.8pt;">
                    • <strong>방식:</strong> UE를 실행하지 않고 <span class="code-inline">dump_asset_metadata.py</span>가 직접 바이너리 패키지 헤더(0x9E2A83C1), NameTable, Import/Export 테이블 파싱.<br>
                    • <strong>추출 정보:</strong> <span class="code-inline">detected_widgets</span>, <span class="code-inline">detected_viewmodels</span>, <span class="code-inline">detected_notifies</span>, <span class="code-inline">parent_class</span>.
                </div>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">Tier 2: Python Remote Execution (~0.05s)</span>
                    <span class="badge badge-success">LIVE EDITOR REFLECTION</span>
                </div>
                <div style="font-size: 7.8pt;">
                    • <strong>설정:</strong> <span class="code-inline">DefaultEngine.ini</span> 내 <span class="code-inline">bRemoteExecution=True</span> 활성화.<br>
                    • <strong>연동:</strong> 에디터 구동 시 멀티캐스트 UDP 소켓 통신을 통해 실행 중인 에디터 내부에서 실시간 CDO 프로퍼티, 노티파이, 슬롯 검사 (Cmdlet 대비 200배 빠름).
                </div>
            </div>
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project • Lead Systems & Multiplayer Architect Directives • AI System Architecture Report</span>
        <span>Page 5 of 7 • MCP & Asset Reflection</span>
    </div>
</div>

<!-- ================= PAGE 6 ================= -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC ARCHITECTURE DIRECTIVE & AI ECOSYSTEM REPORT</span>
            <span class="tag">UNIFIED PIPELINE & SELF-HEALING (SCRIPTS/PIPELINE.PY)</span>
            <span>CONFIDENTIAL & PROPRIETARY</span>
        </div>

        <h2>6. 통합 파이프라인 및 순환 엔지니어링 (Unified Pipeline & Self-Healing)</h2>
        <p>
            여러 도구를 개별 호출하면서 발생하는 컨텍스트 윈도우 오버헤드와 왕복 지연을 방지하기 위해, FC 프로젝트는 단일 마스터 러너인 <span class="code-inline">Scripts/pipeline.py</span>와 이를 감싸는 터미널 래퍼(<span class="code-inline">pipeline.ps1</span> / <span class="code-inline">pipeline.bat</span>)를 운영합니다.
        </p>

        <h3>가. 파이프라인 7대 실행 모드 체계</h3>
        <table>
            <thead>
                <tr>
                    <th style="width: 14%;">모드 명칭</th>
                    <th style="width: 32%;">실행 명령어</th>
                    <th style="width: 14%;">소요 시간</th>
                    <th style="width: 40%;">동작 내용 및 검증 단계</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>Fast Mode</strong></td>
                    <td>(스크립트 미실행 - 기본 상태)</td>
                    <td>0.0s</td>
                    <td>단순 코드 작성, 설명, 리팩토링 시 스크립트를 일절 실행하지 않고 즉시 코드 제공 (토큰 보존)</td>
                </tr>
                <tr>
                    <td><strong>static</strong></td>
                    <td><span class="code-inline">python Scripts/pipeline.py static</span></td>
                    <td>~0.4s</td>
                    <td>GAS 준수, 대역폭 누수, 리플리케이션 위반 초고속 AST 정적 분석 (초저비용 토큰)</td>
                </tr>
                <tr>
                    <td><strong>build</strong></td>
                    <td><span class="code-inline">python Scripts/pipeline.py build</span></td>
                    <td>~5s - 30s</td>
                    <td>UBT 컴파일 구동, C++20/UHT 에러 JSON 트리아지, 에이전트 자가 치유 루프(최대 5회) 연동</td>
                </tr>
                <tr>
                    <td><strong>balance</strong></td>
                    <td><span class="code-inline">python Scripts/pipeline.py balance</span></td>
                    <td>~0.2s</td>
                    <td>전투 수학 모델링, 방어구 관통 공식 기반 TTK 및 유효 DPS 밸런스 지표 산출</td>
                </tr>
                <tr>
                    <td><strong>test</strong></td>
                    <td><span class="code-inline">python Scripts/pipeline.py test</span></td>
                    <td>~10s</td>
                    <td>헤드리스 언리얼 에디터 환경에서 Core Combat 및 AI 유닛 테스트 자동 실행</td>
                </tr>
                <tr>
                    <td><strong>inspect</strong><br><span class="badge badge-success">NEW</span></td>
                    <td><span class="code-inline">python Scripts/pipeline.py inspect --asset &lt;path&gt;</span></td>
                    <td>~0.005s-0.05s</td>
                    <td>BP/WBP/Anim 메타데이터 실측 추출. <span class="code-inline">--deep</span> 추가 시 정밀 엔진 리플렉션 수행</td>
                </tr>
                <tr>
                    <td><strong>full</strong></td>
                    <td><span class="code-inline">python Scripts/pipeline.py full --auto-commit</span></td>
                    <td>순차 실행</td>
                    <td>Static 검증 → Build 하네스 → Test 검증 순차 통과 시 Git 자동 커밋 생성</td>
                </tr>
            </tbody>
        </table>

        <h3>나. 자율 자가 교정(Self-Healing) 엔지니어링 프로세스</h3>
        <div class="callout">
            <div class="callout-title">
                <span>🔄 에러 자동 파싱 및 최대 5회 자가 치유 루프 동작 메커니즘</span>
                <span class="badge badge-primary">AUTONOMOUS LOOP</span>
            </div>
            <ol>
                <li><strong>에이전트 코드 수정:</strong> C++ 소스 또는 헤더 파일 작성.</li>
                <li><strong>검증 호출:</strong> <span class="code-inline">pipeline.py build</span> 또는 <span class="code-inline">pipeline.py static</span> 단일 호출.</li>
                <li><strong>JSON 파싱 & 진단:</strong> 에러 발생 시 UBT 또는 AST 스크립트가 표준 JSON(<span class="code-inline">file</span>, <span class="code-inline">line</span>, <span class="code-inline">code</span>, <span class="code-inline">message</span>) 출력.</li>
                <li><strong>원인 추론 및 코드 패치:</strong> 사용자 개입 없이 에이전트가 해당 라인 전후 컨텍스트를 읽고 즉각 패치 적용.</li>
                <li><strong>루프 반복 (Max 5 Cycles):</strong> 정상 통과할 때까지 최대 5회 재컴파일 수행. 5회 초과 실패 시에만 원인 분석 리포트 제출 후 사용자 피드백 요청.</li>
            </ol>
        </div>

        <h3>다. 에러 코드별 자동 해결 전략 가이드 (Triage Matrix)</h3>
        <table>
            <thead>
                <tr>
                    <th style="width: 30%;">에러 유형 및 코드</th>
                    <th style="width: 35%;">주요 발생 원인</th>
                    <th style="width: 35%;">에이전트 자가 치유 전략</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>C2065: undeclared identifier</strong></td>
                    <td>타입 누락, 필수 헤더 미인클루드, 오타</td>
                    <td>해당 심볼 정의 위치 조사 후 <span class="code-inline">.cpp</span>에 <span class="code-inline">#include</span> 추가 또는 <span class="code-inline">.h</span>에 전방 선언 추가</td>
                </tr>
                <tr>
                    <td><strong>UHT: Missing WithValidation</strong></td>
                    <td>Server RPC에 WithValidation 지정 누락</td>
                    <td><span class="code-inline">UFUNCTION</span> 매크로에 WithValidation 추가 및 <span class="code-inline">_Validate</span> 함수 구현</td>
                </tr>
                <tr>
                    <td><strong>UHT: Missing LifetimeProps</strong></td>
                    <td>Replicated 변수 선언 후 등록 누락</td>
                    <td><span class="code-inline">GetLifetimeReplicatedProps</span> 내 <span class="code-inline">DOREPLIFETIME_CONDITION</span> 등록</td>
                </tr>
                <tr>
                    <td><strong>LNK2019 / LNK2001</strong></td>
                    <td>모듈 종속성 누락 또는 함수 본체 미구현</td>
                    <td><span class="code-inline">FC.Build.cs</span>의 <span class="code-inline">PublicDependencyModuleNames</span>에 모듈 추가 (예: <span class="code-inline">NetCore</span>, <span class="code-inline">GameplayAbilities</span>)</td>
                </tr>
                <tr>
                    <td><strong>UI 오염 (Replication Audit Fail)</strong></td>
                    <td>서버 클래스에 Slate/UMG 위젯 헤더 포함</td>
                    <td>위젯 헤더 인클루드를 클라이언트 전용 클래스로 이동 및 델리게이트 이벤트 구조로 리팩토링</td>
                </tr>
                <tr>
                    <td><strong>에셋 불일치 (Asset Mismatch)</strong></td>
                    <td>WBP 위젯명/노티파이 추측으로 인한 바인딩 결함</td>
                    <td><span class="code-inline">pipeline.py inspect</span>로 실측 메타데이터 덤프 후 C++ 프로퍼티/함수 바인딩 동기화</td>
                </tr>
            </tbody>
        </table>
    </div>
    <div class="footer-bar">
        <span>FC Project • Lead Systems & Multiplayer Architect Directives • AI System Architecture Report</span>
        <span>Page 6 of 7 • Pipeline & Self-Healing</span>
    </div>
</div>

<!-- ================= PAGE 7 ================= -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC ARCHITECTURE DIRECTIVE & AI ECOSYSTEM REPORT</span>
            <span class="tag">SYNTHESIS & PLAYBOOK</span>
            <span>CONFIDENTIAL & PROPRIETARY</span>
        </div>

        <h2>7. 구성 요소 간 상호작용 및 실무 개발 가이드라인</h2>
        <p>프로젝트 내 AI 개발 지원 요소들은 상호 유기적으로 결합되어 다음과 같은 책임 체계를 형성합니다:</p>

        <table>
            <thead>
                <tr>
                    <th style="width: 18%;">구성 요소</th>
                    <th style="width: 25%;">물리 파일 위치</th>
                    <th style="width: 32%;">역할 및 권한 범위</th>
                    <th style="width: 25%;">상호작용 연계점</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>AGENTS.md</strong></td>
                    <td><span class="code-inline">/AGENTS.md</span> (Root)</td>
                    <td>시스템 최상위 헌장: 4계층 권한, 파이프라인 모드, 최소 수정 및 에셋 실측 원칙 정의</td>
                    <td>모든 AI 세션에 기본 주입되며, Rules와 MCP를 총괄 지휘하는 기준점</td>
                </tr>
                <tr>
                    <td><strong>Rules</strong></td>
                    <td><span class="code-inline">.antigravity/rules/*.md</span></td>
                    <td>C++20, 리플리케이션, GAS, 인벤토리, MVVM 도메인별 세부 구현 수칙 및 에셋 인스펙션 규격</td>
                    <td>작업 대상 기능에 따라 온디맨드로 참조되어 프롬프트 팽창 방지</td>
                </tr>
                <tr>
                    <td><strong>Skills</strong></td>
                    <td><span class="code-inline">.antigravity/skills/ue5-harness/</span></td>
                    <td>에이전트가 UBT, 엔진 검증, 에셋 덤퍼를 호출할 수 있는 8대 커맨드 패키지</td>
                    <td>에이전트 액션 인터페이스를 제공하며 <span class="code-inline">mcp.json</span>과 1:1 기능 정렬</td>
                </tr>
                <tr>
                    <td><strong>MCP</strong></td>
                    <td><span class="code-inline">.antigravity/mcp.json</span></td>
                    <td>LLM 컨텍스트와 로컬 파이썬 검증 스크립트를 연결하는 프로토콜 서버</td>
                    <td><span class="code-inline">Scripts/</span> 내 8개 진단 도구들을 구조화된 JSON 출력 툴로 노출</td>
                </tr>
                <tr>
                    <td><strong>Pipeline Scripts</strong></td>
                    <td><span class="code-inline">Scripts/pipeline.py</span> 외 다수</td>
                    <td>실제 AST 파싱, UBT 빌드, 에셋 메타데이터 파싱, 언리얼 에디터 헤더리스 실행 백엔드</td>
                    <td>MCP 툴 및 터미널 래퍼(<span class="code-inline">pipeline.ps1</span> / <span class="code-inline">pipeline.bat</span>)의 실제 구동 엔진</td>
                </tr>
                <tr>
                    <td><strong>Remote Execution</strong></td>
                    <td><span class="code-inline">Config/DefaultEngine.ini</span></td>
                    <td>언리얼 에디터 활성화 중 초고속 실시간 소켓 인터랙션 지원</td>
                    <td><span class="code-inline">benchmark_remote_vs_cmdlet.py</span> 및 실시간 CDO 분석 지원</td>
                </tr>
            </tbody>
        </table>

        <div class="callout">
            <div class="callout-title">
                <span>💡 개발 상황별 실무 적용 가이드라인 (Developer & Agent Playbook)</span>
                <span class="badge badge-primary">ACTION GUIDE</span>
            </div>
            <ul>
                <li><strong>일반 로직 및 C++ 클래스 수정:</strong> <span class="code-inline">AGENTS.md</span>와 <span class="code-inline">01-ue58-coding-standards.md</span>를 준수하여 작성하며, 별도 검증 명령 없이 Fast Mode로 신속히 완료합니다.</li>
                <li><strong>에셋 연동 로직 작성 (신규):</strong> Blueprint, Widget, Animation Montage를 다루기 전 반드시 <span class="code-inline">python Scripts/pipeline.py inspect --asset &lt;path&gt;</span> (또는 MCP <span class="code-inline">ue5_inspect_asset</span>)를 구동하여 슬롯/노티파이/위젯 메타데이터를 확인한 후 C++ 코드를 작성합니다.</li>
                <li><strong>멀티플레이어 액터/컴포넌트 작성:</strong> <span class="code-inline">02-multiplayer-conventions.md</span>에 따라 <span class="code-inline">DOREPLIFETIME_CONDITION</span> 및 Server RPC <span class="code-inline">WithValidation</span>을 작성한 뒤 <span class="code-inline">pipeline.py static</span>으로 정적 검증합니다.</li>
                <li><strong>스킬 및 상태이상 추가:</strong> <span class="code-inline">04-gameplay-ability-system.md</span>를 참조하여 <span class="code-inline">ATTRIBUTE_ACCESSORS</span>와 <span class="code-inline">GameplayCue</span>를 적용하고, <span class="code-inline">pipeline.py static --gas-only</span>로 검증합니다.</li>
                <li><strong>인벤토리/아이템 목록 추가:</strong> <span class="code-inline">05-combat-and-inventory-networking.md</span>를 참조하여 <span class="code-inline">FFastArraySerializer</span>와 <span class="code-inline">COND_OwnerOnly</span>를 구현합니다.</li>
                <li><strong>UI 개발:</strong> <span class="code-inline">06-umg-mvvm-standards.md</span>에 따라 서버 독립적인 <span class="code-inline">UMVVMViewModelBase</span> 및 <span class="code-inline">FieldNotify</span> 바인딩을 구축합니다.</li>
                <li><strong>커밋 전 최종 릴리즈:</strong> <span class="code-inline">python Scripts/pipeline.py full --auto-commit -m "feat: ..."</span> 단일 명령어로 정적 검증, 컴파일, 유닛 테스트를 일괄 통과시킵니다.</li>
            </ul>
        </div>

        <div class="callout" style="border-left-color: #10b981; background: #f0fdf4;">
            <div class="callout-title" style="color: #047857;">
                <span>🏁 결론 및 총평 (Conclusion)</span>
                <span class="badge badge-success">PRODUCTION READY V1.3</span>
            </div>
            <div style="font-size: 8pt; color: #1e293b;">
                FC 프로젝트의 AI 개발 아키텍처는 <strong>최상위 헌장(AGENTS.md)의 엄격한 권한 분리와 최소 침습 통율 하에, 모듈형 도메인 룰(Rules)의 온디맨드 주입, 스킬(Skills)과 MCP를 매개로 한 8대 툴체인 연동, 그리고 에셋 실측 검증(Asset Inspection) 및 자가 치유 루프(Loop Engineering)가 완벽한 유기적 일체</strong>를 이루고 있습니다. 이는 AAA급 언리얼 엔진 5.8 멀티플레이어 환경에서 발생하기 쉬운 네트워크 데싱크, UBT 컴파일 오류, UI 서버 오염, 에셋 바인딩 환각을 사전에 원천 차단하며, 극대화된 개발 속도와 코드 품질을 동시에 보장합니다.
            </div>
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project • Lead Systems & Multiplayer Architect Directives • AI System Architecture Report</span>
        <span>Page 7 of 7 • Conclusion & Playbook</span>
    </div>
</div>

</body>
</html>
"""

def main():
    print("Writing temporary HTML report...")
    OUTPUT_PDF.parent.mkdir(parents=True, exist_ok=True)
    with open(TEMP_HTML, "w", encoding="utf-8") as f:
        f.write(HTML_CONTENT)
    print(f"Temporary HTML saved: {TEMP_HTML}")

    # Locate Chrome or Edge
    chrome_candidates = [
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
    ]
    browser_exe = None
    for cand in chrome_candidates:
        if os.path.exists(cand):
            browser_exe = cand
            break

    if not browser_exe:
        print("[ERROR] Neither Chrome nor Edge executable was found.")
        sys.exit(1)

    print(f"Using browser: {browser_exe}")
    cmd = [
        browser_exe,
        "--headless=new",
        "--disable-gpu",
        "--no-sandbox",
        "--no-pdf-header-footer",
        f"--print-to-pdf={OUTPUT_PDF}",
        str(TEMP_HTML)
    ]

    print(f"Generating PDF: {OUTPUT_PDF}...")
    proc = subprocess.run(cmd, capture_output=True, text=True)
    if proc.returncode != 0:
        print(f"[ERROR] Browser failed with code {proc.returncode}:\n{proc.stderr}")
        sys.exit(1)

    if OUTPUT_PDF.exists():
        size_kb = round(OUTPUT_PDF.stat().st_size / 1024, 1)
        print(f"[SUCCESS] PDF successfully created: {OUTPUT_PDF} ({size_kb} KB)")
        if TEMP_HTML.exists():
            TEMP_HTML.unlink()
    else:
        print("[ERROR] PDF was not generated.")
        sys.exit(1)

if __name__ == "__main__":
    main()
