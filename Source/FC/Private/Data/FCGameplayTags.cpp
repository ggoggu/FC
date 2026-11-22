#include "Data/FCGameplayTags.h"

namespace FCGameplayTags
{
	// --- Elements ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Root, "Element", "Root tag for elemental affinities");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_None, "Element.None", "Non-elemental affinity");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Fire, "Element.Fire", "Fire element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Earth, "Element.Earth", "Earth element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Water, "Element.Water", "Water/Ice element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Wind, "Element.Wind", "Wind element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Lightning, "Element.Lightning", "Lightning element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Holy, "Element.Holy", "Holy element");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Element_Dark, "Element.Dark", "Dark element");

	// --- Character Classes ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Root, "Class", "Root tag for character classes");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Neutral, "Class.Neutral", "Neutral class usable by all");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Mage, "Class.Mage", "Mage character class");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Warrior, "Class.Warrior", "Warrior character class");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Rogue, "Class.Rogue", "Rogue character class");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Class_Priest, "Class.Priest", "Priest character class");

	// --- Card Types ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Type_Root, "Card.Type", "Root tag for card functional categories");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Type_Attack, "Card.Type.Attack", "Attack card category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Type_Skill, "Card.Type.Skill", "Skill card category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Type_Power, "Card.Type.Power", "Power card category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Type_Item, "Card.Type.Item", "Item card category");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Type_Curse, "Card.Type.Curse", "Curse card category");

	// --- Card Keywords ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Keyword_Root, "Card.Keyword", "Root tag for card lifecycle keywords");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Keyword_Exhaust, "Card.Keyword.Exhaust", "Card moves to exhaust pile when played");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Keyword_Retain, "Card.Keyword.Retain", "Card stays in hand across hand cycles");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Keyword_Innate, "Card.Keyword.Innate", "Card is prioritized in opening hand");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Keyword_Ethereal, "Card.Keyword.Ethereal", "Card exhausts if held at end of cycle");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Card_Keyword_Unplayable, "Card.Keyword.Unplayable", "Card cannot be played directly");

	// --- Combat Factions & Attack Traits ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Team_Player, "Combat.Team.Player", "Player faction team tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Team_Enemy, "Combat.Team.Enemy", "Enemy/Mob faction team tag");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Target_Destructible, "Combat.Target.Destructible", "Destructible combat object");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Attack_Projectile, "Combat.Attack.Projectile", "Projectile attack type");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Combat_Attack_Unblockable, "Combat.Attack.Unblockable", "Attack that penetrates projectile barriers");

	// --- GAS SetByCaller Data Tags ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Damage, "Data.Damage", "SetByCaller tag for incoming damage");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Heal, "Data.Heal", "SetByCaller tag for incoming heal");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Magnitude, "Data.Magnitude", "SetByCaller tag for generic magnitude");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Duration, "Data.Duration", "SetByCaller tag for generic duration");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Data_Shield, "Data.Shield", "SetByCaller tag for shield amount");

	// --- GameplayCues & Status ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(GameplayCue_Combat_HitReaction, "GameplayCue.Combat.HitReaction", "Hit reaction animation cue");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Buff_AttackPower, "Status.Buff.AttackPower", "Attack power buff status");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Debuff_AttackReduction, "Status.Debuff.AttackReduction", "Attack power reduction status");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Status_Buff_MagicShield, "Status.Buff.MagicShield", "Magic shield active status");

	// --- Dungeon Room Archetypes ---
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dungeon_Room_Normal, "Dungeon.Room.Normal", "Standard room archetype");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dungeon_Room_Start, "Dungeon.Room.Start", "Starting room archetype");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dungeon_Room_Boss, "Dungeon.Room.Boss", "Boss room archetype");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dungeon_Room_Shop, "Dungeon.Room.Shop", "Shop room archetype");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dungeon_Room_Treasure, "Dungeon.Room.Treasure", "Treasure room archetype");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Dungeon_Room_Secret, "Dungeon.Room.Secret", "Secret room archetype");
}
