// SuperFAISS For Unreal 3.4 -- §9.5a's mid-selection refusal (the 3.4 drift-and-diversity plan
//  §12 dims 4/5/10/11, Option A).
//
// Every test in this file drives Gate 6b's diversity seams (`RunQueryForTest`,
// `SetDiversityLambdaForTest`, `SetQueryKForTest`, `GetLastMMRSelectionForTest`, and
// `SetDiversitySegmentOverrideForTest`) and is guarded by `SUPERFAISS_GATE6B_BUILT`.
//
// Scope: BOTH §9.5a triggers, at both lambda = 1 and lambda < 1 (§9.5a: the redundancy
// evaluation is unconditional on lambda, so the refusal note is the only signal that tells a
// fallback from §9.4's identity). The Metric::Cosine weighted-zero-norm trigger drives through
// SetChannelWeightForTest; the segment-list InvalidArgument trigger through
// SetDiversitySegmentOverrideForTest.
//
// Construction: SelectDiverseMMR first evaluates redundancy at step 1, so
// every cell runs K = 2 on a bank with two candidates once the queried row is excluded.
// Each cell asserts the whole §9.5a disposition: the refusal state, the shared
// note, the fallback order (the pool's own top-K relevance order -- never the aborted call's
// partial buffer), each entry's pool relevance score, and redundancy 0 throughout. The pool
// scores are derived independently of the panel (probe §8/§9,
// a standalone derivation probe).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SSuperFAISSBankInspector.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "Fixtures/SuperFAISSDiversityFixtures.h"
#include "Fixtures/SuperFAISSDriftDiversityTestContracts.h"
#include "superfaiss/types.h"

using namespace SuperFAISSDriftDiversityOracle;

#if SUPERFAISS_GATE6B_BUILT

namespace
{
	using namespace SuperFAISSDriftDiversityOracle::MidSelectionRefusalFixtures;

	USuperFAISSVectorBank* BakeWeightedZeroNormBank(FAutomationTestBase& Test)
	{
		TArray<float> Rows;
		BuildWeightedZeroNormRows(Rows);
		return BakeMetricInt8AssetTwoChannels(Test, Rows, kRows, kDims, kChanLen, ESuperFAISSBankMetric::Cosine,
			TEXT("MidSelectionRefusal"));
	}

	// The segment-list InvalidArgument trigger is a caller-CONTRACT violation on a
	// locally-constructed segment list (§9.5a: "a plugin-internal precondition, not a property
	// of the user's bank"), so the bank itself is an ordinary well-formed channelled bank.
	USuperFAISSVectorBank* BakeOrdinaryChannelledBank(FAutomationTestBase& Test)
	{
		TArray<float> Rows;
		BuildOrdinaryChannelledRows(Rows);
		return BakeMetricInt8AssetTwoChannels(Test, Rows, kRows, kDims, kChanLen, ESuperFAISSBankMetric::Cosine,
			TEXT("MidSelectionRefusalSegmentTrigger"));
	}

	// A malformed segment list (unsorted -- descending offset), injected via the override seam
	// so it reaches SelectDiverseMMR's redundancy call.
	TArray<superfaiss::QuerySegment> MakeMalformedSegmentList()
	{
		TArray<superfaiss::QuerySegment> Segments;
		Segments.Add(superfaiss::QuerySegment{16, 16, 1.0f});
		Segments.Add(superfaiss::QuerySegment{0, 16, 1.0f});
		return Segments;
	}

	// §9.5a's full disposition for one refused query.
	void AssertFallback(FAutomationTestBase& Test, const FString& Label, const FSuperFAISSMMRSelectionForTest& Result,
		const float* ExpectedPoolRelevance)
	{
		AssertExactValue(Test, Label + TEXT(": mid-selection refusal fires"), Result.bMidSelectionRefusal, true);
		AssertExactValue(Test, Label + TEXT(": not a whole-panel refusal"), Result.bWholePanelRefusal, false);
		AssertExactValue(Test, Label + TEXT(": the shared note renders (not §9.4's identity message)"),
			Result.MidSelectionRefusalNoteText, FString(kExpectedMidSelectionNote));
		AssertExactValue(Test, Label + TEXT(": fallback selection count"), Result.SelectedIndices.Num(), kCandidates);
		AssertExactValue(Test, Label + TEXT(": relevance count"), Result.Relevance.Num(), kCandidates);
		AssertExactValue(Test, Label + TEXT(": redundancy count"), Result.Redundancy.Num(), kCandidates);
		if (Result.SelectedIndices.Num() != kCandidates || Result.Relevance.Num() != kCandidates ||
			Result.Redundancy.Num() != kCandidates)
		{
			return;
		}
		for (int32 i = 0; i < kCandidates; ++i)
		{
			AssertExactValue(Test, FString::Printf(TEXT("%s: fallback order step%d is the pool's own relevance order"), *Label, i),
				Result.SelectedIndices[i], kIdentityOrder[i]);
			AssertExactValue(Test, FString::Printf(TEXT("%s: relevance step%d is the pool score"), *Label, i),
				Result.Relevance[i], ExpectedPoolRelevance[i]);
			AssertExactValue(Test, FString::Printf(TEXT("%s: redundancy step%d is 0 (nothing computed)"), *Label, i),
				Result.Redundancy[i], kZeroRedundancy[i]);
		}
	}
}

