import os
import subprocess
import time
from pathlib import Path
from PIL import Image, ImageChops

WORKSPACE_DIR = Path(r"c:\Users\oogg\Documents\Unreal Projects\FC")
REPORT_DIR = WORKSPACE_DIR / "Report"
SCRIPTS_DIR = WORKSPACE_DIR / "Scripts"
MERMAID_JS_PATH = SCRIPTS_DIR / "mermaid.min.js"
CHROME_PATH = r"C:\Program Files\Google\Chrome\Application\chrome.exe"

DIAGRAMS = {
    "FC_Overall_Architecture": {
        "title": "FC Project - 전체 시스템 아키텍처 구조도 (Overall Architecture)",
        "subtitle": "Unreal Engine 5.8 | Authoritative Dedicated/Listen Server & MVVM Architecture",
        "code": """graph TB
    subgraph Game_Framework["1. 게임 프레임워크 계층 (Game Framework)"]
        GM["AFCGameMode<br/>- 세션 생명주기 및 룰 관리<br/>- PostLogin / 레벨 전환"]
        GS["AFCGameState<br/>- 전역 게임 상태 동기화<br/>- 서버 권한 복제"]
        GI["UFCGameInstance<br/>- 전역 라이프사이클 관리"]
    end

    subgraph Subsystems["2. 서브시스템 계층 (Subsystems)"]
        CardSubsystem["UFCCardSubsystem (GameInstance)<br/>- 카드 카탈로그 / 데이터테이블 캐싱<br/>- 비동기 에셋 프리로딩 (Soft Reference)"]
        PersistenceSubsystem["UFCPlayerPersistenceSubsystem (GameInstance)<br/>- 맵 전환 간 덱/손패 스냅샷 저장<br/>- 체력/마나 어트리뷰트 복원"]
        ClassSubsystem["UFCClassSubsystem (GameInstance)<br/>- 클래스별 기본 덱/스킬 데이터셋"]
        MobSpawnSubsystem["UFCMobSpawnSubsystem (World)<br/>- 전역 몬스터 스포너 등록 및 조율<br/>- 필드 활성 몹 개체수 집계"]
    end

    subgraph Player_Hierarchy["3. 플레이어 및 폰 계층 (Player Hierarchy)"]
        PC["AFCPlayerController (Autonomous Client)<br/>- Enhanced Input / 조준점 판정<br/>- 카드 조작 및 Server RPC 전송"]
        PS["AFCPlayerState (Replicated State)<br/>- 캐릭터 클래스 복제 (OnRep)<br/>- UFCCardDeckComponent 소유"]
        PChar["AFCPlayerCharacter (Character Base)<br/>- 3인칭 이동 및 카메라 제어<br/>- 가속 체력 드레인 & 처치 흡혈"]
        DeckComp["UFCCardDeckComponent (Modular ActorComponent)<br/>- Draw / Hand / Discard / Exhaust 관리<br/>- 30초 주기 턴/마나 사이클"]
        HandContainer["FFCCardHandContainer<br/>- FFastArraySerializer 손패 복제<br/>- COND_OwnerOnly 대역폭 최적화"]
    end

    subgraph Combat_GAS["4. 전투 및 어빌리티 계층 (Combat & GAS)"]
        ASC["UAbilitySystemComponent<br/>- 어빌리티 인스턴스화 및 부여"]
        AttrSet["UFCAttributeSet<br/>- Health, Mana, Shield, AttackPower<br/>- OnRep 속성 동기화 및 델리게이트"]
        ElemComp["UFCElementComponent<br/>- 원소 스택 (최대 7개) 관리<br/>- FIFO / Clamp 오버플로 정책"]
        Abilities["Gameplay Abilities (GA)<br/>- FCGA_Fireball (화염구 투사체)<br/>- FCGA_Ignite (즉발 점화)<br/>- FCGA_SandWall (방벽 생성)"]
        Effects["Gameplay Effects (GE)<br/>- FCGE_Damage (직접 피해)<br/>- FCGE_AttackBuff (공격력 버프)<br/>- FCGE_MagicShield (마법 보호막)"]
        CombatUtils["UFCCombatUtils<br/>- 속성 히트 트레이트 적용<br/>- 원자적(Atomic) 원소 소모 판정"]
    end

    subgraph AI_Gameplay["5. AI 및 월드 엔티티 계층 (AI & World Gameplay)"]
        MobChar["AFCMobCharacter<br/>- 인간형 몬스터 / 사망 시 보상 드롭<br/>- 방향성 피격/사망 애니메이션 동기화"]
        MobAI["AFCMobAIController<br/>- Behavior Tree (순찰/추적/공격)<br/>- 커스텀 BT Task & Decorator"]
        CampSpawner["AFCMobCampSpawner<br/>- 캠프 반경 내 몹 생성 및 리스폰"]
        Rewards["AFCChestActor / FCCardPickupActor<br/>- 필드 보상 상자 및 카드 획득 액터"]
    end

    subgraph Presentation_UI["6. UI 및 프레젠테이션 계층 (UMG MVVM Tier)"]
        HUDVM["UFCHUDViewModel<br/>- Current/Max HP, Mana, Shield<br/>- 덱 카운터, 사이클 진행도"]
        HandVM["UFCHandViewModel<br/>- 활성 카드 컬렉션 바인딩"]
        CardVM["UFCCardViewModel<br/>- 개별 카드 메타데이터 및 UI 상태"]
        OverheadVM["FCElementOverheadViewModel<br/>- 몬스터 머리 위 원소 스택 표시"]
        HUDWidget["UFCHUDWidget / UFCHandWidget<br/>- UMG Slate 뷰 (서버 비결합)"]
    end

    %% Framework & Subsystem flow
    GM --> PS
    GM --> CardSubsystem
    GM --> PersistenceSubsystem
    MobSpawnSubsystem --> CampSpawner

    %% Player Connections
    PC --> PS
    PC --> PChar
    PS --> DeckComp
    DeckComp --> HandContainer
    DeckComp -.-> CardSubsystem

    %% Combat Connections
    PChar --> ASC
    PChar --> AttrSet
    PChar --> ElemComp
    MobChar --> ASC
    MobChar --> AttrSet
    MobChar --> ElemComp
    MobAI --> MobChar
    CampSpawner --> MobChar
    MobChar -.-> Rewards

    ASC --> Abilities
    ASC --> Effects
    Abilities --> CombatUtils
    CombatUtils --> ElemComp

    %% Presentation Connections
    PC -.-> HUDVM
    DeckComp -.-> HandVM
    AttrSet -.-> HUDVM
    ElemComp -.-> OverheadVM
    HandVM --> CardVM
    HUDVM --> HUDWidget
    HandVM --> HUDWidget"""
    },
    "FC_Card_Combat_Sequence": {
        "title": "FC Project - 카드 사용 및 전투 처리 시퀀스 (Card Play & Combat Flow)",
        "subtitle": "Unreal Engine 5.8 | Client Input -> Server Validation -> GAS Execution -> FastArray Replication",
        "code": """sequenceDiagram
    autonumber
    actor Player as 플레이어 (Client)
    participant PC as AFCPlayerController
    participant PS as AFCPlayerState
    participant DeckComp as UFCCardDeckComponent (Server)
    participant ASC as UAbilitySystemComponent (Server)
    participant ElemComp as UFCElementComponent (Target)
    participant HUD as UFCHUDWidget / MVVM (Client)

    Note over Player,HUD: [1단계: 사용자 입력 및 3D 타겟 조준]
    Player->>PC: 숫자키 (슬롯 선택) 또는 좌클릭 (카드 사용 의도)
    PC->>PC: ResolveCardTargetUnderCursor()<br/>(마우스 커서 레이캐스트로 대상 액터 및 3D 월드 좌표 산출)
    PC->>DeckComp: Server_PlayCard(CardGuid, TargetInfo) [WithValidation RPC]

    Note over DeckComp,ElemComp: [2단계: 서버 권한 검증 및 GAS 어빌리티 발동]
    rect rgb(245, 248, 255)
        DeckComp->>DeckComp: 핸드 내 카드 유효성 확인 (CardGuid 존재 여부)
        DeckComp->>ASC: 마나 비용 충족 확인 및 차감
        DeckComp->>ASC: TryActivateAbilityByClass() (지정된 GameplayAbility 실행)
        ASC->>ElemComp: UFCCombatUtils::ApplyAttackCardHitTraits()<br/>(Mage 공격 속성 스택 부여 또는 원자적 소모)
        DeckComp->>DeckComp: 손패에서 카드 제거 후 Discard Pile로 이동
    end

    Note over PS,HUD: [3단계: 최적화 네트워크 동기화 및 UI 갱신]
    DeckComp-->>PS: FFCCardHandContainer 변경 복제 (FastArray, COND_OwnerOnly)
    ASC-->>PC: UFCAttributeSet 복제 (Mana / Health OnRep)
    PS-->>HUD: OnCardHandUpdated 델리게이트 발송
    HUD->>HUD: MVVM FieldNotify 반영 (손패 슬롯 제거, 마나 바 애니메이션)"""
    },
    "FC_Mob_Spawn_Combat_Sequence": {
        "title": "FC Project - 몬스터 스폰 및 전투/보상 시퀀스 (Mob Spawn & Combat Flow)",
        "subtitle": "Unreal Engine 5.8 | World Subsystem -> Behavior Tree -> Damage/Direction -> Kill Heal & Drop",
        "code": """sequenceDiagram
    autonumber
    participant SpawnerSubsystem as UFCMobSpawnSubsystem (World)
    participant CampSpawner as AFCMobCampSpawner (Server)
    participant MobChar as AFCMobCharacter (Server)
    participant AI as AFCMobAIController (Behavior Tree)
    participant Player as AFCPlayerCharacter
    participant Chest as AFCChestActor (World)

    Note over SpawnerSubsystem,AI: [1단계: 월드 몬스터 스폰 및 AI 제어]
    SpawnerSubsystem->>CampSpawner: ActivateAllSpawners()
    CampSpawner->>MobChar: SpawnActor (지정 반경 내 무작위 스폰)
    CampSpawner->>SpawnerSubsystem: RegisterActiveMob()
    MobChar->>AI: Possess & RunBehaviorTree()
    AI->>AI: 순찰(Patrol) -> 감지(Chase) -> 공격(Attack) 루프

    Note over Player,MobChar: [2단계: 플레이어 공격 피격 및 리액션]
    Player->>MobChar: 스킬 투사체 충돌 / 타격 (Damage Inflicted)
    MobChar->>MobChar: HandleDamageTaken() (서버 권한 데미지 차감)
    MobChar->>MobChar: CalculateHitDirection(Player) (전/후/좌/우 피격 각도 산출)
    MobChar-->>Player: Multicast_PlayHitAnimation(Direction) (비신뢰성 코스메틱 RPC)

    Note over MobChar,Chest: [3단계: 사망 처리, 처치 흡혈 및 보상 생성]
    MobChar->>MobChar: Die() (체력 0 도달, 이동 및 콜리전 정지)
    MobChar-->>Player: Multicast_PlayDeathAnimation(Direction) (신뢰성 사망 RPC)
    MobChar->>Player: OnKilledEnemy(MobChar) 호출
    Player->>Player: ApplyHeal(HealOnKillAmount) (플레이어 즉시 체력 회복)
    MobChar->>Chest: SpawnActor (보물 상자 / 카드 픽업 드롭)
    MobChar->>CampSpawner: NotifyMobDied() (리스폰 타이머 작동)"""
    },
    "FC_4Tier_Authority_Model": {
        "title": "FC Project - 4계층 권한 분리 모델 (4-Tier Authority Model)",
        "subtitle": "Unreal Engine 5.8 | Authoritative Server -> Replicated State -> Autonomous Client -> Presentation UI",
        "code": """graph TB
    subgraph Tier1["1. Authoritative Server (서버 전용 권한 계층)"]
        T1_Desc["<b>책임:</b> 전투 판정, 피해 계산, 사망 처리, 덱/인벤토리 변이, 스폰 제어<br/><b>보장:</b> HasAuthority() == true 환경에서만 실행"]
        T1_Classes["- AFCGameMode<br/>- AFCMobAIController / Behavior Tree<br/>- UFCMobSpawnSubsystem<br/>- UFCPlayerPersistenceSubsystem"]
    end

    subgraph Tier2["2. Replicated State (서버-클라이언트 상태 동기화 계층)"]
        T2_Desc["<b>책임:</b> 게임플레이 상태 동기화, 대역폭 최적화, OnRep 콜백<br/><b>보장:</b> DOREPLIFETIME_CONDITION, FFastArraySerializer 적용"]
        T2_Classes["- AFCPlayerState (직업, 덱 소유)<br/>- UFCCardDeckComponent (COND_OwnerOnly 덱 존)<br/>- UFCAttributeSet (HP, Mana, Shield)<br/>- UFCElementComponent (원소 스택 컨테이너)"]
    end

    subgraph Tier3["3. Autonomous Client (로컬 입력 및 예측 계층)"]
        T3_Desc["<b>책임:</b> Enhanced Input 수신, 이동 예측(Prediction), 서버 RPC 발송<br/><b>보장:</b> WithValidation RPC 통한 무결성 검증"]
        T3_Classes["- AFCPlayerController (커서 타겟팅, Server_PlayCard)<br/>- AFCPlayerCharacter (WASD 로컬 이동 예측)<br/>- Input Mapping Contexts"]
    end

    subgraph Tier4["4. Simulated Client & UI (프레젠테이션 및 뷰 계층)"]
        T4_Desc["<b>책임:</b> UMG 위젯 렌더링, 오디오/SFX, 나이아가라 VFX, 코스메틱 애니메이션<br/><b>보장:</b> 서버 코드와 100% 분리 (Zero Server Coupling)"]
        T4_Classes["- MVVM ViewModels (UFCHUDViewModel, UFCHandViewModel 등)<br/>- UMG Widgets (UFCHUDWidget, UFCHandWidget, FCCardWidget)<br/>- Multicast RPC 코스메틱 몽타주<br/>- UNiagaraComponent 원소 이펙트"]
    end

    Tier1 ==>|Replication / OnRep| Tier2
    Tier3 ==>|Server_* RPCs WithValidation| Tier1
    Tier2 ==>|Delegates / RepNotify| Tier4
    Tier3 -.->|UI Intent Binding| Tier4"""
    }
}

