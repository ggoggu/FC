#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates Volume 1 of FC Antigravity Environment Architecture Report:
"FC 프로젝트 안티그래비티 환경 종합 아키텍처 보고서 [제1권: 플랫폼 생태계 & 코어 런타임]"
Target: Report/Architecture/FC_Antigravity_Vol1_Platform_and_Core_Runtime.pdf
"""

import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
OUTPUT_DIR = PROJECT_ROOT / "Report" / "Architecture"
OUTPUT_PDF = OUTPUT_DIR / "FC_Antigravity_Vol1_Platform_and_Core_Runtime.pdf"
TEMP_HTML = PROJECT_ROOT / "Scripts" / "temp_antigravity_vol1.html"

def build_vol1_html() -> str:
    return """<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<title>안티그래비티 환경 종합 아키텍처 보고서 [제1권: 플랫폼 생태계 & 코어 런타임]</title>
<style>
    @page {
        size: A4 portrait;
        margin: 10mm 12mm 10mm 12mm;
    }
    *, *:before, *:after {
        box-sizing: border-box;
    }
    body {
        font-family: 'Pretendard', -apple-system, BlinkMacSystemFont, 'Malgun Gothic', '맑은 고딕', 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        color: #1e293b;
        background: #ffffff;
        line-height: 1.40;
        font-size: 8.2pt;
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
        max-height: 268mm;
        overflow: hidden;
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
        border-bottom: 1.5px solid #4338ca;
        padding-bottom: 3px;
        margin-bottom: 9px;
        font-size: 7.1pt;
        font-weight: 700;
        color: #64748b;
        letter-spacing: 0.5px;
    }
    .header-bar .tag {
        color: #4338ca;
    }
    .footer-bar {
        border-top: 1px solid #e2e8f0;
        padding-top: 3.5px;
        margin-top: 7px;
        display: flex;
        justify-content: space-between;
        font-size: 6.9pt;
        color: #94a3b8;
    }
    .badge-top {
        display: inline-block;
        background: #4338ca;
        color: #ffffff;
        font-size: 7.0pt;
        font-weight: 700;
        letter-spacing: 0.8px;
        padding: 2px 7px;
        border-radius: 3px;
        text-transform: uppercase;
        margin-bottom: 5px;
    }
    h1 {
        font-size: 16.5pt;
        font-weight: 800;
        color: #0f172a;
        margin: 0 0 3px 0;
        letter-spacing: -0.5px;
        line-height: 1.2;
    }
    .subtitle {
        font-size: 8.8pt;
        color: #475569;
        margin: 0 0 9px 0;
        font-weight: 500;
    }
    .meta-box {
        display: flex;
        background: #f8fafc;
        border: 1px solid #e2e8f0;
        border-radius: 5px;
        padding: 5px 12px;
        font-size: 7.4pt;
        color: #475569;
        justify-content: space-between;
        margin-bottom: 9px;
    }
    .meta-item strong {
        color: #0f172a;
        display: block;
        font-size: 6.6pt;
        text-transform: uppercase;
        margin-bottom: 1px;
    }
    h2 {
        font-size: 10.2pt;
        font-weight: 800;
        color: #0f172a;
        border-left: 3.5px solid #4338ca;
        padding-left: 7px;
        margin: 8px 0 4px 0;
        page-break-after: avoid;
    }
    h3 {
        font-size: 8.4pt;
        font-weight: 700;
        color: #1e293b;
        margin: 5px 0 2px 0;
        page-break-after: avoid;
    }
    p {
        margin: 0 0 4.5px 0;
    }
    ul, ol {
        margin: 0 0 4.5px 0;
        padding-left: 13px;
    }
    li {
        margin-bottom: 1.5px;
    }
    table {
        width: 100%;
        border-collapse: collapse;
        margin: 4px 0 6px 0;
        font-size: 7.2pt;
    }
    th, td {
        border: 1px solid #cbd5e1;
        padding: 3.5px 5.5px;
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
        padding: 1px 4.5px;
        border-radius: 3px;
        font-size: 6.4pt;
        font-weight: 700;
        white-space: nowrap;
    }
    .badge-primary { background: #e0e7ff; color: #4338ca; }
    .badge-cyan { background: #e0f2fe; color: #0284c7; }
    .badge-danger { background: #fee2e2; color: #991b1b; }
    .badge-warn { background: #fef3c7; color: #92400e; }
    .badge-success { background: #dcfce7; color: #166534; }
    .badge-slate { background: #f1f5f9; color: #475569; }

    .callout {
        background: #f8fafc;
        border-left: 3.5px solid #4338ca;
        padding: 5px 9px;
        margin: 4px 0 6px 0;
        border-radius: 0 4px 4px 0;
        font-size: 7.6pt;
    }
    .callout-title {
        font-weight: 700;
        color: #4338ca;
        margin-bottom: 2px;
        display: flex;
        align-items: center;
        justify-content: space-between;
    }
    .callout-success {
        background: #f0fdf4;
        border-left-color: #16a34a;
    }
    .callout-success .callout-title {
        color: #15803d;
    }
    .callout-warn {
        background: #fffbeb;
        border-left-color: #f59e0b;
    }
    .callout-warn .callout-title {
        color: #b45309;
    }
    .callout-cyan {
        background: #f0f9ff;
        border-left-color: #0284c7;
    }
    .callout-cyan .callout-title {
        color: #0369a1;
    }

    .grid-2 {
        display: grid;
        grid-template-columns: 1fr 1fr;
        gap: 7px;
        margin: 4px 0 6px 0;
    }
    .grid-3 {
        display: grid;
        grid-template-columns: 1fr 1fr 1fr;
        gap: 5px;
        margin: 4px 0 6px 0;
    }
    .grid-4 {
        display: grid;
        grid-template-columns: 1fr 1fr 1fr 1fr;
        gap: 5px;
        margin: 4px 0 6px 0;
    }
    .card {
        background: #ffffff;
        border: 1px solid #e2e8f0;
        border-radius: 4px;
        padding: 5px 7px;
    }
    .card-header {
        display: flex;
        justify-content: space-between;
        align-items: center;
        margin-bottom: 3px;
        border-bottom: 1px solid #f1f5f9;
        padding-bottom: 2px;
    }
    .card-title {
        font-weight: 700;
        font-size: 7.5pt;
        color: #0f172a;
    }

    .code-box {
        background: #0f172a;
        color: #e2e8f0;
        border-radius: 4px;
        padding: 5px 8px;
        font-family: 'Consolas', 'Courier New', monospace;
        font-size: 6.8pt;
        line-height: 1.35;
        margin: 4px 0 6px 0;
        overflow: hidden;
    }
    .kw { color: #f472b6; font-weight: 600; }
    .fn { color: #38bdf8; }
    .str { color: #a3e635; }
    .cm { color: #94a3b8; font-style: italic; }
    .ty { color: #fbbf24; }

    .arch-diagram {
        background: #f8fafc;
        border: 1px solid #cbd5e1;
        border-radius: 5px;
        padding: 6px 8px;
        margin: 5px 0 7px 0;
    }
</style>
</head>
<body>

<!-- ==================================================================== -->
<!-- PAGE 1: 표지 및 플랫폼 총론 (Platform Vision & Overarching Architecture) -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 1</span>
            <span class="tag">PLATFORM & CORE RUNTIME</span>
        </div>

        <div style="margin-bottom: 7px;">
            <span class="badge-top">Google Antigravity 2.0 Ecosystem</span>
            <h1>안티그래비티 환경 종합 아키텍처 보고서</h1>
            <div class="subtitle">[제1권] 플랫폼 생태계, 4대 서피스, 비동기 코어 런타임 및 보안 거버넌스 체계</div>
        </div>

        <div class="meta-box">
            <div class="meta-item">
                <strong>Platform Surfaces</strong>
                Antigravity 2.0 / IDE / CLI / SDK
            </div>
            <div class="meta-item">
                <strong>AI Foundation Core</strong>
                Gemini 3.8 Flash & Pro
            </div>
            <div class="meta-item">
                <strong>Target Project</strong>
                FC (Unreal Engine 5.8 C++)
            </div>
            <div class="meta-item">
                <strong>Publication Date</strong>
                2026-10-22
            </div>
            <div class="meta-item">
                <strong>Architect</strong>
                Lead Systems & AI Architect
            </div>
        </div>

        <h2>1. 개요 및 플랫폼 비전 (Executive Vision)</h2>
        <p>
            <strong>Google Antigravity(안티그래비티)</strong>는 대규모 코드베이스의 설계, 리팩토링, 디버깅 및 자율 검증을 위해 설계된 
            <strong>AI-First 에이전틱 개발 플랫폼(Agentic Development Platform)</strong>이다. 
            단순한 코드 자동완성(Copilot형) 보조 도구를 넘어, 개발자와 동등한 권한과 도구를 가지고 프로젝트 환경을 능동적으로 탐색하고 
            실행 루프를 수행하는 <strong>자율 페어 프로그래머(Autonomous Pair Programmer)</strong> 모델을 구현한다.
        </p>

        <div class="grid-4">
            <div class="card" style="border-top: 2.5px solid #4338ca;">
                <div class="card-title">1. 멀티 서피스 단일성</div>
                <p style="font-size: 7.0pt; margin: 2px 0 0 0; color: #475569;">
                    데스크톱 2.0, VS Code 기반 IDE, 헤드리스 CLI(agy), Python SDK가 동일한 에이전트 코어와 도구셋을 공유.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #0284c7;">
                <div class="card-title">2. 리액티브 이벤트 기상</div>
                <p style="font-size: 7.0pt; margin: 2px 0 0 0; color: #475569;">
                    무한 루프 폴링을 원천 차단하고, 서브에이전트 메시지, 백그라운드 태스크 완료 시 자동 기상(Wakeup)하는 이벤트 구동 모델.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #16a34a;">
                <div class="card-title">3. 계층형 서브에이전트</div>
                <p style="font-size: 7.0pt; margin: 2px 0 0 0; color: #475569;">
                    컨텍스트 윈도우 오염 방지를 위해 Research, Self, Custom 에이전트를 생성하고 branch/share 워크스페이스 격리 지원.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #ea580c;">
                <div class="card-title">4. 샌드박스 보안 거버넌스</div>
                <p style="font-size: 7.0pt; margin: 2px 0 0 0; color: #475569;">
                    4단계 도구 실행 정책, 파일/네트워크 접근 제어, 터미널 샌드박스를 통해 시스템 파괴를 원천 방지하는 안전망 제공.
                </p>
            </div>
        </div>

        <h2>2. 플랫폼 최상위 토폴로지 (System Topography)</h2>
        <div class="arch-diagram">
            <div style="text-align: center; font-weight: 700; font-size: 7.5pt; color: #1e1b4b; margin-bottom: 4px;">
                [Google Antigravity End-to-End Architecture]
            </div>
            <div style="display: flex; gap: 5px; justify-content: space-between; margin-bottom: 5px;">
                <div style="flex: 1; background: #e0e7ff; border: 1px solid #c7d2fe; border-radius: 3px; padding: 3px; text-align: center; font-size: 6.9pt;">
                    <strong>Desktop App (2.0)</strong><br><span style="font-size: 6.2pt; color: #4338ca;">Electron / Sidebars / Panes</span>
                </div>
                <div style="flex: 1; background: #e0f2fe; border: 1px solid #bae6fd; border-radius: 3px; padding: 3px; text-align: center; font-size: 6.9pt;">
                    <strong>Antigravity IDE</strong><br><span style="font-size: 6.2pt; color: #0284c7;">VS Code Engine / Tab / Ctrl+I</span>
                </div>
                <div style="flex: 1; background: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 3px; padding: 3px; text-align: center; font-size: 6.9pt;">
                    <strong>CLI (<code>agy</code>)</strong><br><span style="font-size: 6.2pt; color: #334155;">Terminal TUI / CI/CD Automation</span>
                </div>
                <div style="flex: 1; background: #fef3c7; border: 1px solid #fde68a; border-radius: 3px; padding: 3px; text-align: center; font-size: 6.9pt;">
                    <strong>Python SDK</strong><br><span style="font-size: 6.2pt; color: #92400e;">Async Orchestration / Pipeline</span>
                </div>
            </div>
            <div style="background: #1e1b4b; color: #ffffff; border-radius: 3px; padding: 4px 6px; text-align: center; margin-bottom: 5px; font-size: 7.2pt;">
                &darr; <strong>Unified Agentic Core Runtime</strong> (Gemini 3.8 Multi-Modal CoT / Tool Calling / Streaming Bus) &darr;
            </div>
            <div style="display: flex; gap: 5px; justify-content: space-between;">
                <div style="flex: 1; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 3px; font-size: 6.7pt;">
                    <strong style="color: #4338ca;">Context & Memory</strong><br>
                    Progressive Disclosure / Rules / Skills / Transcripts
                </div>
                <div style="flex: 1; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 3px; font-size: 6.7pt;">
                    <strong style="color: #0284c7;">Execution Engine</strong><br>
                    Reactive Wakeup / Background Tasks / Cron
                </div>
                <div style="flex: 1; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 3px; font-size: 6.7pt;">
                    <strong style="color: #16a34a;">Subagents & IPC</strong><br>
                    Dynamic Types / Branch & Share Workspaces
                </div>
                <div style="flex: 1; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 3px; font-size: 6.7pt;">
                    <strong style="color: #ea580c;">Security & Sandbox</strong><br>
                    Tool Policies / Terminal Jail / Allow/Deny Rules
                </div>
            </div>
        </div>

        <h2>3. 보고서 구성 안내 (Report Structure)</h2>
        <p>
            본 보고서는 총 3권으로 구성되며, 각 권은 안티그래비티 환경의 상이한 아키텍처 계층을 심층 분석한다.
        </p>
        <table>
            <tr>
                <th style="width: 15%;">권차</th>
                <th style="width: 32%;">주제 (Title)</th>
                <th style="width: 38%;">주요 분석 대상</th>
                <th style="width: 15%;">문서 형태</th>
            </tr>
            <tr>
                <td><strong>제1권 (본서)</strong></td>
                <td>플랫폼 생태계 & 코어 런타임</td>
                <td>4대 서피스, 비동기 추론 루프, 서브에이전트 IPC, 스케줄링 및 보안 샌드박스</td>
                <td><span class="badge badge-primary">코어 런타임</span></td>
            </tr>
            <tr>
                <td><strong>제2권</strong></td>
                <td>커스터마이징 & 확장성 프레임워크</td>
                <td>Rules, Skills, Plugins, Hooks, MCP 프로토콜, 아티팩트 및 트랜스크립트 상태 관리</td>
                <td><span class="badge badge-cyan">확장 규격</span></td>
            </tr>
            <tr>
                <td><strong>제3권</strong></td>
                <td>FC 언리얼 5.8 실전 엔지니어링 & 루프</td>
                <td>4계층 네트워크 권한, 6대 도메인 룰셋, pipeline.py 계층 러너, Ground Truth 에셋 추출</td>
                <td><span class="badge badge-warn">실전 구현</span></td>
            </tr>
        </table>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 1 &bull; Page 1 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 2: 4대 인터페이스 서피스 정밀 분석 (The 4 Interface Surfaces)    -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 1</span>
            <span class="tag">INTERFACE SURFACES</span>
        </div>

        <h2>1. 안티그래비티 4대 서피스 (Interface Surfaces) 아키텍처</h2>
        <p>
            안티그래비티는 단일 사용 형태에 얽매이지 않고, 개발자의 작업 맥락(Context)에 따라 최적화된 4가지 독립 접점을 제공한다.
            모든 서피스는 백엔드에서 <strong>동일한 에이전트 바이너리와 설정 저장소</strong>를 공유하므로 상태 연속성이 보장된다.
        </p>

        <div class="grid-2">
            <!-- Surface 1: Desktop App -->
            <div class="card">
                <div class="card-header">
                    <span class="card-title">1. Antigravity 2.0 (Desktop Electron App)</span>
                    <span class="badge badge-primary">통합 대시보드</span>
                </div>
                <p style="font-size: 7.1pt; color: #334155; margin-bottom: 3px;">
                    IDE와 독립적으로 실행되는 풀스택 데스크톱 클라이언트. 에이전트의 전체 오케스트레이션과 백그라운드 태스크 모니터링에 특화.
                </p>
                <ul style="font-size: 6.9pt; color: #475569;">
                    <li><strong>Left Sidebar:</strong> New Conversation, Projects Switcher, Scheduled Tasks(Cron/Timer), Skills/Plugins 매니저.</li>
                    <li><strong>Chat Canvas:</strong> 슬래시 커맨드(<code>/</code>), 컨텍스트 멘션(<code>@</code>: 파일, 규칙, 세션, MCP), 미디어 드래그 앤 드롭 지원.</li>
                    <li><strong>Auxiliary Pane (보조 패널):</strong> Subagents 실시간 상태, Background Tasks, Artifacts(문서/다이어그램), Terminals, Files Changed 시각화.</li>
                </ul>
            </div>

            <!-- Surface 2: IDE -->
            <div class="card">
                <div class="card-header">
                    <span class="card-title">2. Antigravity IDE (VS Code Fork)</span>
                    <span class="badge badge-cyan">코드 작성 특화</span>
                </div>
                <p style="font-size: 7.1pt; color: #334155; margin-bottom: 3px;">
                    개발 환경 내부에 네이티브로 통합된 AI-First 에디터. 3단계 AI 모달리티를 통해 타이핑 단위부터 대규모 리팩토링까지 지원.
                </p>
                <ul style="font-size: 6.9pt; color: #475569;">
                    <li><strong>Passive (Tab):</strong> 단일 키로 다음 의도 예측(Next-Intent). Autocomplete, Supercomplete, Tab to Jump, Tab to Import.</li>
                    <li><strong>Instructive (Ctrl+I):</strong> 에디터 내부 선택 영역 인라인 명령. 집중된 리팩토링, 주석/Docstring 생성, 보일러플레이트 작성.</li>
                    <li><strong>Collaborative (Sidebar Agent):</strong> 파일 생성/편집, 빌드 실행, MCP 도구 연동을 포함하는 대규모 페어 프로그래밍.</li>
                </ul>
            </div>

            <!-- Surface 3: CLI agy -->
            <div class="card">
                <div class="card-header">
                    <span class="card-title">3. Antigravity CLI (<code>agy</code>)</span>
                    <span class="badge badge-slate">헤드리스 & 터미널</span>
                </div>
                <p style="font-size: 7.1pt; color: #334155; margin-bottom: 3px;">
                    터미널 환경에서 빠르고 경량화된 에이전트 상호작용 및 CI/CD 자동화 파이프라인 연동을 위한 전용 TUI 도구.
                </p>
                <ul style="font-size: 6.9pt; color: #475569;">
                    <li><strong>Launch & Auth:</strong> <code>agy</code> 명령어로 즉시 구동, OAuth/API 키 기반 간편 인증.</li>
                    <li><strong>Interactive TUI:</strong> 터미널 내에서 슬래시 커맨드 및 대화형 인터페이스 제공 (<code>Ctrl+D</code>로 종료).</li>
                    <li><strong>Settings:</strong> <code>~/.gemini/antigravity-cli/settings.json</code>을 통한 모델 및 터미널 렌더링 세부 튜닝.</li>
                </ul>
            </div>

            <!-- Surface 4: Python SDK -->
            <div class="card">
                <div class="card-header">
                    <span class="card-title">4. Antigravity Python SDK (<code>google-antigravity</code>)</span>
                    <span class="badge badge-warn">파이프라인 프로그래밍</span>
                </div>
                <p style="font-size: 7.1pt; color: #334155; margin-bottom: 3px;">
                    파이썬 스크립트, 자동화 테스트 및 CI/CD 워크플로 내에서 프로그래밍 방식으로 에이전트를 스폰하고 오케스트레이션.
                </p>
                <ul style="font-size: 6.9pt; color: #475569;">
                    <li><strong>Async Context Manager:</strong> <code>async with Agent(config) as agent:</code>를 통한 완전한 생명주기 관리.</li>
                    <li><strong>Event Streaming:</strong> 응답 토큰뿐 아니라 내부 사고(<code>response.thoughts</code>) 및 도구 호출(<code>response.tool_calls</code>) 실시간 가로채기.</li>
                    <li><strong>Tool Wiring & Policies:</strong> 커스텀 도구 바인딩 및 Capability 정책의 코드 레벨 제어.</li>
                </ul>
            </div>
        </div>

        <h2>2. 4대 인터페이스 서피스 상세 비교 분석표</h2>
        <table>
            <tr>
                <th style="width: 17%;">구분 항목</th>
                <th style="width: 21%;">Antigravity 2.0 (Desktop)</th>
                <th style="width: 21%;">Antigravity IDE</th>
                <th style="width: 20%;">CLI (<code>agy</code>)</th>
                <th style="width: 21%;">Python SDK</th>
            </tr>
            <tr>
                <td><strong>주요 사용 목적</strong></td>
                <td>전체 프로젝트 조망, 태스크 스케줄링, 서브에이전트 총괄</td>
                <td>실시간 인라인 코딩, 자동완성, 심볼 렌즈, 코드 리팩토링</td>
                <td>터미널 작업 중 빠른 질의, 서버 환경, 경량 스크립팅</td>
                <td>자동화 파이프라인, 테스트 하네스, 커스텀 에이전트 빌드</td>
            </tr>
            <tr>
                <td><strong>UI 형태</strong></td>
                <td>Electron 독립 데스크톱 윈도우</td>
                <td>VS Code 기반 통합 개발 환경</td>
                <td>터미널 TUI (Rich Console)</td>
                <td>Headless Python 라이브러리</td>
            </tr>
            <tr>
                <td><strong>컨텍스트 주입</strong></td>
                <td><code>@</code> 멘션 메뉴, 파일 드래그 앤 드롭</td>
                <td>열린 탭, 커서 위치, 진단 목록 자동 수집</td>
                <td>명령 인자, 파이프(stdin), 파일 경로</td>
                <td>API 파라미터, 인메모리 버퍼 객체</td>
            </tr>
            <tr>
                <td><strong>백그라운드 제어</strong></td>
                <td>전용 Auxiliary Tasks 패널에서 직관적 제어</td>
                <td>사이드바 패널 연동 및 상태 표시</td>
                <td>CLI 백그라운드 프로세스 관리</td>
                <td>Asyncio 이벤트 루프 기반 제어</td>
            </tr>
            <tr>
                <td><strong>커스터마이징 범위</strong></td>
                <td>전역 및 프로젝트 설정, GUI 관리기</td>
                <td><code>.agents/</code> 및 인라인 코드 렌즈</td>
                <td><code>settings.json</code> 및 플래그</td>
                <td>코드 기반 <code>CapabilitiesConfig</code> 주입</td>
            </tr>
        </table>

        <div class="callout callout-cyan">
            <div class="callout-title">
                <span>[Architectural Insight] 서피스 간 상태 일관성 보장 메커니즘</span>
                <span class="badge badge-cyan">Unified Backend</span>
            </div>
            모든 서피스는 로컬 파일 시스템의 <code>~/.gemini/config/</code> 및 프로젝트 루트의 <code>.antigravity/</code> 또는 <code>.agents/</code>를 공유한다.
            예를 들어 IDE에서 작성된 규칙이나 스킬은 데스크톱 2.0 및 CLI에서도 즉시 동일한 우선순위로 로드되며,
            트랜스크립트 로그(<code>transcript.jsonl</code>) 역시 표준화된 경로에 기록되어 도구 간 세션 연속성을 완벽히 유지한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 1 &bull; Page 2 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 3: 에이전틱 코어 런타임 & 추론 루프 (Agentic Core Runtime & Loop)  -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 1</span>
            <span class="tag">AGENTIC RUNTIME & REASONING</span>
        </div>

        <h2>1. 에이전틱 코어 런타임 추론 루프 (Reasoning & Dispatch Loop)</h2>
        <p>
            안티그래비티 코어 런타임은 Gemini 3.8의 멀티모달 추론 역량을 기반으로 
            <strong>관찰(Observe) &rarr; 분석/사고(Deliberate/Think) &rarr; 도구 호출(Dispatch) &rarr; 피드백 평가(Evaluate)</strong>의 
            폐루프(Closed Loop) 아키텍처를 형성한다.
        </p>

        <div class="arch-diagram">
            <div style="display: flex; gap: 6px; align-items: center; justify-content: space-between;">
                <div style="flex: 1; background: #e0e7ff; border: 1.5px solid #4338ca; border-radius: 4px; padding: 5px; font-size: 6.8pt;">
                    <strong style="color: #4338ca;">1. Context Ingestion</strong><br>
                    사용자 인텐트, 계층적 규칙(AGENTS.md), 점진적 스킬 메타데이터, 트랜스크립트 이력 병합
                </div>
                <div style="font-size: 10pt; color: #4338ca; font-weight: 800;">&rarr;</div>
                <div style="flex: 1; background: #fdf2f8; border: 1.5px solid #db2777; border-radius: 4px; padding: 5px; font-size: 6.8pt;">
                    <strong style="color: #db2777;">2. Chain-of-Thought</strong><br>
                    심층 추론(Deep Thinking), 변경 파급 효과 분석, 최소 수정 원칙 및 실행 전략 수립
                </div>
                <div style="font-size: 10pt; color: #db2777; font-weight: 800;">&rarr;</div>
                <div style="flex: 1; background: #f0fdf4; border: 1.5px solid #16a34a; border-radius: 4px; padding: 5px; font-size: 6.8pt;">
                    <strong style="color: #16a34a;">3. Tool Call Dispatch</strong><br>
                    보안 정책 필터링 통과 후 <code>run_command</code>, <code>edit_file</code>, <code>read_file</code> 호출
                </div>
                <div style="font-size: 10pt; color: #16a34a; font-weight: 800;">&rarr;</div>
                <div style="flex: 1; background: #fff7ed; border: 1.5px solid #ea580c; border-radius: 4px; padding: 5px; font-size: 6.8pt;">
                    <strong style="color: #ea580c;">4. Feedback Loop</strong><br>
                    컴파일러/명령어 표준 출력 및 종료 코드 수집, 오류 발생 시 자동 수정(Auto-Patch)
                </div>
            </div>
        </div>

        <h2>2. 실시간 스트리밍 버스 (Real-Time Streaming Pipeline)</h2>
        <p>
            런타임은 에이전트의 내부 상태 변화를 실시간 스트림(Stream) 이벤트로 외부에 전파하여, 
            UI 레이어와 SDK 클라이언트가 지연 없이 에이전트의 사고와 행동을 시각화할 수 있도록 지원한다.
        </p>

        <div class="grid-3">
            <div class="card" style="border-left: 3px solid #8b5cf6;">
                <div class="card-title" style="color: #7c3aed;">1. Thought Stream</div>
                <div class="code-box" style="margin: 3px 0; font-size: 6.3pt; padding: 3px 5px;">
                    <span class="kw">async for</span> thought <span class="kw">in</span> resp.thoughts:<br>
                    &nbsp;&nbsp;render_thinking_bubble(thought)
                </div>
                <p style="font-size: 6.8pt; color: #475569; margin: 0;">
                    모델의 CoT(Chain-of-Thought) 추론 조각을 실시간 수신하여 에이전트의 문제 해결 방향성을 투명하게 공개.
                </p>
            </div>
            <div class="card" style="border-left: 3px solid #0284c7;">
                <div class="card-title" style="color: #0369a1;">2. Tool Call Stream</div>
                <div class="code-box" style="margin: 3px 0; font-size: 6.3pt; padding: 3px 5px;">
                    <span class="kw">async for</span> call <span class="kw">in</span> resp.tool_calls:<br>
                    &nbsp;&nbsp;audit_and_dispatch(call.name, call.args)
                </div>
                <p style="font-size: 6.8pt; color: #475569; margin: 0;">
                    강타입화된 도구 호출 객체를 실시간 전송. 보안 가드레일이 실행 전 인수를 사전 가로채기(Interception) 가능.
                </p>
            </div>
            <div class="card" style="border-left: 3px solid #10b981;">
                <div class="card-title" style="color: #047857;">3. Content Token Stream</div>
                <div class="code-box" style="margin: 3px 0; font-size: 6.3pt; padding: 3px 5px;">
                    <span class="kw">async for</span> token <span class="kw">in</span> resp:<br>
                    &nbsp;&nbsp;stdout.write(token)
                </div>
                <p style="font-size: 6.8pt; color: #475569; margin: 0;">
                    최종 생성된 마크다운 텍스트 토큰을 스트리밍 출력하여 사용자 대기 시간(Time-to-First-Token)을 극소화.
                </p>
            </div>
        </div>

        <h2>3. 반응형 기상 아키텍처 (Reactive Wakeup Engine)</h2>
        <div class="callout callout-success">
            <div class="callout-title">
                <span>[핵심 아키텍처 원칙] Busy Polling의 원천 배제와 Reactive Wakeup</span>
                <span class="badge badge-success">Zero-Polling Protocol</span>
            </div>
            기존의 나이브한 에이전트는 백그라운드 태스크나 서브에이전트의 종료를 대기하기 위해 <code>while sleep(5): check()</code> 형태의 루프를 돌며 
            치명적인 토큰 낭비와 컨텍스트 고갈을 유발했다. 안티그래비티는 이를 엄격히 금지하며, 
            <strong>이벤트 드리븐 반응형 기상(Reactive Wakeup)</strong> 메커니즘을 전면 적용한다.
        </div>

        <div class="grid-2">
            <div>
                <h3>반응형 기상 트리거 소스 (Wakeup Triggers)</h3>
                <ul>
                    <li><strong>서브에이전트 통신:</strong> 위임된 서브에이전트가 작업을 마치고 결과를 전송하거나 가이드 요청을 보낼 때 런타임이 부모를 즉시 기상.</li>
                    <li><strong>백그라운드 태스크 완료:</strong> 비동기로 실행 중인 UBT 컴파일러, 패키징 스크립트 종료 시 이벤트 디스패치.</li>
                    <li><strong>사용자 큐 메시지 도착:</strong> 사용자가 에이전트 실행 도중 추가로 전송한 사용자 큐 메시지가 디큐(Dequeue)될 때.</li>
                    <li><strong>타이머 및 크론 만료:</strong> <code>schedule</code> 도구로 등록된 단발성 타이머 만료 또는 크론 주기 도달 시.</li>
                </ul>
            </div>
            <div>
                <h3>기상 동작 시 런타임 상태 복원 흐름</h3>
                <div class="code-box">
<span class="cm"># Reactive Wakeup State Transition</span>
1. Agent enters SLEEP state (No tools called)
2. Event Bus receives: <span class="str">"TaskFinished(TaskId=B49, Code=0)"</span>
3. Runtime captures output payload into Context
4. Priority notification injected into Agent turn:
   <span class="str">"[SYSTEM EVENT] Background task B49 completed successfully."</span>
5. Agent automatically RESUMES execution without user intervention.
                </div>
            </div>
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 1 &bull; Page 3 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 4: 서브에이전트 계층 & 프로세스 간 통신 (Subagents Hierarchy & IPC) -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 1</span>
            <span class="tag">SUBAGENTS & IPC</span>
        </div>

        <h2>1. 서브에이전트 계층 아키텍처 (Subagent Hierarchy)</h2>
        <p>
            단일 에이전트가 방대한 코드베이스 검색, 대량 컴파일 오류 분석, 다중 파일 리팩토링을 단일 대화 창에서 모두 수행하면 
            <strong>컨텍스트 윈도우 한계 도달, 어텐션 분산, 속도 저하</strong>가 발생한다. 
            안티그래비티는 작업을 논리적 단위로 분할하여 독립된 컨텍스트를 가진 서브에이전트에게 위임(Delegate)하는 계층 구조를 채택한다.
        </p>

        <div class="grid-3">
            <div class="card" style="border-top: 3px solid #4338ca;">
                <div class="card-title"><code>self</code> Subagent</div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    부모 에이전트의 전체 도구(파일 편집, 터미널 실행 등), 시스템 프롬프트 및 모델 구성을 그대로 상속.
                </p>
                <div style="font-size: 6.6pt; color: #64748b; background: #f8fafc; padding: 3px; border-radius: 2px;">
                    <strong>용도:</strong> 독립적인 작업 분기가 필요하지만 부모와 동일한 완전한 권한이 요구되는 복잡한 구현 작업.
                </div>
            </div>
            <div class="card" style="border-top: 3px solid #0284c7;">
                <div class="card-title"><code>research</code> Subagent</div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    읽기 전용 도구(<code>read_file</code>, <code>search_web</code>)만 보유하여 파일이나 시스템을 변이시키지 않음.
                </p>
                <div style="font-size: 6.6pt; color: #64748b; background: #f8fafc; padding: 3px; border-radius: 2px;">
                    <strong>용도:</strong> 수십 개의 헤더/CPP 파일 탐색, 웹 API 규격 조사 등 부모의 컨텍스트를 오염시키지 않는 대규모 정탐.
                </div>
            </div>
            <div class="card" style="border-top: 3px solid #10b981;">
                <div class="card-title"><code>define_subagent</code> (Dynamic)</div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    런타임에 특화된 역할, 격리된 도구 권한(EnableWriteTools 등), 전용 시스템 프롬프트를 정의하여 스폰.
                </p>
                <div style="font-size: 6.6pt; color: #64748b; background: #f8fafc; padding: 3px; border-radius: 2px;">
                    <strong>용도:</strong> DB 스키마 마이그레이션 전문 에이전트, 언리얼 리플렉션 전용 에이전트 등 고도로 특화된 작업.
                </div>
            </div>
        </div>

        <h2>2. 워크스페이스 격리 모드 (Workspace Isolation Modes)</h2>
        <p>
            서브에이전트가 소스코드를 수정할 때 운영 체제 및 Git 레벨에서 부모 워크스페이스와의 충돌을 방지하기 위해 3가지 격리 모드를 제공한다.
        </p>

        <table>
            <tr>
                <th style="width: 15%;">격리 모드</th>
                <th style="width: 35%;">작동 원리 (Mechanism)</th>
                <th style="width: 25%;">리소스 및 오버헤드</th>
                <th style="width: 25%;">최적 사용 사례</th>
            </tr>
            <tr>
                <td><strong><code>inherit</code></strong> (기본)</td>
                <td>부모와 동일한 실제 디렉터리 경로를 직접 공유하여 파일 작업 수행.</td>
                <td>오버헤드 제로, 즉각적 파일 변경 반영.</td>
                <td>단일 파일의 사소한 편집, 정적 분석 및 컴파일 검증.</td>
            </tr>
            <tr>
                <td><strong><code>branch</code></strong></td>
                <td>부모 저장소로부터 완전히 독립된 격리 워크스페이스 폴더를 물리적으로 복제/분기.</td>
                <td>디스크 복제 시간 및 용량 소모 발생.</td>
                <td>실패 가능성이 높은 파괴적 대규모 아키텍처 리팩토링, 실험적 빌드.</td>
            </tr>
            <tr>
                <td><strong><code>share</code></strong></td>
                <td>Git worktree 또는 Mercurial <code>hg share</code> 메커니즘을 활용하여 단일 스토리지를 기반으로 독립 브랜칭.</td>
                <td>스토리지 복제 없이 거의 0에 가까운 디스크 오버헤드로 독립성 제공.</td>
                <td>고용량 언리얼 프로젝트(FC)에서 안전한 멀티 브랜치 동시 수정 및 테스트.</td>
            </tr>
        </table>

        <h2>3. 서브에이전트 프로세스 간 통신 (IPC) 및 생명주기 관리</h2>
        <div class="arch-diagram">
            <div style="font-size: 7.0pt; font-family: monospace; line-height: 1.4; color: #1e293b;">
                <strong>[Parent Agent]</strong> &mdash;&mdash;<code>invoke_subagent(Role="Code Researcher", Type="research")</code>&mdash;&rarr; <strong>[Subagent Runtime]</strong><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&vert;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&vert;&larr;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash; Returns unique <code>ConversationId</code> (e.g., "conv-9f82a") &mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&bull; Parent stops calling tools &rarr; enters Sleep (Reactive Wait)<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&bull; Subagent independently queries 20+ files, analyzes symbols, synthesizes findings<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&vert;&larr;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash; Subagent sends completion payload / artifact link &mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                <strong>[Parent Resumes]</strong> &rarr; Ingests synthesized findings &rarr; Calls <code>manage_subagents(Action="kill")</code> &rarr; Workspace Cleaned.
            </div>
        </div>

        <div class="callout callout-warn">
            <div class="callout-title">
                <span>[통신 프로토콜 주의사항] <code>send_message</code>의 용도 제한</span>
                <span class="badge badge-warn">Protocol Strictness</span>
            </div>
            <code>send_message</code> 도구는 시스템 내부에서 부모와 서브에이전트 간의 <strong>프로세스 간 데이터 교환(IPC)</strong> 전용이다.
            사용자에게 응답을 전달할 때 이 도구를 사용해서는 절대 안 되며, 사용자와의 소통은 반드시 <strong>최종 가시적 텍스트 출력</strong>을 통해 이루어져야 한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 1 &bull; Page 4 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 5: 백그라운드 태스크 및 스케줄링 엔진 (Background Tasks & Cron)    -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 1</span>
            <span class="tag">BACKGROUND TASKS & SCHEDULER</span>
        </div>

        <h2>1. 비동기 백그라운드 태스크 엔진 (Background Task Engine)</h2>
        <p>
            언리얼 엔진 빌드(UBT), 셰이더 컴파일, 패키징 등의 작업은 수 분 이상 소요될 수 있다. 
            안티그래비티는 동기식 블로킹으로 인한 타임아웃을 방지하기 위해 
            <strong>자동 비동기 전환 및 데몬 태스크 관리 아키텍처</strong>를 제공한다.
        </p>

        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">일반 명령어의 비동기 승격 (Async Escalation)</span>
                    <span class="badge badge-primary">Dynamic Backgrounding</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    <code>run_command</code> 실행 시 <code>WaitMsBeforeAsync</code>(500~10000ms) 시간을 지정한다. 
                    지정된 시간 내에 명령어가 종료되지 않으면, 시스템은 이를 자동으로 <strong>백그라운드 태스크</strong>로 전환하고 
                    고유 <code>TaskId</code>를 발급한 뒤 에이전트에게 제어권을 즉시 반환한다.
                </p>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">상시 데몬 프로세스 (Daemon Processes)</span>
                    <span class="badge badge-warn">Long-Running Support</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    개발 웹 서버, Live Coding 리스너, 파일 감시자(File Watcher)와 같이 종료되지 않고 무기한 동작해야 하는 프로세스는 
                    <code>IsDaemon: true</code> 플래그를 설정하여 스폰한다. 
                    시스템은 프로세스가 종료되지 않더라도 성공적으로 백그라운드에 안착했음을 확인하고 에이전트를 차단하지 않는다.
                </p>
            </div>
        </div>

        <h2>2. 백그라운드 태스크 제어 인터페이스 (<code>manage_task</code>)</h2>
        <table>
            <tr>
                <th style="width: 18%;">Action</th>
                <th style="width: 25%;">파라미터</th>
                <th style="width: 37%;">동작 설명 및 내부 메커니즘</th>
                <th style="width: 20%;">반환 데이터</th>
            </tr>
            <tr>
                <td><strong><code>list</code></strong></td>
                <td>-</td>
                <td>현재 시스템에서 백그라운드로 실행 중인 모든 태스크의 목록과 상태를 조회.</td>
                <td><code>TaskId</code>, 실행 명령, 실행 시간, 상태</td>
            </tr>
            <tr>
                <td><strong><code>status</code></strong></td>
                <td><code>TaskId</code> (필수)</td>
                <td>지정된 태스크의 최신 표준 출력(stdout) 및 표준 에러(stderr) 버퍼 로그를 실시간 확인.</td>
                <td>최근 N줄 로그 버퍼, CPU/상태</td>
            </tr>
            <tr>
                <td><strong><code>send_input</code></strong></td>
                <td><code>TaskId</code>, <code>Input</code> (필수)</td>
                <td>실행 중인 대화형 프로세스의 <code>stdin</code> 파이프로 텍스트 입력을 직접 주입 (예: Y/N 확인, 디버거 명령).</td>
                <td>입력 전달 성공 여부</td>
            </tr>
            <tr>
                <td><strong><code>kill</code></strong></td>
                <td><code>TaskId</code> (필수)</td>
                <td>비정상 루프에 빠진 태스크 또는 불필요해진 백그라운드 프로세스 프로세스 트리를 강제 종료 (SIGTERM/SIGKILL).</td>
                <td>프로세스 종료 확인 코드</td>
            </tr>
        </table>

        <h2>3. 스케줄링 엔진 (<code>schedule</code> Engine)</h2>
        <p>
            에이전트가 특정 시점이나 주기마다 작업을 자율 수행하도록 타이머 및 크론(Cron) 작업을 스케줄링할 수 있다.
            <strong>쉘의 <code>sleep</code> 명령어를 절대 사용하지 않고</strong> 스케줄러를 통해 이벤트를 수신한다.
        </p>

        <div class="grid-2">
            <div class="card" style="border-left: 3.5px solid #0284c7;">
                <div class="card-title" style="color: #0369a1;">단발성 지연 타이머 (One-Shot Timer)</div>
                <div class="code-box" style="margin: 3px 0; font-size: 6.5pt;">
schedule(DurationSeconds=300, Prompt="300초 경과: 서버 패키징 완료 여부 확인")
                </div>
                <p style="font-size: 6.9pt; color: #475569; margin: 0;">
                    지정된 초 단위 시간 경과 후 시스템이 높은 우선순위 알림(Prompt)을 발행하여 에이전트를 깨움.
                    <code>TimerCondition</code>을 설정하면 특정 메시지 도착 시 타이머를 조기 취소할 수 있음.
                </p>
            </div>
            <div class="card" style="border-left: 3.5px solid #16a34a;">
                <div class="card-title" style="color: #15803d;">주기적 크론 작업 (Recurring Cron Job)</div>
                <div class="code-box" style="margin: 3px 0; font-size: 6.5pt;">
schedule(CronExpression="*/10 * * * *", Prompt="10분 주기: 깃 리포지토리 동기화 점검")
                </div>
                <p style="font-size: 6.9pt; color: #475569; margin: 0;">
                    표준 5-field 크론 문법(분 시 일 월 요일)을 지원. <code>MaxIterations</code>를 통해 최대 반복 횟수를 제한하거나, 
                    <code>IsDaemon: true</code>로 현재 대화 세션 종료 후에도 영속 동작하게 설정 가능.
                </p>
            </div>
        </div>

        <div class="callout callout-cyan">
            <div class="callout-title">
                <span>[아키텍처 통합] 비동기 태스크와 스케줄러의 유기적 결합 흐름</span>
                <span class="badge badge-cyan">Workflow Automation</span>
            </div>
            대규모 언리얼 프로젝트에서 에이전트는 장시간 컴파일 작업(<code>pipeline.py build</code>)을 비동기 태스크로 구동한 후, 
            별도의 지연 루프 없이 즉시 턴을 종료한다. 태스크가 종료되면 운영체제 시그널이 에이전트 런타임 이벤트 버스로 전달되어 
            에이전트를 자동 기상시키며, 에이전트는 컴파일 결과를 읽어 즉각 다음 단계(자율 패치 또는 테스트)를 수행한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 1 &bull; Page 5 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 6: 보안 거버넌스, 권한 정책 및 샌드박스 격리 (Security & Sandboxing) -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 1</span>
            <span class="tag">SECURITY & SANDBOXING</span>
        </div>

        <h2>1. 도구 실행 정책 (Tool Execution Policy) 4단계</h2>
        <p>
            안티그래비티는 자율성과 안정성 사이의 균형을 맞추기 위해 사용자가 환경의 신뢰 수준에 따라 선택할 수 있는 
            <strong>4단계 도구 실행 정책 모델</strong>을 엄격히 적용한다.
        </p>

        <table>
            <tr>
                <th style="width: 22%;">정책 모드</th>
                <th style="width: 38%;">도구 실행 시 동작 (Behavior)</th>
                <th style="width: 22%;">적용 대상 환경</th>
                <th style="width: 18%;">위험 방어 수준</th>
            </tr>
            <tr>
                <td><strong><code>always-proceed</code></strong></td>
                <td>사용자 개입 없이 모든 파일 읽기/쓰기, 터미널 명령어, 도구 호출을 전자동 실행.</td>
                <td>신뢰된 로컬 프로젝트, 고속 자동화 CI/CD</td>
                <td><span class="badge badge-warn">주의 (신뢰 환경)</span></td>
            </tr>
            <tr>
                <td><strong><code>request-review</code></strong> (기본 권장)</td>
                <td>읽기 작업은 자동 실행하되, 파일 생성/수정, 쉘 명령어 등 시스템 변이 시 승인 모달 출력.</td>
                <td>일반 개발 환경 및 페어 프로그래밍</td>
                <td><span class="badge badge-primary">균형적 (표준)</span></td>
            </tr>
            <tr>
                <td><strong><code>strict</code></strong></td>
                <td>단순 파일 읽기를 포함한 모든 단일 도구 호출마다 사용자의 명시적 승인을 요구.</td>
                <td>극도의 보안이 요구되는 레거시/운영 서버</td>
                <td><span class="badge badge-danger">최고 보안 (수동)</span></td>
            </tr>
            <tr>
                <td><strong><code>proceed-in-sandbox</code></strong></td>
                <td>모든 터미널 명령어와 프로세스 실행을 격리된 <strong>터미널 샌드박스</strong> 내부로 강제 라우팅.</td>
                <td>외부 오픈소스 분석, 비검증 패키지 테스트</td>
                <td><span class="badge badge-success">완전 격리 (안전)</span></td>
            </tr>
        </table>

        <h2>2. 터미널 샌드박스 및 리소스 접근 제어 아키텍처</h2>
        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">1. 파일 시스템 가상화 및 경계 제어</span>
                    <span class="badge badge-primary">File Boundary</span>
                </div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    에이전트의 작업 영역은 원칙적으로 워크스페이스 루트(Workspace Root)로 한정된다.
                </p>
                <ul style="font-size: 6.8pt; color: #475569;">
                    <li><strong>Non-Workspace File Policy:</strong> 루트 외부 파일 접근 시 <code>allow</code>, <code>ask</code>, <code>deny</code> 3단계 정책 적용.</li>
                    <li><strong>Path Traversal 차단:</strong> 상대 경로(<code>../../</code>)를 통한 OS 시스템 파일(Windows System32, /etc 등) 접근 원천 차단.</li>
                </ul>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">2. 네트워크 및 브라우저 도메인 제어</span>
                    <span class="badge badge-cyan">Network Policy</span>
                </div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    에이전트의 외부 웹 탐색 및 API 요청에 대한 통신 채널을 제한한다.
                </p>
                <ul style="font-size: 6.8pt; color: #475569;">
                    <li><strong>Internet Access Policy:</strong> 전역 허용(<code>allow</code>), 질의(<code>ask</code>), 완전 차단(<code>deny</code>).</li>
                    <li><strong>Browser Domain Allowlist:</strong> 브라우징 도구가 접속 가능한 호스트를 화이트리스트로 제한하여 정보 유출 방지.</li>
                </ul>
            </div>
        </div>

        <h2>3. 전역 설정(Global)과 프로젝트별(Project-Level) 오버라이드 계층</h2>
        <div class="arch-diagram">
            <div style="font-size: 7.0pt; line-height: 1.4; color: #1e293b;">
                <strong>[Global Settings] (<code>~/.gemini/config/</code>)</strong><br>
                &bull; Active Model Selection (Gemini 3.8 Flash/Pro)<br>
                &bull; Default Tool Execution Policy: <code>request-review</code><br>
                &bull; Global Command Denylist: <code>rm -rf /</code>, <code>format</code>, <code>reg delete</code><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert; [Overrides when project is active]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                <strong>[Project-Level Settings] (Workspace <code>.agents/</code> or <code>.antigravity/</code>)</strong><br>
                &bull; Tool Execution Policy: <code>always-proceed</code> (신뢰된 FC 언리얼 엔진 워크스페이스 내에서 파이프라인 자동화 허용)<br>
                &bull; Sandbox Mode: Disabled for local UBT build acceleration<br>
                &bull; Non-Workspace File Access: <code>ask</code> (엔진 설치 폴더 <code>Engine/Source</code> 읽기 허용)
            </div>
        </div>

        <div class="callout callout-warn">
            <div class="callout-title">
                <span>[보안 거버넌스 원칙] Command Denylist와 최소 권한 원칙</span>
                <span class="badge badge-warn">Least Privilege</span>
            </div>
            프로젝트 설정에서 <code>always-proceed</code>를 채택하더라도, 전역 설정의 Command Denylist(치명적 삭제 명령, 디스크 포맷, 레지스트리 변이 등)는 
            프로젝트 레벨에서 결코 오버라이드할 수 없도록 설계되어 개발자의 로컬 머신 안정성을 무조건 보장한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 1 &bull; Page 6 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 7: 런타임 아키텍처 요약 및 실무 체크리스트 (Synthesis & Best Practices) -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 1</span>
            <span class="tag">SYNTHESIS & BEST PRACTICES</span>
        </div>

        <h2>1. 안티그래비티 런타임 엔드투엔드(End-to-End) 생명주기 종합</h2>
        <p>
            개발자의 프롬프트 입력부터 결과물 생성 및 세션 종료에 이르는 런타임 파이프라인의 전체 상태 전이 흐름도이다.
        </p>

        <div class="arch-diagram">
            <div style="font-size: 6.9pt; font-family: monospace; line-height: 1.45; color: #0f172a;">
                [User Prompt] &rarr; [Progressive Context Assembly (Rules/Skills/Metadata)]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Gemini 3.8 CoT Reasoning (Streaming Thoughts)]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;+-----------------------+-----------------------+<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                [Direct Tool Dispatch]&nbsp;&nbsp;&nbsp;&nbsp;[Spawn Subagent]&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Schedule / Background Task]<br>
                (edit_file / run_command)&nbsp;(research / self)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;(Long compile / Daemon)<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;+-----------------------+-----------------------+<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Reactive Wakeup & Context Update (No Polling)]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Final Artifact Generation / User Response]
            </div>
        </div>

        <h2>2. 개발자를 위한 런타임 핵심 운영 수칙 5선 (Operational Checklist)</h2>
        <div class="grid-2">
            <div class="card" style="border-left: 3px solid #4338ca;">
                <div class="card-title">1. Reactive Wakeup 원칙 엄수</div>
                <p style="font-size: 6.9pt; color: #475569; margin: 2px 0;">
                    명령어나 서브에이전트의 종료를 확인하기 위해 반복적인 <code>sleep</code>이나 상태 체크 루프를 절대 작성하지 않는다. 
                    작업을 비동기로 시작하고 즉시 턴을 종료하면 시스템이 알아서 기상시킨다.
                </p>
            </div>
            <div class="card" style="border-left: 3px solid #0284c7;">
                <div class="card-title">2. Research 서브에이전트 적극 활용</div>
                <p style="font-size: 6.9pt; color: #475569; margin: 2px 0;">
                    방대한 레포지토리 전수 조사나 외부 기술 문서 탐색 시 부모 에이전트의 메인 대화창에 모든 파일 내용을 적재하지 말고, 
                    읽기 전용 <code>research</code> 에이전트에게 조사를 위임하여 요약본만 수취한다.
                </p>
            </div>
            <div class="card" style="border-left: 3px solid #16a34a;">
                <div class="card-title">3. 파괴적 실험 시 Branch 모드 지정</div>
                <p style="font-size: 6.9pt; color: #475569; margin: 2px 0;">
                    코드베이스 전체에 영향을 미치는 실험적 리팩토링이나 빌드 테스트는 <code>Workspace="branch"</code> 또는 <code>"share"</code> 모드로 
                    서브에이전트를 구동하여 메인 작업 트리의 오염을 원천 방지한다.
                </p>
            </div>
            <div class="card" style="border-left: 3px solid #ea580c;">
                <div class="card-title">4. 장기 태스크의 명시적 데몬화</div>
                <p style="font-size: 6.9pt; color: #475569; margin: 2px 0;">
                    종료되지 않는 라이브 서버나 감시 프로세스는 <code>IsDaemon: true</code>를 설정하여 스폰함으로써 
                    에이전트가 무의미하게 타임아웃을 기다리다 실패 처리되는 현상을 방지한다.
                </p>
            </div>
        </div>

        <h2>3. 제1권 결론 및 제2권 연계 안내</h2>
        <p>
            본 제1권에서는 안티그래비티 플랫폼의 구조적 토대인 4대 서피스, 비동기 추론 루프, 서브에이전트 IPC, 스케줄링 및 보안 거버넌스를 살펴보았다.
            이어지는 <strong>[제2권: 커스터마이징 아키텍처 & 확장성 프레임워크]</strong>에서는 
            에이전트를 도메인 전문가로 진화시키는 5대 기둥(Rules, Skills, Plugins, Hooks, MCP)과 
            점진적 공개(Progressive Disclosure), 아티팩트 시스템 및 트랜스크립트 상태 보존 메커니즘을 상세히 분석한다.
        </p>

        <div class="callout callout-cyan" style="margin-top: 4px;">
            <div class="callout-title">
                <span>[Next Volume] 제2권 핵심 예고</span>
                <span class="badge badge-cyan">Volume 2 Preview</span>
            </div>
            Rules & Skills 아키텍처, <code>hooks.json</code> 이벤트 가로채기, <code>mcp_config.json</code>을 통한 언리얼 엔진 5.8 실시간 연동 프로토콜, 
            Generative UI 및 Markdown 아티팩트 엔진 설계 표준 수록.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 1 &bull; Page 7 of 7</span>
    </div>
</div>

</body>
</html>
"""

def main():
    print("[VOL 1] Building temporary HTML...")
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    html_content = build_vol1_html()
    with open(TEMP_HTML, "w", encoding="utf-8") as f:
        f.write(html_content)
    print(f"[VOL 1] Saved temp HTML: {TEMP_HTML}")

    # Browser discovery
    candidates = [
        r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe",
        r"C:\Program Files\Google\Chrome\Application\chrome.exe",
    ]
    browser = None
    for c in candidates:
        if os.path.exists(c):
            browser = c
            break

    if not browser:
        print("[ERROR] Neither Edge nor Chrome found.")
        sys.exit(1)

    print(f"[VOL 1] Using browser: {browser}")
    cmd = [
        browser,
        "--headless",
        "--disable-gpu",
        "--no-sandbox",
        "--run-all-compositor-stages-before-draw",
        "--no-pdf-header-footer",
        f"--print-to-pdf={OUTPUT_PDF}",
        str(TEMP_HTML)
    ]

    print(f"[VOL 1] Rendering PDF: {OUTPUT_PDF}...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[ERROR] Browser failed with code {res.returncode}:\n{res.stderr}")
        sys.exit(1)

    if OUTPUT_PDF.exists():
        size_kb = round(OUTPUT_PDF.stat().st_size / 1024, 1)
        print(f"[SUCCESS] Volume 1 PDF generated: {OUTPUT_PDF} ({size_kb} KB)")
        if TEMP_HTML.exists():
            TEMP_HTML.unlink()
    else:
        print("[ERROR] PDF generation failed, file not found.")
        sys.exit(1)

if __name__ == "__main__":
    main()
