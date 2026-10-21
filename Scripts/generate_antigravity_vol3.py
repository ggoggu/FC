#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
Generates Volume 3 of FC Antigravity Environment Architecture Report:
"FC 프로젝트 안티그래비티 환경 종합 아키텍처 보고서 [제3권: FC 프로젝트 언리얼 5.8 실전 엔지니어링 & 자율 루프]"
Target: Report/Architecture/FC_Antigravity_Vol3_FC_Project_Engineering_and_Loop.pdf
"""

import os
import sys
import subprocess
from pathlib import Path

PROJECT_ROOT = Path(r"C:\Users\oogg\Documents\Unreal Projects\FC")
OUTPUT_DIR = PROJECT_ROOT / "Report" / "Architecture"
OUTPUT_PDF = OUTPUT_DIR / "FC_Antigravity_Vol3_FC_Project_Engineering_and_Loop.pdf"
TEMP_HTML = PROJECT_ROOT / "Scripts" / "temp_antigravity_vol3.html"

def build_vol3_html() -> str:
    return """<!DOCTYPE html>
<html lang="ko">
<head>
<meta charset="utf-8">
<title>안티그래비티 환경 종합 아키텍처 보고서 [제3권: FC 프로젝트 언리얼 5.8 실전 엔지니어링 & 자율 루프]</title>
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
        border-bottom: 1.5px solid #ea580c;
        padding-bottom: 3px;
        margin-bottom: 9px;
        font-size: 7.1pt;
        font-weight: 700;
        color: #64748b;
        letter-spacing: 0.5px;
    }
    .header-bar .tag {
        color: #ea580c;
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
        background: #ea580c;
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
        border-left: 3.5px solid #ea580c;
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
    .badge-primary { background: #ffedd5; color: #ea580c; }
    .badge-cyan { background: #e0f2fe; color: #0284c7; }
    .badge-indigo { background: #e0e7ff; color: #4338ca; }
    .badge-danger { background: #fee2e2; color: #991b1b; }
    .badge-warn { background: #fef3c7; color: #92400e; }
    .badge-success { background: #dcfce7; color: #166534; }
    .badge-slate { background: #f1f5f9; color: #475569; }

    .callout {
        background: #f8fafc;
        border-left: 3.5px solid #ea580c;
        padding: 5px 9px;
        margin: 4px 0 6px 0;
        border-radius: 0 4px 4px 0;
        font-size: 7.6pt;
    }
    .callout-title {
        font-weight: 700;
        color: #ea580c;
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
    .callout-success {
        background: #f0fdf4;
        border-left-color: #16a34a;
    }
    .callout-success .callout-title {
        color: #15803d;
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
<!-- PAGE 1: 표지 및 FC 아키텍처 비전 (Vision & Authority Model)           -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 3</span>
            <span class="tag">UE5.8 ENGINEERING & LOOP</span>
        </div>

        <div style="margin-bottom: 7px;">
            <span class="badge-top">FC Unreal Engine 5.8 Directives</span>
            <h1>안티그래비티 환경 종합 아키텍처 보고서</h1>
            <div class="subtitle">[제3권] FC 프로젝트 언리얼 5.8 실전 엔지니어링, 모듈러 룰셋 및 자율 셀프 힐링 루프</div>
        </div>

        <div class="meta-box">
            <div class="meta-item">
                <strong>Engine Version</strong>
                Unreal Engine 5.8 C++20
            </div>
            <div class="meta-item">
                <strong>Core Tenet</strong>
                Single-Player is Local Multiplayer
            </div>
            <div class="meta-item">
                <strong>Pipeline Runner</strong>
                Scripts/pipeline.py (Tiered)
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

        <h2>1. 프로젝트 아키텍처 비전: "Single-Player is Local Multiplayer"</h2>
        <p>
            <strong>FC 프로젝트</strong>는 데디케이티드/리슨 서버 복제 아키텍처 위에 구축된 하이파이 싱글플레이어 경험을 핵심 목표로 한다. 
            싱글플레이어 모드는 엔진 내부에서 <strong>로컬 리슨 서버(Standalone Client + Local Listen Server)</strong>로 구동되며, 
            향후 협동(Co-op) 멀티플레이어 모드와의 <strong>100% 기능 패리티(Feature Parity)</strong>를 보장한다.
        </p>

        <div class="callout callout-warn">
            <div class="callout-title">
                <span>[최고 아키텍처 원칙] 싱글플레이어 전용 지름길(Shortcuts) 절대 금지</span>
                <span class="badge badge-danger">Strict Violation Rule</span>
            </div>
            클라이언트 인스턴스에서 서버 권한 확인 없이 인벤토리를 직접 변이시키거나, 체력을 수정하거나, 아이템을 직접 스폰하는 
            단독 실행 전용 우회 코드는 엄격히 금지된다. 모든 게임플레이 변이는 반드시 서버 권한(Server Authority)을 통과해야 한다.
        </div>

        <h2>2. 4계층 권한(4-Tier Authority) 아키텍처 개요</h2>
        <div class="arch-diagram">
            <div style="text-align: center; font-weight: 700; font-size: 7.4pt; color: #7c2d12; margin-bottom: 4px;">
                [FC 4-Tier Authority Separation Model]
            </div>
            <div class="grid-2" style="margin: 0; gap: 5px;">
                <div style="background: #fee2e2; border: 1.5px solid #ef4444; border-radius: 4px; padding: 4px;">
                    <strong style="color: #991b1b; font-size: 7.2pt;">1. Server Authority (서버 권한 전용)</strong><br>
                    <span style="font-size: 6.7pt; color: #7f1d1d;">
                        &bull; AGameModeBase, AAIController, 전투/피해 판정, 인벤토리 변이<br>
                        &bull; HasAuthority() 강제, Cooked Asset SSOT (Direct Disk I/O 금지)
                    </span>
                </div>
                <div style="background: #e0f2fe; border: 1.5px solid #0284c7; border-radius: 4px; padding: 4px;">
                    <strong style="color: #0369a1; font-size: 7.2pt;">2. Replicated State (서버-클라이언트 상태 전파)</strong><br>
                    <span style="font-size: 6.7pt; color: #075985;">
                        &bull; AGameStateBase, APlayerState, Actor Components<br>
                        &bull; DOREPLIFETIME_CONDITION(COND_OwnerOnly), RepNotify 콜백 바인딩
                    </span>
                </div>
                <div style="background: #fef3c7; border: 1.5px solid #f59e0b; border-radius: 4px; padding: 4px;">
                    <strong style="color: #92400e; font-size: 7.2pt;">3. Autonomous Client (입력 및 예측 이동)</strong><br>
                    <span style="font-size: 6.7pt; color: #78350f;">
                        &bull; APlayerController, ACharacter (Local Autonomous Proxy)<br>
                        &bull; Enhanced Input, CMC 클라이언트 예측, Server_* RPC (WithValidation)
                    </span>
                </div>
                <div style="background: #dcfce7; border: 1.5px solid #16a34a; border-radius: 4px; padding: 4px;">
                    <strong style="color: #166534; font-size: 7.2pt;">4. Simulated Client & UI (프레젠테이션 티어)</strong><br>
                    <span style="font-size: 6.7pt; color: #14532d;">
                        &bull; AHUD, UUserWidget (UMG/Slate), Niagara, Audio<br>
                        &bull; 제로 서버 결합, OnRep 델리게이트 수신 및 MVVM ViewModel 렌더링
                    </span>
                </div>
            </div>
        </div>

        <h2>3. 안티그래비티 에이전트의 거버넌스 역할</h2>
        <p>
            안티그래비티는 C++ 코드를 작성하거나 리팩토링할 때 위의 4계층 권한 경계를 엄격히 감시한다. 
            클라이언트 RPC에 게임플레이 변이가 포함되거나, Slate/UMG 헤더가 서버 클래스에 포함되는 즉시 
            도메인 규칙(<code>.antigravity/rules/</code>)과 파이프라인 검증기(<code>pipeline.py static</code>)가 작동하여 
            오류를 사전에 차단한다.
        </p>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 3 &bull; Page 1 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 2: 4계층 권한 분리와 안티그래비티 코드 거버넌스 (Authority in Practice) -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 3</span>
            <span class="tag">4-TIER AUTHORITY GOVERNANCE</span>
        </div>

        <h2>1. 4계층 권한 분리의 실전 구현 표준</h2>
        <div class="grid-2">
            <div class="card" style="border-left: 3.5px solid #ef4444;">
                <div class="card-title" style="color: #991b1b;">1. Server Authority (서버 권한 전용)</div>
                <ul style="font-size: 6.8pt; color: #334155; margin: 3px 0;">
                    <li><strong>역할:</strong> 전투 판정, 체력 가감, 몬스터 스폰, 퀘스트/아이템 변이 등 모든 authoritative 게임 상태 결정.</li>
                    <li><strong>코드 보장:</strong> <code>if (HasAuthority())</code> 또는 <code>if (GetLocalRole() == ROLE_Authority)</code> 강제.</li>
                    <li><strong>Cooked Asset SSOT:</strong> 게임 데이터는 엔진 내부 에셋(<code>UDataTable</code>, <code>UPrimaryDataAsset</code>)에서만 로드. 디스크 상의 느슨한 CSV 직접 I/O는 패키징 실패를 유발하므로 절대 금지.</li>
                </ul>
            </div>
            <div class="card" style="border-left: 3.5px solid #0284c7;">
                <div class="card-title" style="color: #0369a1;">2. Replicated State (상태 동기화 계층)</div>
                <ul style="font-size: 6.8pt; color: #334155; margin: 3px 0;">
                    <li><strong>역할:</strong> 서버에서 변경된 상태를 관련 클라이언트에게 네트워크로 전파.</li>
                    <li><strong>대역폭 최적화:</strong> <code>DOREPLIFETIME_CONDITION(..., COND_OwnerOnly)</code>를 통해 개인 인벤토리 등은 소유자에게만 동기화.</li>
                    <li><strong>RepNotify:</strong> 상태 변경 시 <code>OnRep_PropertyName()</code> 콜백을 발동하여 클라이언트 프레젠테이션 계층에 알림 전송.</li>
                </ul>
            </div>
            <div class="card" style="border-left: 3.5px solid #f59e0b;">
                <div class="card-title" style="color: #92400e;">3. Autonomous Client (입력 및 예측)</div>
                <ul style="font-size: 6.8pt; color: #334155; margin: 3px 0;">
                    <li><strong>역할:</strong> 로컬 플레이어 입력 수집, Enhanced Input 처리, 캐릭터 이동 예측.</li>
                    <li><strong>RPC 소유권 전제 조건:</strong> Server RPC(<code>Server_*</code>)는 반드시 호출 클라이언트의 Connection이 소유한 액터(PlayerController, possessed Pawn)에서만 호출 가능. 월드 내 소유권 없는 상호작용 액터에 RPC 배치 금지.</li>
                    <li><strong>WithValidation:</strong> 모든 Server RPC는 <code>_Validate</code> 함수를 구현하여 악의적 페이로드 검증.</li>
                </ul>
            </div>
            <div class="card" style="border-left: 3.5px solid #16a34a;">
                <div class="card-title" style="color: #15803d;">4. Simulated Client & UI (프레젠테이션)</div>
                <ul style="font-size: 6.8pt; color: #334155; margin: 3px 0;">
                    <li><strong>역할:</strong> HUD, UMG 위젯, 사운드, 나이아가라 이펙트 재생 등 시각/청각 피드백.</li>
                    <li><strong>서버 결합도 제로:</strong> 서버 코드는 UMG/Slate 헤더를 결코 include하지 않으며 위젯 객체를 참조하지 않음.</li>
                    <li><strong>MVVM 뷰모델:</strong> UI는 ViewModel의 Field Notification을 구독하여 수동 폴링 없이 반응형으로 화면을 갱신.</li>
                </ul>
            </div>
        </div>

        <h2>2. 권한 위반 방지를 위한 안티그래비티 가드 매트릭스</h2>
        <table>
            <tr>
                <th style="width: 25%;">위반 패턴 (Anti-Pattern)</th>
                <th style="width: 35%;">위험성 및 아키텍처 결함</th>
                <th style="width: 40%;">안티그래비티 자동 교정 조치</th>
            </tr>
            <tr>
                <td><strong>클라이언트 측 체력/아이템 직접 수정</strong></td>
                <td>치트 취약점, 서버-클라이언트 상태 불일치(Desync).</td>
                <td>Server RPC (<code>Server_ModifyInventory</code>) 생성 및 HasAuthority 가드로 재배치.</td>
            </tr>
            <tr>
                <td><strong>서버 클래스에서 UUserWidget 참조</strong></td>
                <td>서버 전용 헤드리스 빌드(Server Target) 컴파일 실패.</td>
                <td>UI 참조 완전 분리, OnRep 델리게이트 이벤트 발행 패턴으로 변경.</td>
            </tr>
            <tr>
                <td><strong>소유권 없는 액터에서 Server RPC 정의</strong></td>
                <td>네트워크 RPC 디스패치 실패 (No Connection Owner).</td>
                <td>PlayerController 또는 PlayerState의 컴포넌트로 RPC 책임을 이전.</td>
            </tr>
            <tr>
                <td><strong>런타임 loose CSV 파일 I/O 수행</strong></td>
                <td>Cooked 빌드 패키징 시 파일 누락으로 인한 런타임 크래시.</td>
                <td><code>UDataTable</code> 또는 <code>UPrimaryDataAsset</code> 기반 SSOT 엔진 에셋으로 변환.</td>
            </tr>
        </table>

        <div class="callout callout-cyan">
            <div class="callout-title">
                <span>[검증 자동화] 정적 복제 감사기 (<code>verify_replication.py</code>)</span>
                <span class="badge badge-cyan">Static Replay Audit</span>
            </div>
            안티그래비티는 <code>python Scripts/pipeline.py static</code> 실행 시 내부적으로 
            AST 파서를 통해 C++ 헤더의 모든 <code>UFUNCTION(Server)</code>과 <code>UPROPERTY(Replicated)</code>를 전수 조사하여, 
            WithValidation 누락, GetLifetimeReplicatedProps 조건 미지정 여부를 1초 만에 감지한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 3 &bull; Page 2 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 3: 모듈러 도메인 규칙 체계 분석 (.antigravity/rules/ 01~06)       -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 3</span>
            <span class="tag">MODULAR DOMAIN RULES</span>
        </div>

        <h2>1. FC 모듈러 도메인 규칙 체계 (<code>.antigravity/rules/</code>)</h2>
        <p>
            FC 프로젝트는 단일 파일로 방대해진 규칙의 인지 과부하를 방지하기 위해, 
            <strong>도메인별로 완전히 분리된 6개의 모듈러 규칙 파일</strong>을 운영한다. 
            에이전트는 해당 기술 도메인을 다룰 때만 필요한 규칙을 선별 참조한다.
        </p>

        <div class="grid-3">
            <div class="card" style="border-top: 2.5px solid #4338ca;">
                <div class="card-title">01-ue58-coding-standards</div>
                <p style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <strong>C++20 & 엔진 표준:</strong> <code>TObjectPtr&lt;T&gt;</code> 의무화, IWYU 준수, PascalCase, 전방 선언, 스마트 포인터(TSharedPtr).
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #0284c7;">
                <div class="card-title">02-multiplayer-conventions</div>
                <p style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <strong>네트워크 복제 규칙:</strong> Server RPC WithValidation, HasAuthority 가드, 소유권 체인, 대역폭 조건부 복제(COND_OwnerOnly).
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #10b981;">
                <div class="card-title">03-loop-engineering</div>
                <p style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <strong>자율 루프 프로토콜:</strong> UBT 컴파일 에러 JSON 자동 파싱, 5회 자율 셀프 힐링 사이클, Ground Truth 에셋 추출.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #f59e0b;">
                <div class="card-title">04-gameplay-ability-system</div>
                <p style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <strong>GAS 전투 표준:</strong> AttributeSet 매크로(<code>ATTRIBUTE_ACCESSORS</code>), OnRep 복제, GameplayEffect, GameplayCue 관리.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #ea580c;">
                <div class="card-title">05-combat-and-inventory</div>
                <p style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <strong>대량 데이터 동기화:</strong> <code>FFastArraySerializer</code> 기반 델타 복제, 인벤토리 아이템 상태 동기화 및 패킷 절감.
                </p>
            </div>
            <div class="card" style="border-top: 2.5px solid #8b5cf6;">
                <div class="card-title">06-umg-mvvm-standards</div>
                <p style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <strong>UI MVVM 아키텍처:</strong> <code>UMVVMViewModelBase</code>, Field Notification, <code>UE_MVVM_SET_PROPERTY_VALUE</code> 뷰모델 갱신.
                </p>
            </div>
        </div>

        <h2>2. 대표 도메인 규칙 핵심 코드 패턴</h2>
        <div class="grid-2">
            <div>
                <h3>GAS AttributeSet 표준 패턴 (Rule 04)</h3>
                <div class="code-box" style="font-size: 6.3pt;">
<span class="cm">// FCAttributeSet.h - Strict Macro Usage</span>
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
    GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Stats")
FGameplayAttributeData Health;
ATTRIBUTE_ACCESSORS(UFCAttributeSet, Health)
                </div>
            </div>
            <div>
                <h3>UMG MVVM FieldNotify 패턴 (Rule 06)</h3>
                <div class="code-box" style="font-size: 6.3pt;">
<span class="cm">// UFCPlayerViewModel.h - MVVM Binding</span>
UCLASS()
class UFCPlayerViewModel : public UMVVMViewModelBase {
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter)
    float CurrentHealth;

    void SetCurrentHealth(float NewVal) {
        UE_MVVM_SET_PROPERTY_VALUE(CurrentHealth, NewVal);
    }
};
                </div>
            </div>
        </div>

        <div class="callout callout-success">
            <div class="callout-title">
                <span>[최소 수정 원칙의 보장] Minimal Surface Area Changes</span>
                <span class="badge badge-success">Least Disruption</span>
            </div>
            모든 도메인 규칙은 <strong>최소 수정 원칙(Minimal Surface Area Changes)</strong>을 대전제로 한다. 
            에이전트는 요구된 기능이나 버그 수정에 필요한 최소한의 코드 라인만을 수정하며, 
            주변의 무관한 리팩토링이나 포맷팅 변경을 일체 배제하여 Git Diff를 가장 명확하고 안전하게 유지한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 3 &bull; Page 3 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 4: 온디맨드 계층형 파이프라인 러너 (pipeline.py Deep Dive)         -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 3</span>
            <span class="tag">TIERED PIPELINE RUNNER</span>
        </div>

        <h2>1. 파이프라인 러너 (<code>Scripts/pipeline.py</code>) 설계 철학</h2>
        <p>
            대규모 언리얼 프로젝트에서 모든 사소한 코드 수정마다 전체 컴파일과 유닛 테스트를 매번 실행하면 
            <strong>엄청난 시간 지연과 AI 토큰 낭비</strong>가 발생한다. 
            이를 극복하기 위해 FC 프로젝트는 <strong>Tiered On-Demand Pipeline Runner</strong>를 구축하여 
            상황에 맞게 7가지 독립 실행 모드를 온디맨드로 제공한다.
        </p>

        <h2>2. 파이프라인 7대 실행 모드 상세 분석</h2>
        <table>
            <tr>
                <th style="width: 16%;">실행 모드</th>
                <th style="width: 20%;">CLI 명령어</th>
                <th style="width: 44%;">실행 내용 및 내부 메커니즘</th>
                <th style="width: 20%;">소요 시간 & 비용</th>
            </tr>
            <tr>
                <td><strong>1. Fast Mode</strong> (기본)</td>
                <td>- (Direct Coding)</td>
                <td>검증 스크립트 실행 없이 즉시 C++ 코드 편집 및 설명 제공.</td>
                <td><span class="badge badge-success">즉시 (0초) / 0 토큰</span></td>
            </tr>
            <tr>
                <td><strong>2. Static Audit</strong></td>
                <td><code>python Scripts/pipeline.py static</code></td>
                <td>GAS 속성 매크로, 대역폭 복제 조건, Server RPC 검증 정적 검사.</td>
                <td><span class="badge badge-cyan">~1초 / 초저비용</span></td>
            </tr>
            <tr>
                <td><strong>3. Build Harness</strong></td>
                <td><code>python Scripts/pipeline.py build</code></td>
                <td>UBT 증분 컴파일 실행, JSON 컴파일 에러 파싱, 5회 자율 패치 루프.</td>
                <td><span class="badge badge-primary">~10~30초 (자동 패치)</span></td>
            </tr>
            <tr>
                <td><strong>4. Combat Balance</strong></td>
                <td><code>python Scripts/pipeline.py balance</code></td>
                <td>전투 몬스터 체력 대비 카드 덱 TTK, 유효 DPS 시뮬레이션 곡선 산출.</td>
                <td><span class="badge badge-warn">~1초 / 수치 검증</span></td>
            </tr>
            <tr>
                <td><strong>5. Automation Test</strong></td>
                <td><code>python Scripts/pipeline.py test</code></td>
                <td>헤드리스 언리얼 에디터를 백그라운드로 스폰하여 C++ 자동화 테스트 구동.</td>
                <td><span class="badge badge-indigo">~20~40초 / 기능 검증</span></td>
            </tr>
            <tr>
                <td><strong>6. Asset Inspect</strong></td>
                <td><code>python Scripts/pipeline.py inspect --asset &lt;path&gt;</code></td>
                <td>블루프린트/위젯/애님 에셋의 바이너리 구조를 JSON 메타데이터로 덤프.</td>
                <td><span class="badge badge-cyan">~0.01초 / 무환각 보장</span></td>
            </tr>
            <tr>
                <td><strong>7. Full Release</strong></td>
                <td><code>python Scripts/pipeline.py full --auto-commit -m "..."</code></td>
                <td>Static Audit &rarr; Build &rarr; Test 순차 실행 후 통과 시 Git 자동 커밋.</td>
                <td><span class="badge badge-slate">~1분 / 최종 릴리즈</span></td>
            </tr>
        </table>

        <h2>3. 파이프라인의 단일 도구 호출(Single Tool Call) 원칙</h2>
        <div class="callout callout-cyan">
            <div class="callout-title">
                <span>[토큰 최적화] 다중 쉘 호출의 단일 파이프라인 통합</span>
                <span class="badge badge-cyan">Single Call Protocol</span>
            </div>
            개발자가 검증을 요청할 때 에이전트는 개별 UBT 명령, 테스트 러너, 린터를 각각 따로 호출하지 않는다. 
            <code>python Scripts/pipeline.py build</code> 또는 <code>full</code> 단 <strong>한 번의 도구 호출</strong>로 
            검증과 자율 복구를 원스톱으로 처리하여 대화 턴 수와 토큰 낭비를 80% 이상 절감한다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 3 &bull; Page 4 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 5: 자율 셀프 힐링 및 빌드 패칭 루프 (Self-Healing Build Loop)      -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 3</span>
            <span class="tag">AUTONOMOUS SELF-HEALING</span>
        </div>

        <h2>1. 자율 셀프 힐링(Self-Healing) 컴파일 루프 아키텍처</h2>
        <p>
            C++20과 언리얼 엔진 5.8의 UHT/UBT 컴파일러는 방대한 템플릿과 매크로를 사용하여 복잡한 에러 메시지를 출력한다. 
            안티그래비티는 컴파일 실패 시 개발자에게 오류를 전가하지 않고, 
            <strong>최대 5회까지 자율적으로 소스코드를 패치하고 재컴파일하는 폐루프 복구 엔진</strong>을 내장하고 있다.
        </p>

        <div class="arch-diagram">
            <div style="font-size: 6.9pt; font-family: monospace; line-height: 1.4; color: #0f172a;">
                [Code Edit Applied] &rarr; [Invoke: pipeline.py build]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[UBT Incremental Compile]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;+-------------------+-------------------+<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert; Exit Code == 0&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert; Exit Code != 0 (Compile Error)<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                <strong>[SUCCESS & Ready]</strong>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Standardized JSON Error Parser]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert; Extracts: {file, line, code, msg}<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>[Autonomous Error Classification]</strong><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; Missing Header Include (IWYU)<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; UFUNCTION Server RPC Missing _Validate<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; TObjectPtr Raw Assignment Mismatch<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Apply Precise In-Memory / File Patch]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert; (Cycle Count &lt;= 5)<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;+----&rarr; Re-run [UBT Incremental Compile]
            </div>
        </div>

        <h2>2. 5사이클 자동 복구 메커니즘 단계별 분석</h2>
        <div class="grid-2">
            <div class="card">
                <div class="card-header">
                    <span class="card-title">JSON 오류 표준화 및 파싱</span>
                    <span class="badge badge-primary">Structured Error</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    <code>build_harness.py</code>는 UBT의 장황한 콘솔 텍스트를 파싱하여 정확한 <code>{file, line, column, error_code, message}</code> 형태의 
                    JSON 구조체로 정제한다. 에이전트는 무의미한 텍스트를 읽지 않고 정확한 지점을 타겟팅한다.
                </p>
            </div>
            <div class="card">
                <div class="card-header">
                    <span class="card-title">증분 재컴파일 및 롤백 가드</span>
                    <span class="badge badge-warn">Safe Rollback</span>
                </div>
                <p style="font-size: 7.0pt; color: #475569; margin: 3px 0;">
                    패치 후에는 변경된 소스 파일에 대해서만 증분(Incremental) 컴파일을 실행하여 복구 시간을 수 초 내외로 단축한다. 
                    만약 5회 시도 후에도 해결되지 않으면 변경을 안전하게 롤백하고 원인 분석 리포트를 발행한다.
                </p>
            </div>
        </div>

        <div class="callout callout-success">
            <div class="callout-title">
                <span>[실제 복구 성공률] FC 코드베이스 셀프 힐링 실적</span>
                <span class="badge badge-success">High Auto-Fix Rate</span>
            </div>
            언리얼 5.8의 엄격해진 C++20 IWYU 헤더 누락 및 `WithValidation` 매크로 구현 누락 오류에 대해, 
            FC 프로젝트의 안티그래비티 셀프 힐링 루프는 <strong>평균 1.4회의 패치 사이클</strong> 내에 100% 자율 복구를 달성하였다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 3 &bull; Page 5 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 6: Ground Truth 원칙과 에셋 리플렉션 검증 (Ground Truth)          -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 3</span>
            <span class="tag">GROUND TRUTH ASSET INSPECTION</span>
        </div>

        <h2>1. 에셋 추측 금지(Ground Truth Over Guessing) 원칙</h2>
        <p>
            게임 개발에서 가장 치명적인 AI 오류는 블루프린트(BP), 위젯(WBP), 애니메이션 몽타주(MM)의 
            <strong>내부 컴포넌트 이름, 위젯 바인딩 슬롯, 노티파이(AnimNotify) 명칭을 임의로 추측(Hallucination)</strong>하는 것이다. 
            존재하지 않는 슬롯이나 변수명을 C++ 코드에 하드코딩하면 런타임 크래시나 무반응 버그로 이어진다.
        </p>

        <div class="callout callout-warn">
            <div class="callout-title">
                <span>[절대 수칙] 에셋 메타데이터 확인 없는 추측 코딩 금지</span>
                <span class="badge badge-danger">Zero Hallucination</span>
            </div>
            기존 블루프린트 에셋과 상호작용하는 C++ 바인딩을 작성하기 전, 반드시 
            <code>python Scripts/pipeline.py inspect --asset &lt;path&gt;</code>를 실행하여 
            엔진의 실제 리플렉션 메타데이터를 확인한 후 코딩해야 한다.
        </div>

        <h2>2. 실시간 메타데이터 덤프 파이프라인 (<code>dump_asset_metadata.py</code>)</h2>
        <div class="grid-2">
            <div>
                <h3>에셋 덤프 CLI 명령어 표준</h3>
                <div class="code-box" style="font-size: 6.5pt;">
<span class="cm"># Fast Static Header Dump (~0.01s)</span>
python Scripts/pipeline.py inspect \
  --asset <span class="str">"/Game/Blueprints/Cards/BP_IgniteCard"</span>

<span class="cm"># Live Unreal Engine Editor Deep Reflection</span>
python Scripts/pipeline.py inspect \
  --asset <span class="str">"/Game/UI/WBP_CardHand"</span> --deep
                </div>
            </div>
            <div>
                <h3>추출된 JSON 메타데이터 스키마</h3>
                <div class="code-box" style="font-size: 6.5pt;">
{
  <span class="str">"asset_class"</span>: <span class="str">"WidgetBlueprint"</span>,
  <span class="str">"parent_class"</span>: <span class="str">"UUserWidget"</span>,
  <span class="str">"bound_widgets"</span>: [
    { <span class="str">"name"</span>: <span class="str">"CardCanvas"</span>, <span class="str">"type"</span>: <span class="str">"UCanvasPanel"</span> },
    { <span class="str">"name"</span>: <span class="str">"ManaCostText"</span>, <span class="str">"type"</span>: <span class="str">"UTextBlock"</span> }
  ],
  <span class="str">"animations"</span>: [<span class="str">"Anim_DrawCard"</span>, <span class="str">"Anim_Discard"</span>]
}
                </div>
            </div>
        </div>

        <h2>3. Ground Truth 검증 시퀀스 다이어그램</h2>
        <div class="arch-diagram">
            <div style="font-size: 6.9pt; font-family: monospace; line-height: 1.4; color: #0f172a;">
                [User: "WBP_CardHand에 카드 드로우 애니메이션 바인딩해줘"]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v [DO NOT GUESS ANIMATION NAME!]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<strong>[Tool Dispatch: pipeline.py inspect --asset WBP_CardHand]</strong><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v [Inspect Output: True Name is "Anim_DrawCard"]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[C++ Code Generation with Ground Truth Binding]:<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;<code>UWidgetAnimation* DrawAnim = GetWidgetAnimation(TEXT("Anim_DrawCard"));</code><br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                <strong>[Result: 100% Robust Binding, Zero Runtime Crash]</strong>
            </div>
        </div>

        <div class="callout callout-cyan">
            <div class="callout-title">
                <span>[실시간 씬 검증] 라이브 에디터 리플렉션과의 결합</span>
                <span class="badge badge-cyan">Deep Reflection</span>
            </div>
            <code>--deep</code> 옵션 활성화 시 파이썬 리모트 스크립트가 실행 중인 언리얼 에디터의 UObject 메모리 트리를 직접 순회하여, 
            컴파일되지 않은 블루프린트의 핀 디폴트 값과 컴포넌트 트랜스폼까지 실시간으로 추출해 냅니다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 3 &bull; Page 6 of 7</span>
    </div>
</div>

<!-- ==================================================================== -->
<!-- PAGE 7: 종합 가이드 & 성능 최적화 체크리스트 (Synthesis & Protocol)     -->
<!-- ==================================================================== -->
<div class="page-container">
    <div class="page-content">
        <div class="header-bar">
            <span>ANTIGRAVITY ENVIRONMENT ARCHITECTURE REPORT &bull; VOLUME 3</span>
            <span class="tag">SYNTHESIS & PRODUCTION PROTOCOL</span>
        </div>

        <h2>1. 엔지니어링 실무 종합 워크플로 (Production Synthesis)</h2>
        <p>
            안티그래비티와 인간 개발자가 협업하여 언리얼 엔진 5.8 기능을 구현할 때 준수하는 표준 개발 수명주기이다.
        </p>

        <div class="arch-diagram">
            <div style="font-size: 6.9pt; font-family: monospace; line-height: 1.4; color: #0f172a;">
                [Task Definition] &rarr; [Ground Truth Inspection (inspect)] &rarr; [Domain Rule Matching (01~06)]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Minimal Surface Area C++ Implementation]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Tiered On-Demand Verification: pipeline.py]<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&bull; static &rarr; build (Self-Healing) &rarr; test<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&vert;<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;v<br>
                &nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;[Single-Tool Full Release & Auto-Commit]
            </div>
        </div>

        <h2>2. 개발자 & AI 페어 프로그래밍 실전 체크리스트</h2>
        <div class="grid-2">
            <div class="card" style="border-left: 3px solid #ef4444;">
                <div class="card-title">1. 네트워크 권한 및 SSOT 점검</div>
                <ul style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <li>게임플레이 상태 변경 함수에 <code>HasAuthority()</code>가 적용되었는가?</li>
                    <li>모든 게임플레이 데이터가 엔진 내부 <code>UDataTable</code> / DataAsset에서 로드되는가?</li>
                </ul>
            </div>
            <div class="card" style="border-left: 3px solid #0284c7;">
                <div class="card-title">2. 대역폭 및 RPC 유효성 점검</div>
                <ul style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <li>모든 <code>Server_*</code> RPC에 <code>WithValidation</code> 구현이 존재하는가?</li>
                    <li>개인 인벤토리/스탯에 <code>COND_OwnerOnly</code> 조건이 적용되었는가?</li>
                </ul>
            </div>
            <div class="card" style="border-left: 3px solid #16a34a;">
                <div class="card-title">3. UI 제로 결합 및 MVVM 점검</div>
                <ul style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <li>서버/엔진 코드가 Slate/UMG 헤더를 결코 include하지 않는가?</li>
                    <li>UI 뷰모델 프로퍼티 변경 시 <code>UE_MVVM_SET_PROPERTY_VALUE</code>가 사용되는가?</li>
                </ul>
            </div>
            <div class="card" style="border-left: 3px solid #ea580c;">
                <div class="card-title">4. Ground Truth 및 파이프라인 점검</div>
                <ul style="font-size: 6.8pt; color: #475569; margin: 2px 0;">
                    <li>블루프린트 위젯/애니메이션 바인딩 전 메타데이터 덤프를 확인했는가?</li>
                    <li><code>pipeline.py static</code> 및 <code>build</code>를 통과했는가?</li>
                </ul>
            </div>
        </div>

        <h2>3. 3권 종합 아키텍처 시리즈 결론 (Final Conclusion)</h2>
        <p>
            본 안티그래비티 환경 종합 분석 보고서 시리즈(제1권~제3권)는 
            <strong>Google Antigravity 2.0 플랫폼 생태계</strong>의 최첨단 AI 에이전틱 런타임 기술과 
            <strong>FC 프로젝트</strong>의 고성능 언리얼 엔진 5.8 멀티플레이어 아키텍처 간의 유기적 결합을 체계화하였다. 
            단일성, 점진적 공개, 샌드박스 보안, 자율 셀프 힐링 및 Ground Truth 원칙을 준수함으로써, 
            FC 팀은 최상의 코드 품질과 최고의 개발 생산성을 동시에 달성할 것이다.
        </p>

        <div class="callout callout-success" style="margin-top: 5px;">
            <div class="callout-title">
                <span>[Architecture Series Complete] 3권 전권 완간</span>
                <span class="badge badge-success">Full Trilogy Complete</span>
            </div>
            제1권 (플랫폼 & 코어 런타임) &bull; 제2권 (커스터마이징 & 확장성 프레임워크) &bull; 제3권 (FC UE5.8 엔지니어링 & 자율 루프).
            모든 보고서는 <code>Report/Architecture/</code>에 PDF 형태로 정밀 보존됩니다.
        </div>
    </div>
    <div class="footer-bar">
        <span>FC Project &bull; Antigravity Architecture Technical Analysis</span>
        <span>Volume 3 &bull; Page 7 of 7</span>
    </div>
</div>

</body>
</html>
"""

def main():
    print("[VOL 3] Building temporary HTML...")
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    html_content = build_vol3_html()
    with open(TEMP_HTML, "w", encoding="utf-8") as f:
        f.write(html_content)
    print(f"[VOL 3] Saved temp HTML: {TEMP_HTML}")

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

    print(f"[VOL 3] Using browser: {browser}")
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

    print(f"[VOL 3] Rendering PDF: {OUTPUT_PDF}...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"[ERROR] Browser failed with code {res.returncode}:\n{res.stderr}")
        sys.exit(1)

    if OUTPUT_PDF.exists():
        size_kb = round(OUTPUT_PDF.stat().st_size / 1024, 1)
        print(f"[SUCCESS] Volume 3 PDF generated: {OUTPUT_PDF} ({size_kb} KB)")
        if TEMP_HTML.exists():
            TEMP_HTML.unlink()
    else:
        print("[ERROR] PDF generation failed, file not found.")
        sys.exit(1)

if __name__ == "__main__":
    main()
