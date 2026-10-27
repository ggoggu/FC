#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates an executive PDF report:
"FC 프로젝트 독립 서브에이전트 기반 적대적 코드 심사 및 5대 최적화 검증 체계 구현 보고서"
Stored in Report/Architecture/FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf
"""

import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
OUTPUT_PDF = PROJECT_ROOT / "Report" / "Architecture" / "FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf"
TEMP_HTML = PROJECT_ROOT / "Report" / "Architecture" / "FC_Adversarial_Subagent_Code_Audit_Architecture_Report.html"

HTML_CONTENT = """<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<title>FC 프로젝트 독립 서브에이전트 기반 적대적 코드 심사 및 5대 최적화 검증 체계 구현 보고서</title>
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
        line-height: 1.5;
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
        font-size: 17pt;
        font-weight: 800;
        color: #0f172a;
        margin: 0 0 4px 0;
        letter-spacing: -0.5px;
        line-height: 1.25;
    }
    .subtitle {
        font-size: 9pt;
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
        font-size: 11pt;
        font-weight: 800;
        color: #0f172a;
        border-left: 4px solid #0284c7;
        padding-left: 8px;
        margin: 12px 0 8px 0;
        page-break-after: avoid;
    }
    h3 {
        font-size: 9pt;
        font-weight: 700;
        color: #1e293b;
        margin: 8px 0 4px 0;
        page-break-after: avoid;
    }
    p {
        margin: 0 0 6px 0;
        text-align: justify;
    }
    ul, ol {
        margin: 0 0 8px 0;
        padding-left: 16px;
    }
    li {
        margin-bottom: 3px;
    }
    .callout {
        background: #f0fdf4;
        border-left: 3.5px solid #16a34a;
        padding: 8px 10px;
        border-radius: 0 4px 4px 0;
        margin: 8px 0;
        font-size: 8.4pt;
    }
    .callout-blue {
        background: #f0f9ff;
        border-left: 3.5px solid #0284c7;
        padding: 8px 10px;
        border-radius: 0 4px 4px 0;
        margin: 8px 0;
        font-size: 8.4pt;
    }
    table {
        width: 100%;
        border-collapse: collapse;
        font-size: 8pt;
        margin: 8px 0;
    }
    th, td {
        border: 1px solid #cbd5e1;
        padding: 5px 8px;
        text-align: left;
    }
    th {
        background: #f1f5f9;
        font-weight: 700;
        color: #0f172a;
    }
    pre, code {
        font-family: 'Consolas', 'Courier New', monospace;
        font-size: 7.8pt;
    }
    pre {
        background: #0f172a;
        color: #f8fafc;
        padding: 8px 10px;
        border-radius: 4px;
        overflow-x: auto;
        margin: 6px 0;
        line-height: 1.35;
    }
    .diagram-box {
        background: #f8fafc;
        border: 1px solid #cbd5e1;
        border-radius: 6px;
        padding: 10px;
        margin: 8px 0;
        text-align: center;
        font-family: 'Consolas', monospace;
        font-size: 7.6pt;
        white-space: pre;
        line-height: 1.25;
        color: #1e293b;
    }
    .badge-blocking {
        background: #fee2e2;
        color: #b91c1c;
        font-weight: 700;
        padding: 1px 5px;
        border-radius: 3px;
    }
    .badge-warning {
        background: #fef3c7;
        color: #b45309;
        font-weight: 700;
        padding: 1px 5px;
        border-radius: 3px;
    }
    .badge-suggestion {
        background: #e0f2fe;
        color: #0369a1;
        font-weight: 700;
        padding: 1px 5px;
        border-radius: 3px;
    }
</style>
</head>
<body>

