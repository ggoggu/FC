#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates Volume 2 of FC Antigravity Environment Architecture Report:
"FC 프로젝트 안티그래비티 환경 종합 아키텍처 보고서 [제2권: 커스터마이징 & 확장성 프레임워크]"
Target: Report/Architecture/FC_Antigravity_Vol2_Customization_and_Extensibility.pdf
"""

import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
OUTPUT_DIR = PROJECT_ROOT / "Report" / "Architecture"
OUTPUT_PDF = OUTPUT_DIR / "FC_Antigravity_Vol2_Customization_and_Extensibility.pdf"
TEMP_HTML = PROJECT_ROOT / "Scripts" / "temp_antigravity_vol2.html"

def build_vol2_html() -> str:
    return """<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<title>안티그래비티 환경 종합 아키텍처 보고서 [제2권: 커스터마이징 & 확장성 프레임워크]</title>
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
        border-bottom: 1.5px solid #059669;
        padding-bottom: 3px;
        margin-bottom: 9px;
        font-size: 7.1pt;
        font-weight: 700;
        color: #64748b;
        letter-spacing: 0.5px;
    }
    .header-bar .tag {
        color: #059669;
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
        background: #059669;
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
        border-left: 3.5px solid #059669;
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
    .badge-primary { background: #d1fae5; color: #059669; }
    .badge-cyan { background: #e0f2fe; color: #0284c7; }
    .badge-indigo { background: #e0e7ff; color: #4338ca; }
    .badge-danger { background: #fee2e2; color: #991b1b; }
    .badge-warn { background: #fef3c7; color: #92400e; }
    .badge-slate { background: #f1f5f9; color: #475569; }

    .callout {
        background: #f8fafc;
        border-left: 3.5px solid #059669;
        padding: 5px 9px;
        margin: 4px 0 6px 0;
        border-radius: 0 4px 4px 0;
        font-size: 7.6pt;
    }
    .callout-title {
        font-weight: 700;
        color: #059669;
        margin-bottom: 2px;
        display: flex;
        align-items: center;
        justify-content: space-between;
    }
    .callout-cyan {
        background: #f0f9ff;
        border-left-color: #0284c7;
    }
    .callout-cyan .callout-title {
        color: #0369a1;
    }
    .callout-warn {
        background: #fffbeb;
        border-left-color: #f59e0b;
    }
    .callout-warn .callout-title {
        color: #b45309;
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
    .grid-5 {
        display: grid;
        grid-template-columns: 1fr 1fr 1fr 1fr 1fr;
        gap: 4px;
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
<!-- PAGE 1: 표지 및 커스터마이징 5대 기둥 (Overview & 5 Pillars)           -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 2</span>
            <span class="tag">CUSTOMIZATION & EXTENSIBILITY</span>
        </div>

        <div style="margin-bottom: 7px;">
            <span class="badge-top">Antigravity Extensibility Architecture</span>
            <h1>안티그래비티 환경 종합 아키텍처 보고서</h1>
            <div class="subtitle">[제2권] 커스터마이징 5대 기둥, 점진적 공개, MCP 표준 프로토콜 및 아티팩트 엔진</div>
        </div>

        <div class="meta-box">
            <div class="meta-item">
                <strong>Specification</strong>
                AGY Customization Spec 2.0
            </div>
            <div class="meta-item">
                <strong>Extension Pillars</strong>
                Rules / Skills / Plugins / Hooks / MCP
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

        <h2>1. 개요 및 커스터마이징 철학 (Customization Philosophy)</h2>
        <p>
            안티그래비티의 핵심 설계 목표는 범용 대규모 언어 모델(LLM)을 
            <strong>프로젝트 고유의 아키텍처와 컨벤션에 완전히 동기화된 도메인 특화 전문가(Domain-Specific Expert)</strong>로 전환하는 것이다. 
            이를 위해 시스템은 단순 프롬프트 주입 방식을 탈피하여 
            <strong>규칙(Rules), 기술(Skills), 플러그인(Plugins), 훅(Hooks), 모델 컨텍스트 프로토콜(MCP)</strong>의 
            <strong>5대 확장 기둥(The 5 Pillars)</strong>을 제공한다.
        </p>

        <div class="grid-5">
            <div class="card" style="border-top: 2.5px solid #059669;">
                <div class="card-title">1. Rules</div>
                <p style="font-size: 6.8pt; margin: 2px 0 0 0; color: #475569;">
                    불변 아키텍처 원칙, 코딩 표준, API 제약 조건의 계층적 강제 (AGENTS.md, GEMINI.md).
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #0284c7;">
                <div class="card-title">2. Skills</div>
                <p style="font-size: 6.8pt; margin: 2px 0 0 0; color: #475569;">
                    다단계 절차, 런북, 스크립트 실행 워크플로를 온디맨드로 캡슐화한 절차적 지식 단위.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #7c3aed;">
                <div class="card-title">3. Plugins</div>
                <p style="font-size: 6.8pt; margin: 2px 0 0 0; color: #475569;">
                    연관된 스킬, 규칙, MCP 구성을 단일 네임스페이스 단위로 번들링하여 배포/관리.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #ea580c;">
                <div class="card-title">4. Hooks</div>
                <p style="font-size: 6.8pt; margin: 2px 0 0 0; color: #475569;">
                    에이전트 수명주기 이벤트(도구 호출 전/후 등)를 가로채어 정책 검증 및 보안 감사 자동화.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #e11d48;">
                <div class="card-title">5. MCP Servers</div>
                <p style="font-size: 6.8pt; margin: 2px 0 0 0; color: #475569;">
                    외부 도구, 데이터베이스, 언리얼 엔진 런타임을 표준 JSON-RPC로 결합하는 오픈 프로토콜.
                </p>
            </div>
        </div>

        <h2>2. 커스터마이징 생태계 통합 토폴로지 (Ecosystem Architecture)</h2>
        <div class="arch-diagram">
            <div style="text-align: center; font-weight: 700; font-size: 7.4pt; color: #064e3b; margin-bottom: 4px;">
                [Antigravity 5-Pillar Customization & Context Ingestion Pipeline]
            </div>
            <div style="display: flex; gap: 5px; justify-content: space-between; margin-bottom: 5px;">
                <div style="flex: 1; background: #ecfdf5; border: 1px solid #a7f3d0; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong>[Rules]</strong><br>Hierarchical & Path-Walk
                </div>
                <div style="flex: 1; background: #f0f9ff; border: 1px solid #bae6fd; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong>[Skills]</strong><br>Progressive Disclosure
                </div>
                <div style="flex: 1; background: #faf5ff; border: 1px solid #e9d5ff; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong>[Plugins]</strong><br>Bundled Unit & Manifest
                </div>
                <div style="flex: 1; background: #fff7ed; border: 1px solid #ffedd5; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong>[Hooks]</strong><br>Lifecycle Interception
                </div>
                <div style="flex: 1; background: #fff1f2; border: 1px solid #fecdd3; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong>[MCP Servers]</strong><br>Live Engine RPC Bridge
                </div>
            </div>
            <div style="background: #064e3b; color: #ffffff; border-radius: 3px; padding: 4px 6px; text-align: center; margin-bottom: 5px; font-size: 7.2pt;">
                &darr; <strong>Discovery Engine & Deduplication Resolver</strong> (Precedence Order Evaluation) &darr;
            </div>
            <div style="display: flex; gap: 5px; justify-content: space-between;">
                <div style="flex: 1; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong style="color: #059669;">Context Window Injection</strong><br>
                    선별적 프롬프트 주입 (토큰 최적화)
                </div>
                <div style="flex: 1; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong style="color: #0284c7;">Artifact Engine</strong><br>
                    구조화된 다이어그램 / 마크다운 문서 생성
                </div>
                <div style="flex: 1; background: #ffffff; border: 1px solid #cbd5e1; border-radius: 3px; padding: 4px; font-size: 6.8pt;">
                    <strong style="color: #7c3aed;">Session Transcripts</strong><br>
                    JSONL 기반 상태 추적 및 /compact 압축
                </div>
            </div>
        </div>

        <h2>3. 5대 기둥의 핵심 특성 비교 매트릭스</h2>
        <table>
            <tr>
                <th style="width: 14%;">기둥 (Pillar)</th>
                <th style="width: 22%;">구성 파일 / 폴더</th>
                <th style="width: 18%;">적용 스코프</th>
                <th style="width: 30%;">주요 역할 및 강제 사항</th>
                <th style="width: 16%;">로드 타이밍</th>
            </tr>
            <tr>
                <td><strong>Rules</strong></td>
                <td><code>AGENTS.md</code>, <code>GEMINI.md</code></td>
                <td>프로젝트 전역 / 디렉터리</td>
                <td>아키텍처 불변 원칙, 코딩 표준, 금지 패턴 강제</td>
                <td>상시 또는 모델 판단</td>
            </tr>
            <tr>
                <td><strong>Skills</strong></td>
                <td><code>skills/&lt;name&gt;/SKILL.md</code></td>
                <td>온디맨드 (Progressive)</td>
                <td>복잡한 다단계 워크플로, 빌드/테스트 스크립트 실행 절차</td>
                <td>동적 점진적 로드</td>
            </tr>
            <tr>
                <td><strong>Plugins</strong></td>
                <td><code>plugins/&lt;name&gt;/plugin.json</code></td>
                <td>네임스페이스 번들</td>
                <td>팀 단위 스킬셋, 룰, MCP 도구 구성을 단일 패키지로 제공</td>
                <td>설치/활성화 시</td>
            </tr>
            <tr>
                <td><strong>Hooks</strong></td>
                <td><code>hooks.json</code></td>
                <td>수명주기 이벤트</td>
                <td>도구 실행 전/후 린터 검사, 보안 차단, 감사 로그 기록</td>
                <td>이벤트 발생 시점</td>
            </tr>
            <tr>
                <td><strong>MCP Servers</strong></td>
                <td><code>mcp_config.json</code></td>
                <td>도구/리소스 통합</td>
                <td>언리얼 라이브 에디터, DB, 원격 API와의 양방향 JSON-RPC 통신</td>
                <td>도구 디스패치 시</td>
            </tr>
        </table>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 2 &bull; Page 1 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 2: 탐색 계층 & 우선순위 모델 (Discovery Hierarchy & Precedence)   -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 2</span>
            <span class="tag">DISCOVERY & PRECEDENCE</span>
        </div>

        <h2>1. 탐색 우선순위 (Loading Priority & Precedence Order)</h2>
        <p>
            안티그래비티는 여러 위치에서 동일한 이름의 스킬, 규칙, 도구가 발견될 때 명확한 우선순위 규칙에 따라 충돌을 해결한다.
            <strong>더 구체적인 프로젝트 레벨 구성이 전역 머신 구성을 항상 오버라이드</strong>한다.
        </p>

        <div class="grid-2">
            <div>
                <h3>5단계 우선순위 계층도 (Highest &rarr; Lowest)</h3>
                <ol style="font-size: 7.1pt; color: #1e293b;">
                    <li>
                        <strong style="color: #059669;">1. Workspace Project (최우선)</strong><br>
                        작업 디렉터리(CWD)부터 Git 루트까지 상향 탐색하여 발견된 <code>.agents/</code>, <code>.antigravity/</code>, <code>AGENTS.md</code>.
                    </li>
                    <li>
                        <strong style="color: #0284c7;">2. Declared Configurations</strong><br>
                        워크스페이스 내부 <code>skills.json</code> 또는 <code>plugins.json</code>에 명시적으로 등록된 외부 경로 커스터마이징.
                    </li>
                    <li>
                        <strong style="color: #4338ca;">3. Global Discovery</strong><br>
                        개발자 로컬 머신의 사용자 홈 디렉터리 <code>~/.gemini/config/</code> 하위의 프로젝트 비종속 설정.
                    </li>
                    <li>
                        <strong style="color: #64748b;">4. Built-in Customizations</strong><br>
                        안티그래비티 애플리케이션 바이너리에 기본 탑재된 빌트인 스킬 및 시스템 프롬프트.
                    </li>
                    <li>
                        <strong style="color: #94a3b8;">5. Global Declared Configurations (최하위)</strong><br>
                        머신 전역 JSON 설정에 등록된 외부 참조 커스터마이징.
                    </li>
                </ol>
            </div>
            <div>
                <h3>탐색 및 해결 메커니즘 다이어그램</h3>
                <div class="code-box">
<span class="cm"># Customization Resolution Hierarchy</span>
[Local CWD: FC/Source/FC/Combat]
  |-- Walk-up &rarr; FC/ (Found .antigravity/rules, AGENTS.md) [Priority 1]
  |-- Walk-up &rarr; Git Root (.git) &rarr; Stop traversal
  v
[Workspace Manifests: skills.json]                         [Priority 2]
  v
[Global Dir: ~/.gemini/config/]                            [Priority 3]
  v
[Built-in Runtime Defaults]                                 [Priority 4]

<span class="kw">Outcome:</span> Project rules (<span class="str">AGENTS.md</span>) always override global defaults!
                </div>
            </div>
        </div>

        <h2>2. 점진적 공개 메커니즘 (Progressive Disclosure)</h2>
        <div class="callout callout-success">
            <div class="callout-title">
                <span>[컨텍스트 최적화 핵심] Progressive Disclosure 원리</span>
                <span class="badge badge-success">Context Preservation</span>
            </div>
            수십 개의 스킬과 수백 줄의 룰셋을 세션 시작 시 한꺼번에 컨텍스트 윈도우에 주입하면, 
            모델의 유효 토큰이 고갈되고 환각(Hallucination) 위험이 극대화된다. 
            안티그래비티는 <strong>메타데이터 요약본만 초기 주입하고, 실제 필요 시에만 본문을 주입</strong>하는 
            점진적 공개 아키텍처를 채택한다.
        </div>

        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">1단계: 경량 메타데이터 주입 (Lightweight Catalog)</span>
                    <span class="badge badge-primary">Initial Turn</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    세션 초기화 시 각 스킬의 <code>name</code>과 <code>description</code>(1~2줄 요약)만을 시스템 프롬프트의 카탈로그 영역에 등록한다. 
                    토큰 소모량은 스킬당 수십 토큰 미만으로 극소화된다.
                </p>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">2단계: 온디맨드 심층 로드 (On-Demand Deep Load)</span>
                    <span class="badge badge-cyan">Dynamic Activation</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    사용자의 요청이 해당 스킬의 설명과 일치하거나 에이전트가 절차적 지식을 필요로 할 때, 
                    에이전트 런타임이 <code>SKILL.md</code> 및 참조 문서(<code>references/</code>) 전문을 해당 턴의 컨텍스트로 지연 로드(Lazy Load)한다.
                </p>
            </div>
        </div>

        <h2>3. 경로 기반 중복 제거 (Path-Based Deduplication)</h2>
        <p>
            여러 규칙 디렉터리나 트리거 조건이 동일한 파일 경로를 가리키더라도, 런타임의 <strong>Deduplication Engine</strong>이 
            절대 정규화 경로(Canonical Resolved Path)를 기준으로 중복을 필터링한다. 
            단일 대화 턴 내에서 동일한 룰셋이나 스킬이 중복 인젝션되는 현상은 시스템 레벨에서 원천적으로 방지된다.
        </p>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 2 &bull; Page 2 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 3: 규칙(Rules) & 절차적 스킬(Skills) 아키텍처 (Rules & Skills)     -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 2</span>
            <span class="tag">RULES & SKILLS ARCHITECTURE</span>
        </div>

        <h2>1. 규칙(Rules) 아키텍처 & 디렉터리 계층화</h2>
        <p>
            규칙(Rules)은 에이전트가 코드를 작성하거나 수정할 때 반드시 준수해야 하는 <strong>불변의 제약 조건(Constraints)과 컨벤션</strong>을 정의한다.
        </p>

        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">1. 전역/프로젝트 최상위 규칙 (AGENTS.md)</span>
                    <span class="badge badge-primary">Supreme Directives</span>
                </div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    프로젝트 루트의 <code>AGENTS.md</code>는 모든 하위 작업에 상시 적용되는 최고 권위 지침서이다. 
                    네트워크 4계층 권한 분리, Server Authority 준수, Cooked Asset SSOT, 최소 수정 원칙을 정의한다.
                </p>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">2. 모듈러 도메인 룰셋 (.antigravity/rules/*.md)</span>
                    <span class="badge badge-cyan">Domain-Specific</span>
                </div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    C++20 코딩 표준, 멀티플레이어 컨벤션, 루프 엔지니어링, GAS 속성, 인벤토리 네트워킹, MVVM 등 
                    특정 도메인 영역 작업 시에만 선별적으로 활성화되는 전문 규칙 모듈이다.
                </p>
            </div>
        </div>

        <h2>2. 규칙 파일 포맷 및 트리거 메타데이터</h2>
        <div class="code-box">
<span class="cm">---</span>
<span class="kw">trigger:</span> always_on  <span class="cm"># or trigger: model_decision (온디맨드 점진적 로드)</span>
<span class="kw">description:</span> <span class="str">"언리얼 엔진 5.8 C++20 코딩 표준 및 메모리 관리 지침"</span>
<span class="kw">paths:</span>
  - <span class="str">"Source/**/*.h"</span>
  - <span class="str">"Source/**/*.cpp"</span>
<span class="cm">---</span>
# Rule Content (Markdown)
- TObjectPtr&lt;T&gt; 사용 의무화 (원시 포인터 UPROPERTY 금지)
- IWYU (Include-What-You-Use) 준수 및 전방 선언(Forward Declaration) 적극 활용
        </div>

        <h2>3. 기술(Skills) 구조 및 다단계 절차 캡슐화</h2>
        <p>
            스킬(Skills)은 복잡한 다단계 워크플로, 디버깅 절차, 도구 실행 체인을 캡슐화하여 에이전트에게 
            <strong>'방법(How-To)'</strong>을 교육하는 실행형 지식 모듈이다.
        </p>

        <div class="grid-2">
            <div>
                <h3>스킬 디렉터리 표준 구조</h3>
                <div class="code-box" style="font-size: 6.6pt;">
skills/&lt;skill-name&gt;/
  |-- <span class="fn">SKILL.md</span>         <span class="cm"># 메인 지침 및 YAML 프론트매터 (필수)</span>
  |-- <span class="ty">scripts/</span>         <span class="cm"># 자동화 보조 파이썬/배치 스크립트</span>
  |-- <span class="ty">references/</span>      <span class="cm"># 오프라인 참조 하위 문서 (Markdown)</span>
  |-- <span class="ty">examples/</span>        <span class="cm"># 실제 코드 예제 및 입출력 템플릿</span>
  +-- <span class="ty">resources/</span>       <span class="cm"># 에셋 템플릿, 사전 컴파일 바이너리</span>
                </div>
            </div>
            <div>
                <h3><code>SKILL.md</code> 프론트매터 표준</h3>
                <div class="code-box" style="font-size: 6.6pt;">
<span class="cm">---</span>
<span class="kw">name:</span> ue5-gas-generator
<span class="kw">description:</span> >-
  <span class="str">언리얼 엔진 5.8 GameplayAbility, AttributeSet,</span>
  <span class="str">GameplayEffect 보일러플레이트를 생성하고 복제 검증을 수행.</span>
<span class="cm">---</span>
# Multi-Step Procedure:
1. Define FGameplayAttributeData in Header
2. Implement OnRep functions with RepNotify
3. Run python Scripts/pipeline.py static for verification
                </div>
            </div>
        </div>

        <div class="callout callout-cyan">
            <div class="callout-title">
                <span>[규칙과 스킬의 상호보완적 결합] The Rules + Skills Synergy</span>
                <span class="badge badge-cyan">Architecture Cohesion</span>
            </div>
            <strong>규칙(Rules)</strong>이 "서버 전용 함수는 반드시 HasAuthority()로 감싸야 한다"는 <strong>불변의 원칙(What/Constraint)</strong>을 정의한다면, 
            <strong>스킬(Skills)</strong>은 "전투 능력 구현 시 AttributeSet 생성부터 OnRep 바인딩 및 파이프라인 검증까지 단계별로 어떻게 수행하는가"의 
            <strong>절차적 실행(How-To/Workflow)</strong>을 이끈다. 이 둘의 결합을 통해 에이전트는 결함 없는 고품질 코드를 자율 작성한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 2 &bull; Page 3 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 4: 플러그인(Plugins) & 라이프사이클 훅(Hooks) (Plugins & Hooks)   -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 2</span>
            <span class="tag">PLUGINS & HOOKS ENGINE</span>
        </div>

        <h2>1. 플러그인(Plugins) 번들링 아키텍처</h2>
        <p>
            플러그인(Plugins)은 연관된 스킬, 규칙, MCP 서버 설정 및 라이프사이클 훅을 
            <strong>단일 배포 단위로 캡슐화한 네임스페이스 모듈</strong>이다. 
            팀 단위로 표준화된 도구셋을 형상 관리(VCS)하고 프로젝트 간에 손쉽게 공유할 수 있도록 지원한다.
        </p>

        <div class="grid-2">
            <div>
                <h3>플러그인 매니페스트 (<code>plugin.json</code>) 구조</h3>
                <div class="code-box" style="font-size: 6.6pt;">
{
  <span class="str">"name"</span>: <span class="str">"fc-unreal-toolkit"</span>,
  <span class="str">"version"</span>: <span class="str">"2.1.0"</span>,
  <span class="str">"description"</span>: <span class="str">"FC UE5.8 게임플레이 및 파이프라인 전용 플러그인"</span>,
  <span class="str">"skills"</span>: [<span class="str">"skills/ue5-build"</span>, <span class="str">"skills/asset-inspect"</span>],
  <span class="str">"rules"</span>: [<span class="str">"rules/01-ue58-coding-standards.md"</span>],
  <span class="str">"mcp_servers"</span>: <span class="str">"mcp_config.json"</span>,
  <span class="str">"hooks"</span>: <span class="str">"hooks.json"</span>
}
                </div>
            </div>
            <div>
                <h3>플러그인 관리 및 슬래시 커맨드</h3>
                <ul style="font-size: 7.0pt; color: #334155;">
                    <li><strong><code>/plugin install &lt;path&gt;</code>:</strong> 로컬 또는 원격 리포지토리의 플러그인을 워크스페이스에 즉시 설치.</li>
                    <li><strong><code>/plugin enable/disable</code>:</strong> 현재 세션에서 특정 플러그인의 스킬/룰셋을 동적으로 활성화/비활성화.</li>
                    <li><strong>네임스페이스 충돌 격리:</strong> 각 플러그인은 고유 네임스페이스를 부여받아 다른 플러그인의 동일 이름 스킬과 상호 충돌하지 않음.</li>
                </ul>
            </div>
        </div>

        <h2>2. 라이프사이클 훅(Hooks) 엔진</h2>
        <p>
            훅(Hooks)은 에이전트의 런타임 수명주기 이벤트 시점에 외부 스크립트나 명령어를 가로채어 실행(Interception)하는 거버넌스 자동화 계층이다.
        </p>

        <div class="grid-4">
            <div class="card" style="border-top: 2.5px solid #059669;">
                <div class="card-title"><code>pre_tool_call</code></div>
                <p style="font-size: 6.7pt; color: #475569; margin: 2px 0 0 0;">
                    도구가 실행되기 직전 호출. 파괴적 명령어 사전 차단, 보안 정책 검증, 린터 사전 점검.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #0284c7;">
                <div class="card-title"><code>post_tool_call</code></div>
                <p style="font-size: 6.7pt; color: #475569; margin: 2px 0 0 0;">
                    도구 실행 직후 호출. 표준 출력 로그 수집, 파일 변경 이력 Git 자동 스테이징.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #7c3aed;">
                <div class="card-title"><code>pre_message</code></div>
                <p style="font-size: 6.7pt; color: #475569; margin: 2px 0 0 0;">
                    사용자 입력 처리 직전 호출. 민감 정보(API 키 등) 마스킹 및 추가 컨텍스트 주입.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #ea580c;">
                <div class="card-title"><code>session_start</code></div>
                <p style="font-size: 6.7pt; color: #475569; margin: 2px 0 0 0;">
                    세션 초기화 시점 호출. 환경 변수 검증, 빌드 도구 바이너리 설치 상태 점검.
                </p>
            </div>
        </div>

        <h2>3. 훅 인터셉션 흐름도 및 <code>hooks.json</code> 설정 스펙</h2>
        <div class="arch-diagram">
            <div style="font-size: 6.9pt; font-family: monospace; line-height: 1.4; color: #0f172a;">
                [Agent Emits Tool Call: run_command("rm -rf Content/")]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>[Hook Engine: pre_tool_call &rarr; Scripts/audit_guard.py]</strong><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;+-----------------------------------+-----------------------------------+<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;&nbsp;Exit Code == 0 (Valid)&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;&nbsp;Exit Code != 0 (Policy Violation)<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                [Proceed with Tool Execution]&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>[ABORT Tool Call]</strong><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;Inject Error: "Destructive command blocked by Policy Hook."
            </div>
        </div>

        <div class="callout callout-warn">
            <div class="callout-title">
                <span>[보안 거버넌스] 무결성 보장을 위한 훅 Fail-Closed 원칙</span>
                <span class="badge badge-warn">Fail-Closed Security</span>
            </div>
            사전 검증 훅(<code>pre_tool_call</code>) 스크립트 실행 중 파이썬 예외가 발생하거나 타임아웃이 초과되면, 
            시스템은 안전을 위해 해당 도구의 실행을 <strong>기본 차단(Fail-Closed)</strong>한다. 
            이를 통해 예기치 않은 스크립트 버그로 인해 비검증 위험 명령어가 시스템에 전파되는 사고를 원천 방지한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 2 &bull; Page 4 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 5: Model Context Protocol (MCP) 표준 아키텍처 (MCP Protocol)    -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 2</span>
            <span class="tag">MODEL CONTEXT PROTOCOL (MCP)</span>
        </div>

        <h2>1. Model Context Protocol (MCP) 개요 및 표준화</h2>
        <p>
            <strong>Model Context Protocol (MCP)</strong>은 AI 에이전트와 외부 시스템(개발 도구, 데이터베이스, 게임 엔진) 간의 상호작용을 
            표준화하기 위해 설계된 <strong>개방형 프로토콜(Open Standard)</strong>이다. 
            에이전트가 각 도구의 내부 구현을 몰라도 일관된 JSON-RPC 규격을 통해 기능과 데이터를 탐색하고 안전하게 호출할 수 있다.
        </p>

        <div class="grid-3">
            <div class="card" style="border-top: 3px solid #059669;">
                <div class="card-title">1. Tools (실행 함수)</div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    에이전트가 매개변수를 전달하여 환경을 변이시키거나 작업을 수행하도록 노출된 실행형 RPC 함수 (예: 언리얼 액터 스폰).
                </p>
            </div>
            <div class="card" style="border-top: 3px solid #0284c7;">
                <div class="card-title">2. Resources (데이터 피드)</div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    에이전트가 읽을 수 있는 파일, 로그 스트림, DB 테이블 스키마 등 실시간 정적/동적 데이터 소스 (URI 기반 라우팅).
                </p>
            </div>
            <div class="card" style="border-top: 3px solid #7c3aed;">
                <div class="card-title">3. Prompts (프롬프트 템플릿)</div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    서버가 클라이언트에게 제공하는 구조화된 사전 정의 프롬프트 템플릿 및 인터랙티브 워크플로 매크로.
                </p>
            </div>
        </div>

        <h2>2. MCP 전송 계층 (Transport Layers) 및 설정 표준</h2>
        <div class="grid-2">
            <div>
                <h3>전송 계층 비교 (stdio vs sse)</h3>
                <ul style="font-size: 7.0pt; color: #334155;">
                    <li><strong><code>stdio</code> (표준 입출력):</strong> 로컬 머신에서 서브프로세스를 스폰하고 파이프(stdin/stdout)를 통해 초고속 JSON 통신 수행. 로컬 도구에 최적.</li>
                    <li><strong><code>sse</code> (Server-Sent Events):</strong> HTTP 기반으로 원격 도커 컨테이너, 클라우드 인프라 또는 분산 서비스와 이벤트 스트림 통신.</li>
                </ul>
            </div>
            <div>
                <h3><code>mcp_config.json</code> 설정 스키마</h3>
                <div class="code-box" style="font-size: 6.5pt;">
{
  <span class="str">"mcpServers"</span>: {
    <span class="str">"unreal-live-bridge"</span>: {
      <span class="str">"command"</span>: <span class="str">"python"</span>,
      <span class="str">"args"</span>: [<span class="str">"Scripts/mcp_unreal_bridge.py"</span>],
      <span class="str">"env"</span>: { <span class="str">"UE_PORT"</span>: <span class="str">"30010"</span> }
    }
  }
}
                </div>
            </div>
        </div>

        <h2>3. FC 언리얼 엔진 5.8 라이브 에디터 연동 아키텍처</h2>
        <p>
            안티그래비티는 MCP 브릿지를 통해 언리얼 에디터의 Live Remote Execution API와 연결되어 
            <strong>실시간 레벨 뷰포트 조작, 액터 배치, 에셋 검사</strong>를 자율 수행한다.
        </p>

        <div class="arch-diagram">
            <div style="font-size: 6.9pt; font-family: monospace; line-height: 1.4; color: #0f172a;">
                [Antigravity Agent] &mdash;&mdash;Call MCP Tool: <code>spawn_mob(Blueprint="BP_Goblin", Location=[0,0,100])</code>&mdash;&rarr;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v [stdio JSON-RPC]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>[MCP Server: Scripts/create_mcp_test_scene.py]</strong><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v [TCP/HTTP Remote Control]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>[Unreal Engine 5.8 Editor Live Runtime]</strong><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; Instantiates Actor in Current Level<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; Validates NavMesh & Component Attachments<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; Returns Result JSON with Actor GUID & Coordinates<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                [Agent Receives Success Payload] &larr;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;&mdash;+
            </div>
        </div>

        <div class="callout callout-cyan">
            <div class="callout-title">
                <span>[MCP의 가치] 에디터 재시작 없는 실시간 라이브 튜닝</span>
                <span class="badge badge-cyan">Zero-Restart Velocity</span>
            </div>
            전통적인 엔진 개발에서는 사소한 파라미터 변경이나 씬 구성을 위해 에디터를 재시작해야 했다. 
            안티그래비티 MCP 브릿지를 활용하면 에이전트가 실행 중인 에디터 내부의 블루프린트 변수, 카메라 앵글, 스포너 배치를 
            원격으로 직접 쿼리하고 변경하므로 개발 속도가 비약적으로 향상된다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 2 &bull; Page 5 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 6: 아티팩트 시스템 & 인터랙티브 UI (Artifacts & Generative UI)    -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 2</span>
            <span class="tag">ARTIFACTS & GENERATIVE UI</span>
        </div>

        <h2>1. 아티팩트(Artifacts) 엔진 아키텍처</h2>
        <p>
            아티팩트(Artifacts)는 단순 대화형 말풍선 텍스트의 한계를 극복하고, 
            <strong>구조화된 보고서, 설계 다이어그램, 태스크 목록, 코드 Diff</strong>를 영속적으로 기록/렌더링하기 위한 특수 마크다운 문서 시스템이다.
        </p>

        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">1. 저장소 격리 및 메타데이터 바인딩</span>
                    <span class="badge badge-primary">Dedicated Storage</span>
                </div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    모든 아티팩트는 <code>&lt;appDataDir&gt;\brain\&lt;conversation-id&gt;</code> 전용 디렉터리에 독립 <code>.md</code> 파일로 저장된다. 
                    생성 시 <code>ArtifactMetadata(Summary, UserFacing, RequestFeedback)</code>가 필수 바인딩된다.
                </p>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">2. 대화 컨텍스트 비대화 방지</span>
                    <span class="badge badge-cyan">Context Decoupling</span>
                </div>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    아티팩트를 생성한 후 에이전트는 대화창에 아티팩트 전문을 중복 출력하지 않고 
                    링크만을 제시함으로써 컨텍스트 윈도우 토큰을 비약적으로 보존한다.
                </p>
            </div>
        </div>

        <h2>2. 아티팩트 지원 렌더링 확장 기능</h2>
        <table>
            <tr>
                <th style="width: 22%;">확장 기능</th>
                <th style="width: 38%;">문법 및 사용 형식</th>
                <th style="width: 40%;">시각화 효과 및 활용 사례</th>
            </tr>
            <tr>
                <td><strong>GitHub Alerts</strong></td>
                <td><code>&gt; [!NOTE]</code>, <code>&gt; [!TIP]</code>, <code>&gt; [!IMPORTANT]</code>, <code>&gt; [!WARNING]</code></td>
                <td>배경색과 전용 아이콘이 적용된 시각적 콜아웃 박스 (경고, 중요 사항 강조).</td>
            </tr>
            <tr>
                <td><strong>Mermaid Diagrams</strong></td>
                <td><code>```mermaid ... ```</code> (flowchart, sequenceDiagram, stateDiagram, erDiagram)</td>
                <td>인라인 렌더링되는 인터랙티브 벡터 흐름도 및 시퀀스 다이어그램.</td>
            </tr>
            <tr>
                <td><strong>Four-Backtick Carousels</strong></td>
                <td><code>````carousel ... &lt;!-- slide --&gt; ````</code></td>
                <td>비포/애프터 코드 비교, 단계별 튜토리얼을 슬라이드 형태로 순차 탐색.</td>
            </tr>
            <tr>
                <td><strong>KaTeX Math</strong></td>
                <td>인라인 <code>\(...\)</code>, <code>$...$</code> 또는 디스플레이 <code>$$...$$</code></td>
                <td>전투 대미지 공식, TTK 기대값 계산식 등 완벽한 수학 기호 렌더링.</td>
            </tr>
        </table>

        <h2>3. 인터랙티브 컴포넌트: <code>ask_question</code> & Generative UI</h2>
        <div class="grid-2">
            <div>
                <h3>대화형 질문 모달 (<code>ask_question</code>)</h3>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    요구사항이 모호하거나 아키텍처 선택지가 갈릴 때, 사용자가 즉시 선택할 수 있는 
                    <strong>인터랙티브 모달 UI</strong>를 렌더링하고 사용자 응답 시까지 안전하게 블로킹한다.
                </p>
                <div class="code-box" style="font-size: 6.5pt;">
ask_question(questions=[{
  <span class="str">"question"</span>: <span class="str">"인벤토리 네트워킹 방식을 선택하십시오."</span>,
  <span class="str">"options"</span>: [
    <span class="str">"(Recommended) FFastArraySerializer 기반 대역폭 최적화"</span>,
    <span class="str">"표준 TArray OnRep 단순 복제"</span>
  ],
  <span class="str">"is_multi_select"</span>: <span class="kw">false</span>
}])
                </div>
            </div>
            <div>
                <h3>Generative UI & 스크래치 패드</h3>
                <p style="font-size: 7.0pt; color: #334155; margin: 3px 0;">
                    단순 마크다운을 넘어 풍부한 HTML/CSS 인터랙티브 컨트롤, 실시간 파트 차트, 데이터 테이블을 인라인 또는 독립 패널에 직접 렌더링.
                </p>
                <ul style="font-size: 6.8pt; color: #475569;">
                    <li><strong>Scratch Directory:</strong> 디버깅용 임시 스크립트 및 테스트 데이터를 <code>&lt;conversation-id&gt;/scratch/</code>에 영속 저장.</li>
                    <li><strong>Interactive Approval:</strong> <code>RequestFeedback: true</code> 설정을 통해 사용자의 1-클릭 실행 승인(Proceed) 획득.</li>
                </ul>
            </div>
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 2 &bull; Page 6 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 7: 세션 트랜스크립트 & 상태 보존 (Transcripts & Memory Management) -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 2</span>
            <span class="tag">TRANSCRIPTS & MEMORY MANAGEMENT</span>
        </div>

        <h2>1. 세션 트랜스크립트(Transcripts) 이중화 구조</h2>
        <p>
            안티그래비티는 에이전트와 사용자의 모든 대화 단계, 사고 과정(Thinking), 도구 호출 이력을 
            <strong>JSON Lines (JSONL)</strong> 포맷으로 정밀하게 영속화한다.
        </p>

        <table>
            <tr>
                <th style="width: 25%;">로그 파일</th>
                <th style="width: 35%;">데이터 특성 및 구조</th>
                <th style="width: 40%;">설계 목적 및 최적화 기법</th>
            </tr>
            <tr>
                <td><strong><code>transcript.jsonl</code></strong><br>(Compact Log)</td>
                <td>대규모 내용(content, thinking, tool_calls)이 <code>truncated_fields</code>로 축약된 경량 스트림.</td>
                <td>고속 탐색, 서브에이전트 이력 스캔 및 대화 흐름 개요를 최소 I/O로 빠르게 조회.</td>
            </tr>
            <tr>
                <td><strong><code>transcript_full.jsonl</code></strong><br>(Full Log)</td>
                <td>모든 프롬프트, 도구 페이로드, 수십 KB의 표준 출력이 생략 없이 보존된 완전체 로그.</td>
                <td>축약된 필드의 원본 데이터 정밀 복원 및 세션 간 완전한 상태 리플레이 지원.</td>
            </tr>
        </table>

        <h2>2. 메모리 최적화 및 슬래시 커맨드 생태계</h2>
        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">컨텍스트 압축 (<code>/compact</code>)</span>
                    <span class="badge badge-primary">Context Compaction</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    대화가 장기화되어 컨텍스트 윈도우가 임계치에 도달할 때, <code>/compact</code> 커맨드를 실행하여 
                    과거의 사소한 도구 출력과 질의응답을 정밀한 상태 요약본으로 압축하고 토큰을 대폭 확보한다.
                </p>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">목표 지향 자율 실행 (<code>/goal</code>)</span>
                    <span class="badge badge-warn">Long-Running Autonomy</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    야간 대규모 리팩토링이나 풀 빌드 검증 시 에이전트가 중도에 멈추지 않고 
                    최종 목표가 100% 달성될 때까지 자율적으로 서브태스크를 연속 해결하도록 보장한다.
                </p>
            </div>
        </div>

        <h2>3. 제2권 종합 결론 및 제3권 연계 안내</h2>
        <p>
            본 제2권에서는 안티그래비티의 커스터마이징 5대 기둥(Rules, Skills, Plugins, Hooks, MCP)과 
            점진적 공개, 아티팩트 엔진 및 트랜스크립트 상태 관리 메커니즘을 상세히 규명하였다.
            이러한 확장성 프레임워크는 대규모 언리얼 엔진 프로젝트인 <strong>FC</strong>의 
            복합적인 멀티플레이어 아키텍처와 결합하여 비로소 최고의 시너지를 발휘한다.
        </p>

        <div class="callout callout-cyan" style="margin-top: 6px;">
            <div class="callout-title">
                <span>[Next Volume] 제3권: FC 프로젝트 언리얼 5.8 실전 엔지니어링 & 자율 루프</span>
                <span class="badge badge-cyan">Volume 3 Preview</span>
            </div>
            &bull; <strong>4계층 권한 분리 모델:</strong> Server Authority, Replicated State, Autonomous Client, Simulated UI 실전 구현.<br>
            &bull; <strong>모듈러 도메인 룰셋 01~06:</strong> C++20 코딩 표준, GAS AttributeSet, Fast Array 직렬화, UMG MVVM 연동 규격.<br>
            &bull; <strong>자율 파이프라인 러너:</strong> <code>pipeline.py</code> 7대 모드 및 UBT 컴파일 에러 5회 자율 셀프 힐링(Self-Healing) 루프 완벽 분석.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 2 &bull; Page 7 of 7</span>
    </div>
</div>

</body>
</html>
"""

def main():
    print("[VOL 2] Building temporary HTML...")
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    html_content = build_vol2_html()
    with open(TEMP_HTML, "w", encoding="utf-8") as f:
        f.write(html_content)
    print(f"[VOL 2] Saved temp HTML: {TEMP_HTML}")

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

    print(f"[VOL 2] Using browser: {browser}")
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

    print(f"[VOL 2] Rendering PDF: {OUTPUT_PDF}...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[ERROR] Browser failed with code {res.returncode}:\n{res.stderr}")
        sys.exit(1)

    if OUTPUT_PDF.exists():
        size_kb = round(OUTPUT_PDF.stat().st_size / 1024, 1)
        print(f"[SUCCESS] Volume 2 PDF generated: {OUTPUT_PDF} ({size_kb} KB)")
        if TEMP_HTML.exists():
            TEMP_HTML.unlink()
    else:
        print("[ERROR] PDF generation failed, file not found.")
        sys.exit(1)

if __name__ == "__main__":
    main()
