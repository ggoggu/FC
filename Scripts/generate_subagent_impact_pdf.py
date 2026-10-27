#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates an executive PDF report:
"FC 프로젝트 SubAgent 추가에 따른 Rule, AGENTS.md, Skill 시스템 연쇄 변경 및 통합 분석 보고서"
Stored strictly as PDF in Report/Architecture/FC_SubAgent_Addition_Rules_Skills_Impact_Report.pdf
"""

import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
OUTPUT_PDF = PROJECT_ROOT / "Report" / "Architecture" / "FC_SubAgent_Addition_Rules_Skills_Impact_Report.pdf"
TEMP_HTML = PROJECT_ROOT / "Intermediate" / "temp_subagent_impact_report.html"

HTML_CONTENT = """<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<title>SubAgent 추가에 따른 Rule, AGENTS.md, Skill 연쇄 변경 및 통합 분석 보고서</title>
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
        font-size: 8.6pt;
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
        font-size: 16.5pt;
        font-weight: 800;
        color: #0f172a;
        margin: 0 0 4px 0;
        letter-spacing: -0.5px;
        line-height: 1.25;
    }
    .subtitle {
        font-size: 8.8pt;
        color: #64748b;
        margin: 0 0 10px 0;
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
        margin: 7px 0;
        font-size: 8.2pt;
    }
    .callout-blue {
        background: #f0f9ff;
        border-left: 3.5px solid #0284c7;
        padding: 7px 10px;
        border-radius: 0 4px 4px 0;
        margin: 7px 0;
        font-size: 8.2pt;
    }
    .callout-amber {
        background: #fffbeb;
        border-left: 3.5px solid #f59e0b;
        padding: 7px 10px;
        border-radius: 0 4px 4px 0;
        margin: 7px 0;
        font-size: 8.2pt;
    }
    table {
        width: 100%;
        border-collapse: collapse;
        font-size: 7.9pt;
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
        font-size: 7.6pt;
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
        font-size: 7.3pt;
        white-space: pre;
        line-height: 1.25;
        color: #1e293b;
    }
    .badge {
        font-size: 6.8pt;
        font-weight: 700;
        padding: 1.5px 5px;
        border-radius: 3px;
        display: inline-block;
    }
    .badge-subagent { background: #e0e7ff; color: #4338ca; }
    .badge-rule     { background: #fef3c7; color: #92400e; }
    .badge-doc      { background: #dcfce7; color: #166534; }
    .badge-skill    { background: #fae8ff; color: #86198f; }
</style>
</head>
<body>

<!-- PAGE 1 -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC PROJECT AI ARCHITECTURE REPORT</span>
            <span class="tag">SUBAGENT SYSTEM IMPACT ANALYSIS</span>
        </div>

        <div class="badge-top">Autonomous Engineering Architecture</div>
        <h1>SubAgent 추가에 따른 시스템 연쇄 영향도 분석<br>(Rule, AGENTS.md, Skill 변경 및 통합)</h1>
        <div class="subtitle">독립 서브에이전트 도입 시 프로젝트 거버넌스, 지침, 룰, 실행 인터페이스의 상호작용 체계 분석</div>

        <div class="meta-box">
            <div class="meta-item"><strong>프로젝트</strong> FC (Unreal Engine 5.8)</div>
            <div class="meta-item"><strong>문서 성격</strong> 아키텍처 연쇄 수정 영향도 분석 보고서</div>
            <div class="meta-item"><strong>발행 형식</strong> Executive PDF Edition</div>
            <div class="meta-item"><strong>일자</strong> 2026. 10. 28</div>
        </div>

        <div class="callout">
            <strong>Executive Summary</strong><br>
            Antigravity 에이전트 환경에서 <strong>새로운 SubAgent(예: <code>adversarial-auditor</code>)</strong>를 추가하는 작업은 단순한 프롬프트 정의에 그치지 않습니다. 에이전트 간의 권한 분리, 엄격한 심사 루브릭, 자율 파이프라인 연계, 슬래시 명령어 트리거를 유기적으로 완성하기 위해 <strong>① Rule 계층</strong>, <strong>② 거버넌스 계층(AGENTS.md)</strong>, <strong>③ Skill 계층</strong>이 반드시 동기화되어 수정되어야 합니다. 본 보고서는 서브에이전트 도입 시 이 4대 계층이 어떻게 상호작용하고 변경되는지를 체계적으로 규명합니다.
        </div>

        <h2>1. SubAgent 추가 시 4계층 연쇄 변경 개요 (4-Tier Co-Evolution)</h2>
        <p>
            AI 하네스에서 서브에이전트가 단독으로 생성되면, 메인 에이전트는 해당 서브에이전트의 존재를 인지하거나 언제 호출해야 할지 알 수 없으며, 서브에이전트 또한 프로젝트 고유의 설계 원칙을 준수할 수 없습니다. 따라서 다음 4단계 연쇄 수정이 수반됩니다:
        </p>

        <div class="diagram-box">
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│ 1. SUBAGENT LAYER (런타임 정의)                                                              │
│    - define_subagent 로 'adversarial-auditor' 타입 등록                                     │
│    - Read-Only 권한 격리 (EnableWriteTools: false) 및 독립적 시스템 프롬프트 부여            │
└──────────────────────────────────────┬──────────────────────────────────────────────────────┘
                                       │ 심사 기준 및 도메인 지식 분리 (토큰 절약)
                                       v
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│ 2. RULE LAYER (.antigravity/rules/ 모듈화)                                                  │
│    - 07-adversarial-audit-standards.md 생성                                                 │
│    - 5대 핵심 심사 차원(간결성, 기존 구조 활용, 범위 내 확장성, 네트워크/일반 최적화) 표준화 │
└──────────────────────────────────────┬──────────────────────────────────────────────────────┘
                                       │ 최상위 프로젝트 규정 및 파이프라인 공식 편입
                                       v
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│ 3. GOVERNANCE & PIPELINE LAYER (AGENTS.md SSOT)                                             │
│    - 파이프라인 모드 표에 7번째 공식 모드(Subagent Audit: /interrogate) 등록                │
│    - 도메인 룰 목록(Modular Domain Rules)에 Rule 07 연계 명시 및 최소 수정 원칙 결합        │
└──────────────────────────────────────┬──────────────────────────────────────────────────────┘
                                       │ 사용자 및 메인 에이전트 실행 인터페이스화
                                       v
┌─────────────────────────────────────────────────────────────────────────────────────────────┐
│ 4. SKILL & TRIGGER LAYER (.agents/skills/interrogate/)                                      │
│    - SKILL.md 생성: Frontmatter 인텐트 등록 ('/interrogate', '코드 검사해줘')                │
│    - Git diff 추출 -> Stated Intent 작성 -> invoke_subagent 호출 -> Findings 취합 자동화   │
└─────────────────────────────────────────────────────────────────────────────────────────────┘</div>

        <h2>2. 각 계층별 세부 변경 내역 및 설계 의도</h2>
        <p>
            각 계층이 왜 수정되어야 하며, 어떤 엔지니어링 의도를 담고 있는지 상세 분석합니다.
        </p>

        <h3>2.1 SubAgent Layer (정의 및 격리)</h3>
        <ul>
            <li><strong>변경 내용</strong>: <code>define_subagent</code> API를 통해 <code>adversarial-auditor</code> 등록.</li>
            <li><strong>핵심 설계 의도 (Read-Only)</strong>: <code>EnableWriteTools: false</code>를 강제하여, 심사 서브에이전트가 코드를 직접 수정하는 과정에서 발생할 수 있는 환각(Hallucination) 및 2차 버그 유발을 원천 차단했습니다.</li>
            <li><strong>모델 다변화</strong>: <code>Model: pro</code>를 지원하여, 코드를 작성한 모델보다 더 깊은 추론력을 가진 상위 모델이 비판적 결함을 찾아내도록 구성.</li>
        </ul>
    </div>
    <div class="footer-bar">
        <span>FC Project Engineering &amp; Verification Protocol</span>
        <span>Page 1 of 3</span>
    </div>
</div>

<!-- PAGE 2 -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC PROJECT AI ARCHITECTURE REPORT</span>
            <span class="tag">DETAILED LAYER MODIFICATIONS</span>
        </div>

        <h3>2.2 Rule Layer: <code>.antigravity/rules/</code> 의 확장</h3>
        <ul>
            <li><strong>신규 추가 파일</strong>: <code>.antigravity/rules/07-adversarial-audit-standards.md</code></li>
            <li><strong>왜 서브에이전트 프롬프트에 전부 넣지 않고 Rule 파일로 분리하는가?</strong>
                <ul>
                    <li><strong>토큰 효율성 (Progressive Disclosure)</strong>: 심사 기준 전체를 시스템 프롬프트에 상시 유지하면 서브에이전트 구동 시마다 막대한 컨텍스트 토큰이 소모됩니다. 기준을 외부 룰 파일로 모듈화하면 필요할 때만 참조할 수 있습니다.</li>
                    <li><strong>일관성 및 단일 진실 공급원(SSOT)</strong>: 인간 개발자, 메인 에이전트, 심사 서브에이전트가 모두 <strong>동일한 5대 심사 기준 문서</strong>를 읽고 소통하므로 피드백의 왜곡이 발생하지 않습니다.</li>
                </ul>
            </li>
            <li><strong>5대 심사 차원의 확립</strong>:
                <ol>
                    <li><strong>코드 간결성</strong>: 불필요한 보일러플레이트 및 과도한 클래스 래핑 배제.</li>
                    <li><strong>기존 구조 및 코드 활용</strong>: 기존 Subsystem, DataTable, Component 재사용 강제.</li>
                    <li><strong>범위 내 적정 확장성</strong>: 기획서에 명시된 범위를 초과하는 미래 대비 과설계(YAGNI 위반) 억제.</li>
                    <li><strong>네트워크 최적화</strong>: HasAuthority, RPC Connection 소유권, 복제 조건(COND) 엄수.</li>
                    <li><strong>일반 성능 최적화</strong>: Tick 기본 비활성화, 핫패스 힙 할당 제로화, TObjectPtr GC 위생.</li>
                </ol>
            </li>
        </ul>

        <h3>2.3 Governance Layer: <code>AGENTS.md</code> 의 동기화</h3>
        <ul>
            <li><strong>수정 위치</strong>: <code>AGENTS.md</code> Section 3(파이프라인 실행 모드) 및 Section 4(도메인 룰 목록).</li>
            <li><strong>변경 내역</strong>:
                <pre>┌──────────────────────────────────────────────────────────────────────────────────────┐
│                              PIPELINE EXECUTION MODES                                │
├──────────────────────────────────────────────────────────────────────────────────────┤
│ ...                                                                                  │
│ 7. Adversarial Audit (/interrogate): Read-only subagent review on 5 core dimensions │
│ 8. Full Release (`full`)      : Sequential (Static -> Build -> Test -> [Auto-Commit])│
└──────────────────────────────────────────────────────────────────────────────────────┘</pre>
            </li>
            <li><strong>설계 의도</strong>:
                <ul>
                    <li><code>AGENTS.md</code>는 세션이 새로 열리거나 다른 개발자가 참여할 때 가장 먼저 주입되는 프로젝트 헌법입니다.</li>
                    <li>여기에 서브에이전트 검사 모드가 누락되면, 메인 에이전트는 코드를 작성한 후 서브에이전트에게 검사를 요청해야 한다는 사실 자체를 잊어버리게 됩니다. 따라서 프로토콜 가이드라인에 <strong>의무적 교차 검사 절차</strong>로 공식 편입했습니다.</li>
                </ul>
            </li>
        </ul>

        <h3>2.4 Skill Layer: <code>.agents/skills/interrogate/</code> 의 구축</h3>
        <ul>
            <li><strong>신규 추가 파일</strong>: <code>.agents/skills/interrogate/SKILL.md</code></li>
            <li><strong>역할 및 트리거링</strong>: 사용자가 <code>/interrogate</code> 또는 <code>"코드 검사해줘"</code>, <code>"서브에이전트 리뷰 돌려줘"</code>라고 말할 때 Antigravity 하네스가 해당 스킬을 즉시 감지하여 절차를 실행합니다.</li>
            <li><strong>자동화된 3단계 워크플로우</strong>:
                <ol>
                    <li><strong>컨텍스트 취합</strong>: <code>git diff HEAD</code>를 자동으로 수집하고 작성자의 구현 의도(Stated Intent)를 요약.</li>
                    <li><strong>서브에이전트 격리 실행</strong>: <code>invoke_subagent</code>를 통해 <code>adversarial-auditor</code>를 호출하여 심사 의뢰.</li>
                    <li><strong>판정 및 보완</strong>: 반환된 Findings Matrix(BLOCKING / WARNING / SUGGESTION)를 확인하여 치명적 결함을 우선 수정.</li>
                </ol>
            </li>
        </ul>

        <div class="callout-amber">
            <strong>핵심 결론</strong>: 서브에이전트는 독립된 스크립트가 아니라 <strong>[룰 - 거버넌스 - 스킬 - 런타임]</strong>의 4중 결합체로 구성될 때 비로소 사람의 개입 없이도 누락 없이 안정적으로 작동합니다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project Engineering &amp; Verification Protocol</span>
        <span>Page 2 of 3</span>
    </div>
</div>

<!-- PAGE 3 -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>FC PROJECT AI ARCHITECTURE REPORT</span>
            <span class="tag">IMPACT MATRIX &amp; LIFECYCLE GUIDE</span>
        </div>

        <h2>3. 서브에이전트 추가에 따른 종합 시스템 영향도 매트릭스</h2>
        <table>
            <thead>
                <tr>
                    <th style="width: 18%;">계층 / 파일</th>
                    <th style="width: 14%;">유형</th>
                    <th style="width: 38%;">주요 변경 및 추가 내용</th>
                    <th style="width: 30%;">상호작용 및 파급 효과</th>
                </tr>
            </thead>
            <tbody>
                <tr>
                    <td><code>define_subagent<br>(adversarial-auditor)</code></td>
                    <td><span class="badge badge-subagent">SubAgent</span></td>
                    <td>Read-Only 심사 전문 에이전트 런타임 정의, 엄격한 심사관 시스템 프롬프트 부여</td>
                    <td>코드 오염 없이 비판적 결함만 탐지하는 독립 실행 주체 생성</td>
                </tr>
                <tr>
                    <td><code>.antigravity/rules/<br>07-adversarial-audit.md</code></td>
                    <td><span class="badge badge-rule">Rule</span></td>
                    <td>간결성, 기존 구조 활용, 범위 내 확장성, 네트워크/일반 최적화 5대 루브릭 규정</td>
                    <td>심사관과 작업자가 동일한 잣대로 소통할 수 있는 객관적 기준 제공</td>
                </tr>
                <tr>
                    <td><code>AGENTS.md<br>(Section 3 &amp; 4)</code></td>
                    <td><span class="badge badge-doc">Governance</span></td>
                    <td>파이프라인 실행 모드 7번에 편입, 도메인 룰 목록 링크, 최소 수정 원칙 준수 강조</td>
                    <td>메인 에이전트가 코드 커밋 전 서브에이전트 감사를 의무화하도록 강제</td>
                </tr>
                <tr>
                    <td><code>.agents/skills/<br>interrogate/SKILL.md</code></td>
                    <td><span class="badge badge-skill">Skill</span></td>
                    <td>/interrogate 슬래시 커맨드 등록, diff 수집 -> 서브에이전트 호출 자동화</td>
                    <td>사용자 한마디로 복잡한 멀티에이전트 교차 검증 절차가 원클릭 실행됨</td>
                </tr>
            </tbody>
        </table>

        <h2>4. 엔드투엔드 실행 라이프사이클 (End-to-End Operational Lifecycle)</h2>
        <div class="diagram-box">
[사용자 요청 / 기능 구현 완료]
      │
      ▼
1. 사용자 호출: "/interrogate" 또는 "코드 검사해줘"
      │
      ▼
2. Skill 활성화: .agents/skills/interrogate/SKILL.md 로드
   - `git diff` 자동 수집 및 의도(Intent) 정리
      │
      ▼
3. Subagent 실행: invoke_subagent(TypeName: "adversarial-auditor")
   - Rule 참조: .antigravity/rules/07-adversarial-audit-standards.md
   - 5대 핵심 차원(간결성/재사용/확장성/네트워크/일반최적화) 대조 평가
      │
      ▼
4. 리포트 반환: Findings Matrix (BLOCKING / WARNING / SUGGESTION)
      │
      ├───────────────────────────────┐
      │ (BLOCKING 결함 발견 시)       │ (ALL PASS / APPROVED 시)
      ▼                               ▼
5. 메인 에이전트가 결함 자동 수정      6. 안전하게 커밋 및 빌드 파이프라인 진행
   - pipeline.py static/build 재검증     - python Scripts/pipeline.py full</div>

        <h2>5. 향후 신규 SubAgent 확장 시 표준 체크리스트</h2>
        <p>
            향후 프로젝트에 또 다른 서브에이전트(예: <em>Balance-Sim-Auditor, Asset-Validator</em> 등)를 추가할 때도 이번과 동일하게 다음 4단계를 수행해야 시스템 무결성이 유지됩니다:
        </p>
        <ol>
            <li><strong>SubAgent 정의</strong>: <code>define_subagent</code>로 목적에 맞는 도구 권한(Read-Only 여부)과 시스템 프롬프트 설정.</li>
            <li><strong>도메인 룰 분리</strong>: <code>.antigravity/rules/XX-*.md</code>에 점진적 공개(Progressive Disclosure)가 가능한 기준 문서 작성.</li>
            <li><strong>지침서 반영</strong>: <code>AGENTS.md</code>의 파이프라인 모드 및 룰 목록에 공식 등록하여 단일 진실 공급원 유지.</li>
            <li><strong>호출 스킬 구축</strong>: <code>.agents/skills/&lt;name&gt;/SKILL.md</code>에 트리거 키워드와 컨텍스트 조립 절차를 작성.</li>
        </ol>
    </div>
    <div class="footer-bar">
        <span>FC Project Engineering &amp; Verification Protocol</span>
        <span>Page 3 of 3</span>
    </div>
</div>

</body>
</html>
"""

def main():
    print("Writing temporary HTML...")
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
            print("Temporary HTML cleaned up.")
    else:
        print("[ERROR] PDF was not generated.")
        sys.exit(1)

if __name__ == "__main__":
    main()