// ===========================================================================
// dim 5/dim 10/dim 11, lambda < 1: the weighted-zero-norm trigger fires the fallback.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityMidSelectionRefusalLambdaLessThan1Test,
	"SuperFAISS.D.DiversityMidSelectionRefusal.LambdaLessThan1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityMidSelectionRefusalLambdaLessThan1Test::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Bank = BakeWeightedZeroNormBank(*this);
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	// chanA weighted to exactly 0: row 2 carries content only on chanA, so its weighted
	// self-norm is exactly 0 and scoring it at step 1 returns ZeroNormQuery.
	Inspector->SetChannelWeightForTest(0, 0.0f); // chanA
	Inspector->SetChannelWeightForTest(1, 1.0f); // chanB
	Inspector->SetDiversityLambdaForTest(0.5f);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);

	AssertFallback(*this, TEXT("weighted-zero-norm lambda<1"), Inspector->GetLastMMRSelectionForTest(),
		kWeightedZeroNormFallbackRelevance);
	return true;
}

// ===========================================================================
// dim 5/dim 10/dim 11, lambda == 1.0 exactly: the identical trigger at §9.4's own slider
// position -- the refusal note is shown INSTEAD of the identity message. A build that gates
// the identity message on row order alone passes every ordinary case and fails here, since
// the fallback's order IS the identity order.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityMidSelectionRefusalLambdaEquals1Test,
	"SuperFAISS.D.DiversityMidSelectionRefusal.LambdaEquals1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityMidSelectionRefusalLambdaEquals1Test::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Bank = BakeWeightedZeroNormBank(*this);
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetChannelWeightForTest(0, 0.0f);
	Inspector->SetChannelWeightForTest(1, 1.0f);
	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);

	AssertFallback(*this, TEXT("weighted-zero-norm lambda=1"), Inspector->GetLastMMRSelectionForTest(),
		kWeightedZeroNormFallbackRelevance);
	return true;
}

// ===========================================================================
// dim 5/dim 10/dim 11, the segment-list InvalidArgument trigger, lambda < 1: the identical
// fallback and the identical note as the weighted-zero-norm trigger.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversitySegmentOverrideRefusalLambdaLessThan1Test,
	"SuperFAISS.D.DiversityMidSelectionRefusal.SegmentOverrideTrigger.LambdaLessThan1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversitySegmentOverrideRefusalLambdaLessThan1Test::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Bank = BakeOrdinaryChannelledBank(*this);
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversitySegmentOverrideForTest(MakeMalformedSegmentList());
	Inspector->SetDiversityLambdaForTest(0.5f);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);

	AssertFallback(*this, TEXT("segment-override lambda<1"), Inspector->GetLastMMRSelectionForTest(),
		kOrdinaryFallbackRelevance);
	return true;
}

// ===========================================================================
// dim 5/dim 10/dim 11, the segment-list InvalidArgument trigger, lambda == 1.0 exactly.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversitySegmentOverrideRefusalLambdaEquals1Test,
	"SuperFAISS.D.DiversityMidSelectionRefusal.SegmentOverrideTrigger.LambdaEquals1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversitySegmentOverrideRefusalLambdaEquals1Test::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Bank = BakeOrdinaryChannelledBank(*this);
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversitySegmentOverrideForTest(MakeMalformedSegmentList());
	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);

	AssertFallback(*this, TEXT("segment-override lambda=1"), Inspector->GetLastMMRSelectionForTest(),
		kOrdinaryFallbackRelevance);
	return true;
}

// ===========================================================================
// dim 5/dim 10: the override is CONSUMED, one-shot-per-query -- the query
// immediately following the overridden one, with no override re-armed, runs the kernel on
// the bank's own well-formed segment list and displays its values.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversitySegmentOverrideIsOneShotTest,
	"SuperFAISS.D.DiversityMidSelectionRefusal.SegmentOverrideTrigger.OneShotConsumed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversitySegmentOverrideIsOneShotTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Bank = BakeOrdinaryChannelledBank(*this);
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversitySegmentOverrideForTest(MakeMalformedSegmentList());
	Inspector->SetDiversityLambdaForTest(0.5f);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertExactValue(*this, TEXT("(setup) first query refuses"), Inspector->GetLastMMRSelectionForTest().bMidSelectionRefusal, true);

	// No override re-armed.
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("second query, override consumed"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kOrdinaryLambda05Order, kOrdinaryLambda05Relevance, kOrdinaryLambda05Redundancy);
	return true;
}

#endif // SUPERFAISS_GATE6B_BUILT

#endif // WITH_DEV_AUTOMATION_TESTS