HTML_TEMPLATE = """<!DOCTYPE html>
<html>
<head>
  <meta charset="utf-8">
  <title>{title}</title>
  <style>
    * {{
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }}
    body {{
      background-color: #f8fafc;
      font-family: 'Segoe UI', -apple-system, BlinkMacSystemFont, 'Apple SD Gothic Neo', Roboto, 'Noto Sans KR', sans-serif;
      padding: 30px;
      display: inline-block;
      min-width: 1300px;
    }}
    .diagram-container {{
      background-color: #ffffff;
      border: 1px solid #e2e8f0;
      border-radius: 12px;
      box-shadow: 0 10px 25px -5px rgba(0, 0, 0, 0.05), 0 8px 10px -6px rgba(0, 0, 0, 0.03);
      overflow: hidden;
      padding: 28px 36px;
    }}
    .header {{
      border-bottom: 2px solid #f1f5f9;
      padding-bottom: 18px;
      margin-bottom: 28px;
    }}
    .title {{
      font-size: 26px;
      font-weight: 700;
      color: #0f172a;
      letter-spacing: -0.02em;
      margin-bottom: 6px;
    }}
    .subtitle {{
      font-size: 15px;
      color: #64748b;
      font-weight: 500;
    }}
    .mermaid {{
      display: flex;
      justify-content: center;
      background-color: #ffffff;
    }}
  </style>
  <script src="{mermaid_js_uri}"></script>
</head>
<body>
  <div class="diagram-container" id="diagram-card">
    <div class="header">
      <div class="title">{title}</div>
      <div class="subtitle">{subtitle}</div>
    </div>
    <div class="mermaid" id="mermaid-graph">
{code}
    </div>
  </div>
  <script>
    mermaid.initialize({{
      startOnLoad: true,
      theme: 'base',
      themeVariables: {{
        fontFamily: "'Segoe UI', -apple-system, BlinkMacSystemFont, 'Apple SD Gothic Neo', Roboto, 'Noto Sans KR', sans-serif",
        fontSize: '14px',
        primaryColor: '#eef2ff',
        primaryTextColor: '#1e293b',
        primaryBorderColor: '#6366f1',
        lineColor: '#475569',
        secondaryColor: '#f0fdf4',
        secondaryBorderColor: '#22c55e',
        tertiaryColor: '#fef2f2',
        tertiaryBorderColor: '#ef4444',
        noteBkgColor: '#fef9c3',
        noteTextColor: '#713f12',
        noteBorderColor: '#eab308',
        actorBkg: '#f8fafc',
        actorBorder: '#64748b',
        actorTextColor: '#0f172a',
        signalColor: '#2563eb',
        signalTextColor: '#1e293b',
        labelBoxBkgColor: '#f8fafc',
        labelBoxBorderColor: '#cbd5e1',
        labelTextColor: '#334155'
      }}
    }});
  </script>
</body>
</html>
"""

