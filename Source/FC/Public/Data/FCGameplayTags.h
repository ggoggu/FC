#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * FCGameplayTags
 *
 * Centralized native GameplayTag declarations for FC core domains.
 * Feature-specific tags can also be added independently via Config/Tags/*.ini
 * or UGameplayTagsManager DataTables without modifying this file.
 */
namespace FCGameplayTags
{
	// --- Elements ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Root);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_None);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Fire);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Earth);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Water);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Wind);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Lightning);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Holy);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Element_Dark);

	// --- Character Classes ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Root);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Neutral);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Mage);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Warrior);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Rogue);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Class_Priest);

	// --- Card Types ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Type_Root);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Type_Attack);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Type_Skill);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Type_Power);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Type_Item);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Type_Curse);

	// --- Card Keywords ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Root);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Exhaust);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Retain);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Innate);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Ethereal);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Card_Keyword_Unplayable);

	// --- Combat Factions & Attack Traits ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Team_Player);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Team_Enemy);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Target_Destructible);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Attack_Projectile);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Attack_Unblockable);

	// --- GAS SetByCaller Data Tags ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Damage);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Heal);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Magnitude);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Duration);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Data_Shield);

	// --- GameplayCues & Status ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Combat_HitReaction);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Buff_AttackPower);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Debuff_AttackReduction);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Buff_MagicShield);

	// --- Dungeon Room Archetypes ---
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dungeon_Room_Normal);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dungeon_Room_Start);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dungeon_Room_Boss);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dungeon_Room_Shop);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dungeon_Room_Treasure);
	FC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Dungeon_Room_Secret);
}
