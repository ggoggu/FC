# `FFCCardAbilityTargetData` 네트워크 직렬화 누락 크래시 분석 및 수정 보고서

**작성일**: 2026-11-20  
**대상 크래시**: `Saved/Crashes/UECC-Windows-F5F65F304E9CBDE62CF15FBBA35B4725_0000` (`Saved/Logs/FC.log`)  
**분류**: Gameplay Ability System (GAS) / 네트워크 리플리케이션 (`FGameplayAbilityTargetDataHandle::NetSerialize`)

---

## 1. 요약 (Executive Summary)

PIE 리슨 서버/클라이언트 환경에서 카드를 사용할 때 `GameplayAbilityTargetTypes.cpp:252`에서 Fatal Error가 발생하여 에디터가 즉시 종료되는 크래시를 분석하고 수정했습니다.

- **직접 원인**: `FFCCardAbilityTargetData`가 `FGameplayAbilityTargetData`를 상속받고 `GetScriptStruct()`를 오버라이드했으나, `NetSerialize()` 및 `TStructOpsTypeTraits::WithNetSerializer = true`를 구현하지 않아 `ServerInitiated` 어빌리티(`UFCGA_SpawnProjectile` 등) 활성화 시 `FGameplayAbilityTargetDataHandle::NetSerialize`가 Fatal Error를 발생시킴.
- **부수 경고**: `ActivateCardAbilityWithContext()`가 런타임 `Transient` 패키지에 생성된 `UFCCardDataAsset`을 `Spec.SourceObject` 및 `EventData.OptionalObject`에 할당하여 `FNetGUIDCache::SupportsObject ... NOT Supported` 경고를 유발함.
- **조치 결과**: `FFCCardAbilityTargetData::NetSerialize` 및 `TStructOpsTypeTraits` 구현, `SourceObject` 네트워크 지원 필터링, `CardId` / `EventMagnitude` 기반 fallback 조회 보강 후 정적 검증(`static`), UBT 빌드(`build`), 자동화 테스트(`test`) 모두 통과.

---

## 2. 크래시 로그 및 콜스택 분석

### 2.1 핵심 에러 로그 (`Saved/Logs/FC.log`)
```text
[2026.11.20-06.03.21:816][837]LogNetPackageMap: Warning: FNetGUIDCache::SupportsObject: FCCardDataAsset /Engine/Transient.UnrealEdEngine_0:BP_FCGameInstance_C_0.FCCardSubsystem_0.FCCardDataAsset_0 NOT Supported.
[2026.11.20-06.03.21:816][837]LogOutputDevice: Warning: 

Script Stack (1 frames) :
/Script/FC.FCCardDeckComponent.Server_PlayCard

[2026.11.20-06.03.21:816][837]LogWindows: Error: appError called: Fatal error: [File:D:\build\++UE5\Sync\Engine\Plugins\Runtime\GameplayAbilities\Source\GameplayAbilities\Private\GameplayAbilityTargetTypes.cpp] [Line: 252] 
FGameplayAbilityTargetDataHandle::NetSerialize called on data struct FCCardAbilityTargetData without a native NetSerialize
```

### 2.2 호출 흐름
```text
UFCCardDeckComponent::Server_PlayCard_Implementation() [FCCardDeckComponent.cpp:614]
  └─ FFCCardEffect_ActivateAbility::Execute() [FCCardEffects.cpp:191]
       └─ ActivateCardAbilityWithContext() [FCCardEffects.cpp:47-50]
            ├─ EventData.TargetData = Context.MakeAbilityTargetDataHandle() (FFCCardAbilityTargetData 포함)
            └─ ASC->GiveAbilityAndActivateOnce(Spec, &EventData)
                 └─ ServerInitiated 어빌리티 활성화 리플리케이션 -> FGameplayAbilityTargetDataHandle::NetSerialize()
                      └─ TStructOpsTypeTraits<FFCCardAbilityTargetData>::WithNetSerializer == false -> Fatal Error!
```

