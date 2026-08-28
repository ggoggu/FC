#!/usr/bin/env python3
"""
Unreal Engine 5.8 Mathematical TTK (Time-to-Kill) & Combat Balance Simulator
Computes weapon DPS, effective damage scaling against armor tiers, hits-to-kill (HTK), and TTK curves.
Supports archetype benchmarking and automated Markdown / JSON comparison outputs for autonomous balance tuning.
"""

import sys
import math
import json
import argparse
from dataclasses import dataclass, asdict

# Standard Armor Profiles for Benchmarking
STANDARD_ARMOR_TIERS = {
    "Unarmored": {"armor": 0.0, "reduction": 0.0, "health": 100.0},
    "Light":     {"armor": 25.0, "reduction": 0.0, "health": 125.0},
    "Medium":    {"armor": 50.0, "reduction": 0.05, "health": 150.0},
    "Heavy":     {"armor": 100.0, "reduction": 0.10, "health": 200.0},
    "Boss":      {"armor": 200.0, "reduction": 0.20, "health": 500.0},
}

# Standard Weapon Archetypes for Benchmarking
ARCHETYPE_PRESETS = {
    "Dagger": {
        "base_damage": 22.0,
        "attack_speed": 3.0,     # 3 hits/sec
        "crit_chance": 0.30,
        "crit_multiplier": 2.0,
        "armor_pen": 0.05
    },
    "Arming_Sword": {
        "base_damage": 45.0,
        "attack_speed": 1.4,     # 1.4 hits/sec
        "crit_chance": 0.15,
        "crit_multiplier": 1.75,
        "armor_pen": 0.15
    },
    "Greatsword": {
        "base_damage": 110.0,
        "attack_speed": 0.7,     # 0.7 hits/sec
        "crit_chance": 0.10,
        "crit_multiplier": 2.2,
        "armor_pen": 0.30
    },
    "Assault_Rifle": {
        "base_damage": 28.0,
        "attack_speed": 9.0,     # 9 rounds/sec
        "crit_chance": 0.15,
        "crit_multiplier": 1.5,
        "armor_pen": 0.10
    },
    "Shotgun": {
        "base_damage": 130.0,    # Per blast (all pellets)
        "attack_speed": 1.0,     # 1 blast/sec
        "crit_chance": 0.10,
        "crit_multiplier": 1.5,
        "armor_pen": 0.05
    },
    "Sniper_Rifle": {
        "base_damage": 180.0,
        "attack_speed": 0.6,     # 0.6 shots/sec
        "crit_chance": 0.40,
        "crit_multiplier": 2.5,
        "armor_pen": 0.40
    }
}

def calculate_effective_damage(
    base_damage: float,
    armor: float,
    armor_pen: float,
    damage_reduction: float,
    crit_chance: float = 0.0,
    crit_multiplier: float = 1.0
) -> dict:
    """
    Computes effective damage per hit factoring in armor scaling, penetration, damage reduction, and expected crit.
    Armor formula: Multiplier = 100 / (100 + EffectiveArmor)
    """
    effective_armor = max(0.0, armor * (1.0 - armor_pen))
    armor_mitigation = 100.0 / (100.0 + effective_armor)
    mitigated_base_hit = base_damage * armor_mitigation * (1.0 - damage_reduction)

    # Average hit considering critical strike expectancy
    avg_crit_multiplier = (1.0 - crit_chance) * 1.0 + crit_chance * crit_multiplier
    avg_hit = mitigated_base_hit * avg_crit_multiplier
    crit_hit = mitigated_base_hit * crit_multiplier

    return {
        "effective_armor": round(effective_armor, 2),
        "mitigation_percentage": round((1.0 - armor_mitigation * (1.0 - damage_reduction)) * 100.0, 2),
        "base_hit_damage": round(mitigated_base_hit, 2),
        "crit_hit_damage": round(crit_hit, 2),
        "avg_hit_damage": round(avg_hit, 2)
    }

def calculate_ttk(
    base_damage: float,
    attack_speed: float,
    crit_chance: float,
    crit_multiplier: float,
    armor_pen: float,
    target_health: float,
    target_armor: float,
    target_reduction: float
) -> dict:
    """
    Calculates Time-To-Kill (TTK), Hits-To-Kill (HTK), and sustained DPS.
    TTK assumes the first hit lands at t=0s: TTK = (Hits - 1) / AttackSpeed.
    """
    dmg_metrics = calculate_effective_damage(
        base_damage=base_damage,
        armor=target_armor,
        armor_pen=armor_pen,
        damage_reduction=target_reduction,
        crit_chance=crit_chance,
        crit_multiplier=crit_multiplier
    )

    avg_hit = dmg_metrics["avg_hit_damage"]
    base_hit = dmg_metrics["base_hit_damage"]

    # HTK based on average expected damage
    htk_avg = math.ceil(target_health / max(avg_hit, 0.001))
    # HTK based purely on non-crit base hits
    htk_non_crit = math.ceil(target_health / max(base_hit, 0.001))

    # TTK (First hit at t=0)
    ttk_avg_seconds = round(max(0.0, (htk_avg - 1) / max(attack_speed, 0.001)), 3)
    ttk_non_crit_seconds = round(max(0.0, (htk_non_crit - 1) / max(attack_speed, 0.001)), 3)

    raw_dps = round(base_damage * attack_speed * ((1.0 - crit_chance) + crit_chance * crit_multiplier), 2)
    effective_dps = round(avg_hit * attack_speed, 2)

    return {
        "target_health": target_health,
        "target_armor": target_armor,
        "target_reduction": target_reduction,
        "effective_damage": dmg_metrics,
        "raw_dps": raw_dps,
        "effective_dps": effective_dps,
        "htk_avg": htk_avg,
        "htk_non_crit": htk_non_crit,
        "ttk_avg_seconds": ttk_avg_seconds,
        "ttk_non_crit_seconds": ttk_non_crit_seconds
    }