def render_diagrams():
    REPORT_DIR.mkdir(parents=True, exist_ok=True)
    SCRIPTS_DIR.mkdir(parents=True, exist_ok=True)

    mermaid_js_uri = MERMAID_JS_PATH.as_uri()

    for name, data in DIAGRAMS.items():
        print(f"=== Rendering {name} ===")
        html_file = SCRIPTS_DIR / f"{name}.html"
        raw_png_file = REPORT_DIR / f"{name}_raw.png"
        final_png_file = REPORT_DIR / f"{name}.png"

        html_content = HTML_TEMPLATE.format(
            title=data["title"],
            subtitle=data["subtitle"],
            mermaid_js_uri=mermaid_js_uri,
            code=data["code"]
        )
        html_file.write_text(html_content, encoding="utf-8")

        # Run Chrome Headless
        cmd = [
            CHROME_PATH,
            "--headless=new",
            "--disable-gpu",
            "--force-device-scale-factor=2",
            "--window-size=3200,2600",
            "--virtual-time-budget=4000",
            f"--screenshot={str(raw_png_file)}",
            html_file.as_uri()
        ]
        res = subprocess.run(cmd, capture_output=True, text=True)
        if not raw_png_file.exists():
            print(f"Failed to render {name}: {res.stderr}")
            continue

        # Crop around content
        img = Image.open(raw_png_file)
        bg = Image.new(img.mode, img.size, (248, 250, 252))
        diff = ImageChops.difference(img.convert("RGB"), bg)
        bbox = diff.getbbox()

        if bbox:
            pad = 24
            w, h = img.size
            crop_box = (
                max(0, bbox[0] - pad),
                max(0, bbox[1] - pad),
                min(w, bbox[2] + pad),
                min(h, bbox[3] + pad)
            )
            cropped = img.crop(crop_box)
            cropped.save(final_png_file, "PNG", optimize=True)
            print(f"Saved: {final_png_file.name} (Resolution: {cropped.size[0]}x{cropped.size[1]})")
        else:
            img.save(final_png_file, "PNG", optimize=True)
            print(f"Saved (uncropped): {final_png_file.name} (Resolution: {img.size[0]}x{img.size[1]})")

        if raw_png_file.exists():
            raw_png_file.unlink()
        if html_file.exists():
            html_file.unlink()

    print("\nAll diagrams successfully generated!")

if __name__ == "__main__":
    render_diagrams()
