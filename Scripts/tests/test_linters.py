#!/usr/bin/env python3
"""
Harness Meta-Test Suite (Scripts/tests/test_linters.py)
Validates that every static verifier, runtime runner guard, circuit breaker,
and Antigravity hook actually catches real violations and passes valid code.
Zero external dependencies (Python stdlib unittest).
"""

import json
import sys
import tempfile
import unittest
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent.parent
SCRIPTS_DIR = PROJECT_ROOT / "Scripts"
HOOKS_DIR = PROJECT_ROOT / ".agents" / "hooks"

sys.path.insert(0, str(SCRIPTS_DIR))
sys.path.insert(0, str(HOOKS_DIR))

import audit_bandwidth
import guard_verifiers
import pipeline
import run_tests
import simulate_net_pie
import stop_static_gate
import verify_gas
import verify_replication


class TempSourceTestCase(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.root = Path(self.tmp.name)
        self.src = self.root / "Source" / "FC"
        self.src.mkdir(parents=True, exist_ok=True)

    def tearDown(self):
        self.tmp.cleanup()

    def write_src(self, rel_path: str, content: str) -> Path:
        full = self.root / rel_path
        full.parent.mkdir(parents=True, exist_ok=True)
        full.write_text(content, encoding="utf-8")
        return full


class TestVerifyReplication(TempSourceTestCase):
    def test_catches_multiline_raw_this_lambda(self):
        self.write_src(
            "Source/FC/Private/TestComp.cpp",
            """
            void UTestComp::Bind(UAbilitySystemComponent* ASC)
            {
                ASC->GetDelegate().AddLambda(
                    [this, ASC](int32 Val)
                    {
                        Handle(Val);
                    });
            }
            """,
        )
        res = verify_replication.run_verification(self.root / "Source", baseline_path=None)
        rules = [v["rule"] for v in res["violations"]]
        self.assertIn("CODE_UNSAFE_RAW_THIS_LAMBDA", rules)
        self.assertEqual(res["status"], "FAILED")

    def test_passes_add_weak_lambda(self):
        self.write_src(
            "Source/FC/Private/TestComp.cpp",
            """
            void UTestComp::Bind(UAbilitySystemComponent* ASC)
            {
                ASC->GetDelegate().AddWeakLambda(
                    this,
                    [this](int32 Val)
                    {
                        Handle(Val);
                    });
            }
            """,
        )
        res = verify_replication.run_verification(self.root / "Source", baseline_path=None)
        self.assertEqual(res["error_count"], 0)
        self.assertEqual(res["status"], "PASSED")

    def test_catches_multiline_constructor_helpers_and_honors_baseline(self):
        self.write_src(
            "Source/FC/Private/TestActor.cpp",
            """
            ATestActor::ATestActor()
            {
                static ConstructorHelpers::FObjectFinder<UStaticMesh> FrozenFinder(
                    TEXT("/Game/Meshes/SM_Old.SM_Old"));
                static ConstructorHelpers::FClassFinder<AActor> NewClassFinder(
                    TEXT("/Game/BP/BP_New"));
            }
            """,
        )
        baseline_file = self.root / "baseline.json"
        baseline_file.write_text(
            json.dumps(
                {
                    "CODE_HARDCODED_ASSET_PATH": [
                        {"file": "Source/FC/Private/TestActor.cpp", "symbol": "FrozenFinder"}
                    ]
                }
            ),
            encoding="utf-8",
        )
        res = verify_replication.run_verification(self.root / "Source", baseline_path=baseline_file)
        self.assertEqual(res["baseline_suppressed"], 1)
        self.assertEqual(res["error_count"], 1)
        self.assertIn("NewClassFinder", res["violations"][0]["message"])

        res_ign = verify_replication.run_verification(
            self.root / "Source", baseline_path=baseline_file, ignore_baseline=True
        )
        self.assertEqual(res_ign["error_count"], 2)

    def test_no_class_truncation_with_nested_struct(self):
        self.write_src(
            "Source/FC/Public/NestedTest.h",
            """
            UCLASS()
            class FC_API ANestedActor : public AActor
            {
                GENERATED_BODY()
            public:
                struct FInnerHelper { int32 X = 0; };

                UPROPERTY(Replicated)
                int32 Health = 100;

                UPROPERTY()
                AActor* RawActorPtr = nullptr;
            };
            """,
        )
        res = verify_replication.run_verification(self.root / "Source", baseline_path=None)
        rules = {v["rule"] for v in res["violations"]}
        self.assertIn("REPL_MISSING_LIFETIME_PROPS_DECL", rules)
        self.assertIn("UE58_RAW_POINTER_UPROPERTY", rules)

    def test_rejects_server_rpc_on_ai_controller(self):
        self.write_src(
            "Source/FC/Public/MyAI.h",
            """
            UCLASS()
            class FC_API AMyAIController : public AAIController
            {
                GENERATED_BODY()
            public:
                UFUNCTION(Server, Reliable, WithValidation)
                void Server_DoSomething(int32 Value);
            };
            """,
        )
        res = verify_replication.run_verification(self.root / "Source", baseline_path=None)
        rules = {v["rule"] for v in res["violations"]}
        self.assertIn("REPL_SERVER_RPC_UNOWNED_ACTOR", rules)

    def test_escape_hatch_comments_are_hard_errors(self):
        self.write_src(
            "Source/FC/Private/Clean.cpp",
            """
            void Foo()
            {
                // Workaround: skip validation for now
            }
            """,
        )
        res = verify_replication.run_verification(self.root / "Source", baseline_path=None)
        self.assertEqual(res["status"], "FAILED")
        self.assertEqual(res["violations"][0]["rule"], "CODE_ESCAPE_HATCH_COMMENT")
        self.assertEqual(res["violations"][0]["severity"], "ERROR")

    def test_trivial_rpc_validate_is_hard_error(self):
        self.write_src(
            "Source/FC/Private/RpcActor.cpp",
            """
            bool AMyController::Server_Cast_Validate(int32 CardIndex)
            {
                return true;
            }
            """,
        )
        res = verify_replication.run_verification(self.root / "Source", baseline_path=None)
        self.assertEqual(res["status"], "FAILED")
        self.assertEqual(res["violations"][0]["rule"], "NET_SEC_TRIVIAL_RPC_VALIDATE")
        self.assertEqual(res["violations"][0]["severity"], "ERROR")


class TestVerifyGAS(TempSourceTestCase):
    def test_allows_unreplicated_damage_meta_attribute_with_accessors(self):
        self.write_src(
            "Source/FC/Public/TestAttrSet.h",
            """
            #define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)
            UCLASS()
            class FC_API UTestAttrSet : public UAttributeSet
            {
                GENERATED_BODY()
            public:
                UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "Attributes", meta = (ClampMin = "0.0"))
                FGameplayAttributeData Health;
                ATTRIBUTE_ACCESSORS(UTestAttrSet, Health)

                UPROPERTY(BlueprintReadOnly, Category = "Attributes|Meta")
                FGameplayAttributeData Damage;
                ATTRIBUTE_ACCESSORS(UTestAttrSet, Damage)

                UFUNCTION()
                virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);
            };
            """,
        )
        self.write_src(
            "Source/FC/Private/TestAttrSet.cpp",
            """
            void UTestAttrSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
            {
                Super::GetLifetimeReplicatedProps(OutLifetimeProps);
                DOREPLIFETIME_CONDITION_NOTIFY(UTestAttrSet, Health, COND_None, REPNOTIFY_Always);
            }
            void UTestAttrSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
            {
                if (!FMath::IsFinite(NewValue)) { NewValue = 0.f; return; }
            }
            void UTestAttrSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
            {
                GAMEPLAYATTRIBUTE_REPNOTIFY(UTestAttrSet, Health, OldHealth);
            }
            """,
        )
        res = verify_gas.run_gas_verification(self.root / "Source")
        self.assertEqual(res["status"], "PASS", msg=res)
        self.assertEqual(res["error_count"], 0)

    def test_still_requires_accessors_on_meta_attribute(self):
        self.write_src(
            "Source/FC/Public/BadMetaAttr.h",
            """
            #define ATTRIBUTE_ACCESSORS(ClassName, PropertyName)
            UCLASS()
            class FC_API UBadMetaAttr : public UAttributeSet
            {
                GENERATED_BODY()
            public:
                UPROPERTY(BlueprintReadOnly, NotReplicated, Category = "Attributes|Meta")
                FGameplayAttributeData Damage;
            };
            """,
        )
        res = verify_gas.run_gas_verification(self.root / "Source")
        rules = {v["rule"] for v in res["violations"]}
        self.assertIn("GAS_MISSING_ATTRIBUTE_ACCESSORS", rules)
        self.assertNotIn("GAS_ATTRIBUTE_MISSING_REPLICATED_USING", rules)


class TestAuditBandwidth(TempSourceTestCase):
    def test_catches_replicated_tarray_with_nested_meta_parens(self):
        self.write_src(
            "Source/FC/Public/Inv.h",
            """
            UCLASS()
            class FC_API UInvComp : public UActorComponent
            {
                GENERATED_BODY()
            public:
                UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inv", meta = (AllowPrivateAccess = "true"))
                TArray<int32> RawItems;
            };
            """,
        )
        res = audit_bandwidth.run_bandwidth_audit(self.root / "Source")
        self.assertEqual(res["status"], "FAIL")
        self.assertEqual(res["issues"][0]["rule"], "BW_RAW_TARRAY_REPLICATED")

    def test_raw_doreplifetime_is_error(self):
        self.write_src(
            "Source/FC/Private/Inv.cpp",
            """
            void UInvComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
            {
                Super::GetLifetimeReplicatedProps(OutLifetimeProps);
                DOREPLIFETIME(UInvComp, Gold);
            }
            """,
        )
        res = audit_bandwidth.run_bandwidth_audit(self.root / "Source")
        self.assertEqual(res["status"], "FAIL")
        self.assertEqual(res["issues"][0]["rule"], "BW_RAW_DOREPLIFETIME")


class TestRuntimeRunnersAndCircuitBreaker(unittest.TestCase):
    def test_run_tests_rejects_zero_tests_and_incomplete_tests(self):
        status_zero, reason_zero = run_tests.evaluate_test_run_status(0, 0, 0, 0, "NonExistent.")
        self.assertEqual(status_zero, "FAILED")
        self.assertIn("Zero automation tests", reason_zero)

        status_inc, _ = run_tests.evaluate_test_run_status(0, 2, 0, 1, "FC.")
        self.assertEqual(status_inc, "FAILED")

        status_ok, reason_ok = run_tests.evaluate_test_run_status(0, 5, 0, 0, "FC.")
        self.assertEqual(status_ok, "PASSED")
        self.assertEqual(reason_ok, "")

    def test_simulate_net_pie_rejects_missing_map(self):
        res = simulate_net_pie.run_simulated_network_pie(
            uproject_path=PROJECT_ROOT / "FC.uproject",
            map_name="/Game/Maps/DoesNotExist_Map",
        )
        self.assertEqual(res["status"], "FAIL")
        self.assertEqual(res["anomalies"][0]["type"], "MAP_NOT_FOUND")

    def test_simulate_net_pie_resolves_real_default_map(self):
        resolved = simulate_net_pie.resolve_map_file(PROJECT_ROOT, simulate_net_pie.DEFAULT_MAP)
        self.assertIsNotNone(resolved)
        self.assertTrue(resolved.is_file())

    def test_circuit_breaker_detects_oscillation_across_line_shifts(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            orig = pipeline.LOOP_MEMORY_FILE
            pipeline.LOOP_MEMORY_FILE = Path(tmpdir) / "loop.json"
            try:
                cb1 = pipeline.update_oscillation_circuit_breaker("static::A.cpp", False, ["A.cpp:RULE_X"])
                self.assertFalse(cb1["oscillation_detected"])
                cb2 = pipeline.update_oscillation_circuit_breaker("static::A.cpp", False, ["A.cpp:RULE_X"])
                self.assertTrue(cb2["oscillation_detected"])
                self.assertEqual(cb2["consecutive_identical_failures"], 2)

                # Separate file key must not reset A.cpp
                cb_b = pipeline.update_oscillation_circuit_breaker("static::B.cpp", True, [])
                self.assertFalse(cb_b["oscillation_detected"])
                cb3 = pipeline.update_oscillation_circuit_breaker("static::A.cpp", False, ["A.cpp:RULE_X"])
                self.assertEqual(cb3["consecutive_identical_failures"], 3)
            finally:
                pipeline.LOOP_MEMORY_FILE = orig


class TestAntigravityHooks(unittest.TestCase):
    def test_guard_verifiers_blocks_protected_files_and_allows_source(self):
        res_protected = guard_verifiers.evaluate_pre_tool_use(
            {
                "cwd": str(PROJECT_ROOT),
                "toolCall": {
                    "name": "edit_file",
                    "args": {"Path": str(PROJECT_ROOT / "Scripts" / "verify_replication.py")},
                },
            }
        )
        self.assertEqual(res_protected["decision"], "force_ask")

        res_baseline = guard_verifiers.evaluate_pre_tool_use(
            {
                "cwd": str(PROJECT_ROOT),
                "toolCall": {
                    "name": "write_file",
                    "args": {"Path": str(PROJECT_ROOT / "Scripts" / "linter_baseline.json")},
                },
            }
        )
        self.assertEqual(res_baseline["decision"], "force_ask")

        res_normal = guard_verifiers.evaluate_pre_tool_use(
            {
                "cwd": str(PROJECT_ROOT),
                "toolCall": {
                    "name": "edit_file",
                    "args": {"Path": str(PROJECT_ROOT / "Source" / "FC" / "Public" / "Map" / "FCDoorBase.h")},
                },
            }
        )
        self.assertEqual(res_normal["decision"], "allow")

    def test_stop_hook_respects_continuation_and_termination_reason(self):
        res_cont = stop_static_gate.evaluate_stop(
            {"cwd": str(PROJECT_ROOT), "continuationActive": True, "terminationReason": "NO_TOOL_CALL"}
        )
        self.assertEqual(res_cont["decision"], "allow")

        res_cancel = stop_static_gate.evaluate_stop(
            {"cwd": str(PROJECT_ROOT), "continuationActive": False, "terminationReason": "USER_CANCELED"}
        )
        self.assertEqual(res_cancel["decision"], "allow")


if __name__ == "__main__":
    unittest.main(verbosity=2)