---

## 3. 근본 원인 (Root Cause)

1. **`FFCCardAbilityTargetData`의 `NetSerialize` 미구현** (`Source/FC/Public/Card/FCCardExecutionContext.h`)
   - UE5 GAS에서 `FGameplayAbilityTargetDataHandle::NetSerialize`는 포함된 `FGameplayAbilityTargetData` 파생 구조체가 `TStructOpsTypeTraits<T>::WithNetSerializer == true`를 갖추지 않은 경우 `UE_LOG(LogAbilitySystem, Fatal, ...)`로 즉시 크래시합니다.
   - `FFCCardAbilityTargetData`는 `CardId`, `UpgradeLevel`, `AimLocation`, `VerifiedTargets`를 보유하며 `GetScriptStruct()`를 오버라이드했지만 `NetSerialize()`와 `TStructOpsTypeTraits` 특수화가 누락되어 있었습니다.

2. **`Transient` `UFCCardDataAsset`의 `SourceObject` 할당** (`Source/FC/Private/Card/Effects/FCCardEffects.cpp`)
   - `FCCardSubsystem`이 런타임에 `Transient`로 생성하는 `UFCCardDataAsset`은 네트워크 `NetGUID`가 부여되지 않습니다.
   - 이를 `FGameplayAbilitySpec::SourceObject` / `FGameplayEventData::OptionalObject`에 직접 넣으면 리플리케이션 시 `FNetGUIDCache::SupportsObject ... NOT Supported` 경고가 발생하고 클라이언트에서는 `nullptr`로 수신됩니다.

---

## 4. 수정 내역

| 파일 | 변경 내용 |
| :--- | :--- |
| [FCCardExecutionContext.h](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Source/FC/Public/Card/FCCardExecutionContext.h) | `FFCCardAbilityTargetData::NetSerialize()` 구현 (`CardId`, `UpgradeLevel`, `AimLocation.NetSerialize`, `SafeNetSerializeTArray_Default<31>(Ar, VerifiedTargets)`) 및 `TStructOpsTypeTraits<FFCCardAbilityTargetData>` (`WithNetSerializer = true`, `WithCopy = true`) 추가 |
| [FCCardEffects.cpp](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Source/FC/Private/Card/Effects/FCCardEffects.cpp) | `ActivateCardAbilityWithContext()`에서 `CandidateSourceObject->IsSupportedForNetworking()`일 때만 `Spec.SourceObject` 및 `EventData.OptionalObject`에 할당하도록 가드 추가 |
| [FCGA_Ignite.cpp](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Source/FC/Private/AbilitySystem/Abilities/FCGA_Ignite.cpp) | `Spec->SourceObject` 캐스팅에 의존하기 전에 `TriggerEventData->EventMagnitude`를 우선 참조하도록 보강 |
| [FCGA_SpawnProjectile.cpp](file:///c:/Users/oogg/Documents/Unreal%20Projects/FC/Source/FC/Private/AbilitySystem/Abilities/FCGA_SpawnProjectile.cpp) | `Spec->SourceObject`에서 프로젝타일 에셋을 얻지 못한 경우 `FFCCardAbilityTargetData::CardId` + `UFCCardSubsystem::GetCardDataAsset()`로 `UFCProjectileDataAsset`을 조회하는 fallback 경로 추가 |

---

## 5. 검증 결과

| 검증 항목 | 명령어 | 결과 |
| :--- | :--- | :--- |
| **정적 검증 (4개 수정 파일)** | `python Scripts/pipeline.py static --file <path>` | **PASS** (각 파일 0 error, 0 warning) |
| **UBT 빌드** | `python Scripts/pipeline.py build` | **PASS** (13.45s, 0 error, 0 warning) |
| **자동화 테스트** | `python Scripts/pipeline.py test` | **PASS** (21.49s, `status=PASSED`) |
