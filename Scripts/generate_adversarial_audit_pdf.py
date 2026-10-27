#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates the updated executive PDF report:
"FC 프로젝트 독립 서브에이전트 기반 적대적 코드 심사 및 5대 최적화 검증 체계 구현 보고서 (최종 완성판)"
Stored in Report/Architecture/FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf
"""

import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
OUTPUT_PDF = PROJECT_ROOT / "Report" / "Architecture" / "FC_Adversarial_Subagent_Code_Audit_Architecture_Report.pdf"
TEMP_HTML = PROJECT_ROOT / "Intermediate" / "temp_adversarial_audit_report.html"

HTML_CONTENT = """<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<title>독립 서브에이전트 기반 적대적 코드 심사 및 5대 최적화 검증 체계 최종 구현 보고서</title>
<style>
    @page {
        size: A4;
        margin: 11mm 13mm 11mm 13mm;
    }
    *, *:before, *:after {
        box-sizing: border-box;
    }
    body {
        font-family: 'Pretendard', -apple-system, BlinkMacSystemFont, 'Malgun Gothic', '맑은 고딕', 'Segoe UI', Roboto, Helvetica, Arial, sans-serif;
        color: #1e293b;
        background: #ffffff;
        line-height: 1.48;
        font-size: 8.5pt;
        margin: 0;
        padding: 0;
    }
    .page-container {
        page-break-after: always;
        height: 100%;
        display: flex;
        flex-direction: column;
        justify-content: space-between;
        min-height: 270mm;
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
        margin-bottom: 10px;
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
        margin-top: 8px;
        display: flex;
        justify-content: space-between;
        font-size: 7.2pt;
        color: #94a3b8;
    }
    .badge-top {
        display: inline-block;
        background: #0284c7;
        color: #ffffff;
        font-size: 7pt;
        font-weight: 700;
        letter-spacing: 0.8px;
        padding: 2.5px 7px;
        border-radius: 3px;
        text-transform: uppercase;
        margin-bottom: 5px;
    }
    h1 {
        font-size: 16pt;
        font-weight: 800;
        color: #0f172a;
        margin: 0 0 3px 0;
        letter-spacing: -0.5px;
        line-height: 1.25;
    }
    .subtitle {
        font-size: 8.8pt;
        color: #64748b;
        margin: 0 0 9px 0;
    }
    .meta-box {
        display: flex;
        background: #f8fafc;
        border: 1px solid #e2e8f0;
        border-radius: 5px;
        padding: 5px 12px;
        font-size: 7.6pt;
        color: #475569;
        justify-content: space-between;
        margin-bottom: 10px;
    }
    .meta-item strong {
        color: #0f172a;
        display: block;
        font-size: 6.8pt;
        text-transform: uppercase;
        margin-bottom: 1px;
    }
    h2 {
        font-size: 10.5pt;
        font-weight: 800;
        color: #0f172a;
        border-left: 3.5px solid #0284c7;
        padding-left: 7px;
        margin: 10px 0 6px 0;
        page-break-after: avoid;
    }
    h3 {
        font-size: 8.8pt;
        font-weight: 700;
        color: #1e293b;
        margin: 7px 0 3px 0;
        page-break-after: avoid;
    }
    p {
        margin: 0 0 5px 0;
        text-align: justify;
    }
    ul, ol {
        margin: 0 0 6px 0;
        padding-left: 16px;
    }
    li {
        margin-bottom: 2.5px;
    }
    .callout {
        background: #f0fdf4;
        border-left: 3.5px solid #16a34a;
        padding: 7px 10px;
        border-radius: 0 4px 4px 0;
        margin: 6px 0;
        font-size: 8.2pt;
    }
    .callout-blue {
        background: #f0f9ff;
        border-left: 3.5px solid #0284c7;
        padding: 7px 10px;
        border-radius: 0 4px 4px 0;
        margin: 6px 0;
        font-size: 8.2pt;
    }
    table {
        width: 100%;
        border-collapse: collapse;
        font-size: 7.8pt;
        margin: 6px 0;
    }
    th, td {
        border: 1px solid #cbd5e1;
        padding: 4.5px 7px;
        text-align: left;
    }
    th {
        background: #f1f5f9;
        font-weight: 700;
        color: #0f172a;
    }
    pre, code {
        font-family: 'Consolas', 'Courier New', monospace;
        font-size: 7.5pt;
    }
    pre {
        background: #0f172a;
        color: #f8fafc;
        padding: 7px 9px;
        border-radius: 4px;
        overflow-x: auto;
        margin: 5px 0;
        line-height: 1.35;
    }
    .diagram-box {
        background: #f8fafc;
        border: 1px solid #cbd5e1;
        border-radius: 5px;
        padding: 8px 10px;
        margin: 6px 0;
        font-family: 'Consolas', monospace;
        font-size: 7.2pt;
        white-space: pre;
        line-height: 1.25;
        color: #1e293b;
    }
    .badge-pass { background: #dcfce7; color: #166534; font-weight: 700; padding: 1px 5px; border-radius: 3px; }
    .badge-block { background: #fee2e2; color: #991b1b; font-weight: 700; padding: 1px 5px; border-radius: 3px; }
    .badge-warn { background: #fef3c7; color: #92400e; font-weight: 700; padding: 1px 5px; border-radius: 3px; }
</style>
</head>
<body>

<!-- PAGE 1 -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC PROJECT ARCHITECTURE REPORT</span>
            <span class="tag">AI MULTI-MODEL AUDITOR (PRODUCTION EDITION)</span>
        </div>

        <div class="badge-top">Architecture &amp; Verification Standard</div>
        <h1>독립 서브에이전트 기반 적대적 코드 심사<br>및 5대 최적화 검증 체계 구현 보고서</h1>
        <div class="subtitle">Lauren Tan(pstack) 멀티모델 검증 패러다임 이식 및 실전 자율 치유 폐루프(10/10 LGTM) 완성</div>

        <div class="meta-box">
            <div class="meta-item"><strong>프로젝트</strong> FC (Unreal Engine 5.8)</div>
            <div class="meta-item"><strong>환경</strong> Google Antigravity</div>
            <div class="meta-item"><strong>완성 모듈</strong> /interrogate, pipeline.py audit, adversarial-auditor</div>
            <div class="meta-item"><strong>최종 갱신</strong> 2026. 10. 28 (Production Verified)</div>
        </div>

        <div class="callout">
            <strong>Executive Summary</strong><br>
            본 문서는 Lauren Tan(pstack)의 멀티모델 교차 검증 패러다임을 FC 프로젝트의 UE 5.8 데디케이트 서버 복제 아키텍처에 특화하여 완성한 <strong>적대적 코드 심사관(Adversarial Code Auditor)</strong> 시스템을 기술합니다. 단일 생성 모델의 인지적 맹점(Blind Spot), 오버엔지니어링, 복제 누락을 막기 위해 Read-Only 서브에이전트가 <strong>5대 핵심 차원(간결성, 기존 구조 재사용, 범위 내 확장성, 네트워크 최적화, 일반 최적화)</strong>을 심사하며, <strong><code>pipeline.py audit</code> 모드</strong>와 <strong>자율 치유(Closed-Loop Remediation) 루프</strong>를 통해 10/10 LGTM 무결점 품질을 실증 달성했습니다.
        </div>

        <h2>1. 시스템 아키텍처 및 자율 치유 폐루프 (Closed-Loop)</h2>
        <div class="diagram-box">
+-----------------------------------------------------------------------------------+
|                           MAIN AGENT (Lead Developer)                             |
|  - 기능 구현, C++20 리팩토링, 전처리: git add -N . & C++ diff 필터링              |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 1. Git Diff & Stated Intent 전달 (/interrogate)
                                         v
+-----------------------------------------------------------------------------------+
|                      ADVERSARIAL AUDITOR (Read-Only Subagent)                     |
|  - 모델 모드: inherit 기본 ('자동 티어링' 요청 시에만 diff 기반 자동 선정)                    |
|  - 수정 권한 완전 격리 (EnableWriteTools: false) 및 5대 심사 루브릭 평가          |
+----------------------------------------+------------------------------------------+
                                         |
                                         | 2. 구조화된 판정 리포트 (Findings Matrix)
                                         v
+-----------------------------------------------------------------------------------+
|                       CLOSED-LOOP AUTONOMOUS REMEDIATION                          |
|  - BLOCKING 결함(HasAuthority, BlueprintSetter, COND_InitialOnly) 즉시 자동 패치  |
|  - 컴파일 검증: python Scripts/pipeline.py static & build (0 error, 0 warning)    |
|  - 서브에이전트 재심사 통과 -> 최종 10/10 APPROVED (LGTM) 획득 후 안전 커밋      |
+-----------------------------------------------------------------------------------+</div>

        <h2>2. 5대 핵심 심사 차원 (The 5 Core Audit Dimensions)</h2>
        <table>
            <thead>
                <tr>
                    <th style="width: 20%;">심사 차원</th>
                    <th style="width: 42%;">핵심 점검 기준</th>
                    <th style="width: 38%;">차단되는 안티패턴</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><strong>1. 코드 간결성<br>(Conciseness)</strong></td>
                    <td>최소 표면 수정(Minimal diff), 명료한 C++20 관용구, 헤더 기본값 활용</td>
                    <td>생성자 내 중복 초기화, 1회성 로직의 무분별한 래퍼 함수 남발</td>
                </tr>
                <tr>
                    <td><strong>2. 기존 구조 활용<br>(Maximal Reuse)</strong></td>
                    <td>기존 Subsystem, CombatStatics, DataTable, Component 재사용</td>
                    <td>기존 패턴 무시 후 중복 구현, loose *.csv 런타임 파일 직접 I/O</td>
                </tr>
                <tr>
                    <td><strong>3. 범위 내 확장성<br>(Scoped Extensibility)</strong></td>
                    <td>기획 스펙 범위에 최적화된 유연성, YAGNI 원칙 엄수, Extent 보존</td>
                    <td>스펙을 벗어난 과도한 추상 팩토리 계층, 하드코딩된 컴포넌트 크기</td>
                </tr>
                <tr>
                    <td><strong>4. 네트워크 최적화<br>(Network &amp; Repl)</strong></td>
                    <td>HasAuthority() 가드, BlueprintSetter 캡슐화 강제, COND_InitialOnly 대역폭 절약</td>
                    <td>소유권 없는 액터의 Server RPC, 클라이언트 세터 변조, 틱 단위 복제 낭비</td>
                </tr>
                <tr>
                    <td><strong>5. 일반 성능 최적화<br>(General Optimization)</strong></td>
                    <td>bCanEverTick = false, FMath::IsNearlyEqual 조기 반환, TObjectPtr GC 위생</td>
                    <td>동일 값 변조 시 불필요한 물리 Overlap 연산, UObject 순환 참조 메모리 누수</td>
                </tr>
            </tbody>
        </table>
    </div>
    <div class="footer-bar">
        <span>FC Project Engineering &amp; Verification Protocol</span>
        <span>Page 1 of 2 • System Architecture</span>
    </div>
</div>

<!-- PAGE 2 -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC PROJECT ARCHITECTURE REPORT</span>
            <span class="tag">BATTLE-TESTED EVIDENCE &amp; PLAYBOOK</span>
        </div>

        <h2>3. 실전 검증 사례: FCDoorBase 결함 탐지 및 10/10 완치</h2>
        <p>실제 프로젝트 코드인 <code>Source/FC/Private/Map/FCDoorBase.cpp</code> 변경 사항을 대상으로 실전 심사 및 자율 치유 폐루프를 구동한 실증 데이터입니다:</p>

        <table>
            <thead>
                <tr>
                    <th style="width: 14%;">상태</th>
                    <th style="width: 16%;">영역</th>
                    <th style="width: 40%;">발견된 결함 및 원인</th>
                    <th style="width: 30%;">적용된 자율 치유 조치</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><span class="badge-pass">RESOLVED</span></td>
                    <td>Network</td>
                    <td><code>SetDoorWidth</code>에 서버 권한 가드가 없어 클라이언트 임의 변조 가능</td>
                    <td><code>if (!HasAuthority()) return;</code> 가드 추가</td>
                </tr>
                <tr>
                    <td><span class="badge-pass">RESOLVED</span></td>
                    <td>Network</td>
                    <td><code>BlueprintReadWrite</code>로 노출되어 블루프린트에서 세터 및 서버 연산 우회</td>
                    <td><code>BlueprintSetter = SetDoorWidth</code> 캡슐화 강제</td>
                </tr>
                <tr>
                    <td><span class="badge-pass">RESOLVED</span></td>
                    <td>Network</td>
                    <td>정적 던전 문에 <code>COND_None</code>을 사용하여 매 틱 더티 체크 대역폭 낭비</td>
                    <td><code>COND_InitialOnly</code> 복제 최적화 적용</td>
                </tr>
                <tr>
                    <td><span class="badge-pass">RESOLVED</span></td>
                    <td>Extensibility</td>
                    <td>하드코딩된 X, Z 박스 범위로 디자이너 커스텀 트리거 설정 파괴</td>
                    <td><code>GetUnscaledBoxExtent()</code>로 기존 X, Z 보존</td>
                </tr>
                <tr>
                    <td><span class="badge-pass">RESOLVED</span></td>
                    <td>General</td>
                    <td>동일 값 입력 시에도 매번 브로드페이즈 물리 Overlap 갱신 발생</td>
                    <td><code>FMath::IsNearlyEqual</code> 조기 반환 추가</td>
                </tr>
            </tbody>
        </table>

        <div class="callout">
            <strong>최종 재심사 채점표 (Approved Verdict)</strong><br>
            • Conciseness: <strong>10/10</strong> &nbsp;|&nbsp; • Architecture Reuse: <strong>10/10</strong> &nbsp;|&nbsp; • Scoped Extensibility: <strong>10/10</strong><br>
            • Network Optimization: <strong>10/10</strong> &nbsp;|&nbsp; • General Optimization: <strong>10/10</strong><br>
            • <strong>UBT 빌드 검증</strong>: <code>python Scripts/pipeline.py build</code> (0 errors, 0 warnings in 31.38s)
        </div>

        <h2>4. 환경 점검을 통해 고도화된 3대 엔지니어링 도구</h2>
        <ul>
            <li><strong>CLI 파이프라인 모드 신설 (<code>pipeline.py audit</code>)</strong>:
                <code>python Scripts/pipeline.py audit</code> 명령어로 C++ 변경 파일 수, diff 라인 수, 정적 복제/대역폭 검사 결과를 일괄 패키징하여 출력.
            </li>
            <li><strong>바이너리 에셋 오염 차단</strong>:
                <code>git add -N .</code>을 통해 신규 Untracked C++ 파일을 diff에 자동 편입하고, <code>-- "*.h" "*.cpp" "*.cs"</code> 필터를 강제하여 바이너리(.uasset) 컨텍스트 낭비 원천 차단.
            </li>
            <li><strong>온디맨드 모델 티어링 (On-Demand Tiering)</strong>:
                모델명을 일일이 지정할 필요 없이, 사용자가 <strong>'자동 티어링 적용해줘'</strong>와 같이 요청한 경우에만 diff 규모(&lt;150줄 <code>flash</code>, 복제/GAS <code>pro</code>)를 분석하여 최적 모델을 자동 분기. 기본 검사 시에는 항상 <code>inherit</code>(메인 상속) 유지.
            </li>
        </ul>

        <h2>5. 결론 및 실무 운영 가이드</h2>
        <div class="callout-blue">
            <strong>개발자 실무 워크플로우</strong>:
            C++ 코드 수정 완료 → 대화창에 <code>/interrogate</code> 또는 <code>"코드 검사해줘"</code> 입력 → 서브에이전트 심사 리포트 수신 → 메인 에이전트가 BLOCKING 항목 자율 치유 → <code>python Scripts/pipeline.py full --auto-commit</code>으로 무결점 안전 커밋 완료.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project Engineering &amp; Verification Protocol</span>
        <span>Page 2 of 2 • Battle-Tested Evidence &amp; Playbook</span>
    </div>
</div>

</body>
</html>
"""

def main():
    print("Writing HTML...")
    TEMP_HTML.parent.mkdir(parents=True, exist_ok=True)
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