<!-- PAGE 1 -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC PROJECT ARCHITECTURE REPORT</span>
            <span class="tag">AI MULTI-MODEL AUDITOR HARNESS</span>
        </div>

        <div class="badge-top">Architecture & Verification Standard</div>
        <h1>독립 서브에이전트 기반 적대적 코드 심사<br>및 5대 최적화 검증 체계 구현 보고서</h1>
        <div class="subtitle">Lauren Tan(pstack) 멀티모델 검증 패러다임의 UE5.8 데디케이트 서버 복제 아키텍처 특화 구현</div>

        <div class="meta-box">
            <div class="meta-item"><strong>프로젝트</strong> FC (Unreal Engine 5.8)</div>
            <div class="meta-item"><strong>환경</strong> Google Antigravity</div>
            <div class="meta-item"><strong>구현 모듈</strong> /interrogate & adversarial-auditor</div>
            <div class="meta-item"><strong>일자</strong> 2026. 10. 28</div>
        </div>

        <div class="callout">
            <strong>Executive Summary</strong><br>
            본 문서는 Lauren Tan(SpaceX/Cursor)의 <code>pstack</code> 멀티모델 교차 검증 패러다임(<code>/interrogate</code>, <code>/arena</code>)을 분석하고, 이를 FC 프로젝트의 Unreal Engine 5.8 데디케이트 서버 아키텍처에 맞추어 구현한 <strong>적대적 코드 심사관(Adversarial Code Auditor)</strong> 시스템을 기술합니다. 단일 생성 모델의 인지적 맹점(Blind Spot), 오버엔지니어링, 복제 누락, 불필요한 틱 활성화를 원천 차단하기 위해, 읽기 전용(Read-Only) 서브에이전트가 <strong>5대 핵심 차원(간결성, 기존 구조 활용, 범위 내 적정 확장성, 네트워크 최적화, 일반 최적화)</strong>을 기준으로 코드를 엄격히 심사합니다.
        </div>

        <h2>1. 배경 및 도입 목적</h2>
        <h3>1.1 단일 생성 모델(Single Agent)의 확증 편향 극복</h3>
        <p>
            단일 AI 모델이 코드를 작성하는 경우, 본인이 작성한 컨텍스트 내에서 확증 편향이 발생하여 소유권 없는 액터에서의 Server RPC 선언, 불필요한 레이어 래핑, GC 추적 누락과 같은 미묘한 버그를 스스로 인지하지 못하는 한계가 있습니다.
        </p>

        <h3>1.2 언리얼 엔진 5.8 멀티플레이어 환경의 특수성</h3>
        <p>
            C++ 컴파일러나 일반 린터는 문법적 정합성만 검사할 뿐, <strong>서버 권한 분리(HasAuthority), RPC Connection 소유권, 복제 조건(DOREPLIFETIME_CONDITION), 가비지 컬렉션(TObjectPtr) 수명 주기</strong>와 같은 도메인 특화 결함을 사전에 탐지하지 못합니다. 따라서 커밋 전 엄격한 루브릭을 갖춘 독립 심사 서브에이전트의 교차 검증이 필수적입니다.
        </p>

        <h2>2. 독립 서브에이전트 아키텍처 다이어그램</h2>
        <div class="diagram-box">
+-----------------------------------------------------------------------------------+
|                           MAIN AGENT (Lead Developer)                             |
|  - 기능 구현, C++20 리팩토링, UBT 빌드 수행 (수정 및 실행 도구 보유)              |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 1. Git Diff & 의도(Stated Intent) 전달
                                         |    (invoke_subagent / /interrogate)
                                         v
+-----------------------------------------------------------------------------------+
|                      ADVERSARIAL AUDITOR (Read-Only Subagent)                     |
|  - 독립 시스템 프롬프트: 엄격한 심사관 역할 수행                                 |
|  - 수정 권한 원천 차단: EnableWriteTools = false (코드 오염 방지)                 |
|  - 5대 핵심 심사 차원(Rules 07 & AGENTS.md) 기반 비판적 채점                     |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 2. 구조화된 심사 보고서 (Findings Matrix)
                                         |    (APPROVED vs CHANGES REQUESTED)
                                         v
+-----------------------------------------------------------------------------------+
|                                TRIAGE & REFINEMENT                                |
|  - BLOCKING 치명적 네트워크/아키텍처 결함 우선 해결                               |
|  - python Scripts/pipeline.py static / build 자동 컴파일 검증                     |
+-----------------------------------------------------------------------------------+</div>
    </div>
    <div class="footer-bar">
        <span>FC Project Engineering &amp; Verification Protocol</span>
        <span>Page 1 of 2</span>
    </div>
</div>