def run_benchmark() -> dict:
    results = {}
    for weapon_name, weapon_specs in ARCHETYPE_PRESETS.items():
        tier_results = {}
        for tier_name, tier_specs in STANDARD_ARMOR_TIERS.items():
            ttk_data = calculate_ttk(
                base_damage=weapon_specs["base_damage"],
                attack_speed=weapon_specs["attack_speed"],
                crit_chance=weapon_specs["crit_chance"],
                crit_multiplier=weapon_specs["crit_multiplier"],
                armor_pen=weapon_specs["armor_pen"],
                target_health=tier_specs["health"],
                target_armor=tier_specs["armor"],
                target_reduction=tier_specs["reduction"]
            )
            tier_results[tier_name] = ttk_data
        results[weapon_name] = {
            "specs": weapon_specs,
            "tiers": tier_results
        }
    return results

def format_markdown_table(benchmark_data: dict) -> str:
    lines = []
    lines.append("# Combat TTK & Balance Simulation Benchmark\n")
    lines.append("| Weapon Archetype | Base Dmg | Speed (hps) | Raw DPS | Light TTK (s) | Medium TTK (s) | Heavy TTK (s) | Boss TTK (s) |")
    lines.append("| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |")

    for weapon, data in benchmark_data.items():
        specs = data["specs"]
        tiers = data["tiers"]
        raw_dps = tiers["Unarmored"]["raw_dps"]
        light_ttk = f"{tiers['Light']['ttk_avg_seconds']}s ({tiers['Light']['htk_avg']} hits)"
        med_ttk = f"{tiers['Medium']['ttk_avg_seconds']}s ({tiers['Medium']['htk_avg']} hits)"
        heavy_ttk = f"{tiers['Heavy']['ttk_avg_seconds']}s ({tiers['Heavy']['htk_avg']} hits)"
        boss_ttk = f"{tiers['Boss']['ttk_avg_seconds']}s ({tiers['Boss']['htk_avg']} hits)"

        lines.append(f"| **{weapon}** | {specs['base_damage']} | {specs['attack_speed']} | {raw_dps} | {light_ttk} | {med_ttk} | {heavy_ttk} | {boss_ttk} |")

    return "\n".join(lines)

def main():
    parser = argparse.ArgumentParser(description="Unreal Engine 5.8 Combat Balance & TTK Simulator")
    parser.add_argument("--benchmark", action="store_true", help="Run full benchmark across standard weapon archetypes and armor tiers")
    parser.add_argument("--markdown", action="store_true", help="Output benchmark in Markdown table format")
    parser.add_argument("--base-damage", type=float, default=50.0, help="Weapon base damage")
    parser.add_argument("--attack-speed", type=float, default=1.5, help="Weapon attack speed (hits/sec)")
    parser.add_argument("--crit-chance", type=float, default=0.10, help="Critical strike chance (0.0 - 1.0)")
    parser.add_argument("--crit-mult", type=float, default=2.0, help="Critical strike damage multiplier")
    parser.add_argument("--armor-pen", type=float, default=0.10, help="Armor penetration ratio (0.0 - 1.0)")
    parser.add_argument("--target-health", type=float, default=100.0, help="Target total health")
    parser.add_argument("--target-armor", type=float, default=50.0, help="Target armor value")
    parser.add_argument("--target-reduction", type=float, default=0.0, help="Target flat damage reduction percentage")
    parser.add_argument("--json", action="store_true", default=True, help="Emit output as JSON (default: True)")
    parser.add_argument("--pretty", action="store_true", help="Format JSON with indentation")

    args = parser.parse_args()

    if args.benchmark:
        benchmark_results = run_benchmark()
        if args.markdown:
            print(format_markdown_table(benchmark_results))
        else:
            output = {
                "status": "PASS",
                "mode": "BENCHMARK",
                "archetypes": benchmark_results
            }
            print(json.dumps(output, indent=2 if args.pretty else None))
        sys.exit(0)

    # Single simulation
    sim_result = calculate_ttk(
        base_damage=args.base_damage,
        attack_speed=args.attack_speed,
        crit_chance=args.crit_chance,
        crit_multiplier=args.crit_mult,
        armor_pen=args.armor_pen,
        target_health=args.target_health,
        target_armor=args.target_armor,
        target_reduction=args.target_reduction
    )

    output = {
        "status": "PASS",
        "mode": "SINGLE",
        "weapon_input": {
            "base_damage": args.base_damage,
            "attack_speed": args.attack_speed,
            "crit_chance": args.crit_chance,
            "crit_multiplier": args.crit_mult,
            "armor_pen": args.armor_pen
        },
        "simulation": sim_result
    }

    print(json.dumps(output, indent=2 if args.pretty else None))
    sys.exit(0)

if __name__ == "__main__":
    main()