<!-- PAGE 2 -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC PROJECT ARCHITECTURE REPORT</span>
            <span class="tag">5 CORE AUDIT DIMENSIONS &amp; PROTOCOL</span>
        </div>

        <h2>3. 5대 핵심 심사 차원 (The 5 Core Audit Dimensions)</h2>
        <table>
            <thead>
                <tr>
                    <th style="width: 22%;">심사 차원</th>
                    <th style="width: 40%;">핵심 점검 기준</th>
                    <th style="width: 38%;">차단되는 안티패턴</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>1. 코드 간결성<br>(Conciseness)</strong></td>
                    <td>최소 표면 수정(Minimal Surface Area), 명료한 C++20 관용구 사용, 불필요한 보일러플레이트 배제</td>
                    <td>1회성 로직을 위한 무분별한 래퍼 함수, 과도한 인터페이스 다형성 남발</td>
                </tr>
                <tr>
                    <td><strong>2. 기존 구조 활용<br>(Maximal Reuse)</strong></td>
                    <td>기존 Subsystem, CombatStatics, DataTable, Component 재사용, SSOT 데이터 자산 준수</td>
                    <td>기존 파이프라인 무시 후 병렬 클래스 생성, loose *.csv 파일 런타임 직접 I/O</td>
                </tr>
                <tr>
                    <td><strong>3. 범위 내 확장성<br>(Scoped Extensibility)</strong></td>
                    <td><strong>주어진 기획 스펙 범위</strong>에 최적화된 유연성, YAGNI 원칙 엄수, 데이터 기반 잎새 확장 보장</td>
                    <td>기획에 없는 먼 미래를 가정한 과설계(Over-engineering), 과도한 추상 팩토리 계층</td>
                </tr>
                <tr>
                    <td><strong>4. 네트워크 최적화<br>(Network &amp; Replication)</strong></td>
                    <td>HasAuthority() 상태 변조 가드, Connection 소유 액터의 Server RPC, DOREPLIFETIME_CONDITION, RepNotify 우선</td>
                    <td>소유권 없는 월드 액터의 Server RPC, 틱 단위 고빈도 RPC 전송, 조건 없는 무분별한 복제</td>
                </tr>
                <tr>
                    <td><strong>5. 일반 성능 최적화<br>(General Optimization)</strong></td>
                    <td>PrimaryActorTick.bCanEverTick = false 기본화, 핫패스 힙 할당 제로, TObjectPtr 및 TWeakObjectPtr GC 위생</td>
                    <td>Tick() 내 TArray::Add 또는 문자열 포맷팅, UObject 스마트 포인터 순환 참조 릭</td>
                </tr>
            </tbody>
        </table>

        <h2>4. 워크플로우 및 프로토콜 규격 (/interrogate)</h2>
        <div class="callout-blue">
            <strong>실무 호출 가이드</strong>: 대화창에 <code>/interrogate</code> 또는 <code>"코드 검사해줘"</code>, <code>"최적화 검사해줘"</code> 입력 시, 메인 에이전트가 <code>git diff</code>를 취합하여 <code>adversarial-auditor</code> 서브에이전트를 호출합니다.
        </div>

        <h3>4.1 심사관 서브에이전트 반환 리포트 규격 예시</h3>
        <pre># 🛡️ Adversarial Audit Report
Verdict: [CHANGES REQUESTED (BLOCKING)]

### Findings Matrix
| Severity | Dimension | Location | Issue & Root Cause | Actionable Fix |
| :--- | :--- | :--- | :--- | :--- |
| <span class="badge-blocking">BLOCKING</span> | Network | Source/FC/...:42 | Server RPC on unowned World Actor | APlayerController로 RPC 이전 |
| <span class="badge-warning">WARNING</span> | Reuse | Source/FC/...:88 | Re-implemented math helper | FCCombatStatics 유틸리티 재사용 |
| <span class="badge-suggestion">SUGGESTION</span>| Conciseness | Source/FC/...:120 | Redundant temporary wrapper | 인라인화 처리로 간결화 |

### Dimension Scoring
- Conciseness: [9/10]  |  Architecture Reuse: [7/10]  |  Scoped Extensibility: [8/10]
- Network Optimization: [6/10]  |  General Optimization: [9/10]</pre>

        <h2>5. 기대 효과 및 결론</h2>
        <ul>
            <li><strong>무결점 멀티플레이어 보장</strong>: 런타임에서 패킷 드롭이나 Desync를 유발하는 RPC 소유권 위반과 권한 체크 누락을 빌드 전 사전 박멸.</li>
            <li><strong>아키텍처 부채 제로화</strong>: 주어진 기획 범위를 초과하는 과설계를 방지하고 기존 서브시스템을 최우선 재사용하여 코드 비대화 차단.</li>
            <li><strong>안전한 자율 에이전트 루프 완성</strong>: 작업 에이전트와 심사 에이전트 간의 견제와 균형을 통해 상용 수준의 C++20 엔지니어링 퀄리티 달성.</li>
        </ul>
    </div>
    <div class="footer-bar">
        <span>FC Project Engineering &amp; Verification Protocol</span>
        <span>Page 2 of 2</span>
    </div>
</div>

</body>
</html>
"""

def main():
    print("Writing HTML...")
    OUTPUT_PDF.parent.mkdir(parents=True, exist_ok=True)
    with open(TEMP_HTML, "w", encoding="utf-8") as f:
        f.write(HTML_CONTENT)

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
