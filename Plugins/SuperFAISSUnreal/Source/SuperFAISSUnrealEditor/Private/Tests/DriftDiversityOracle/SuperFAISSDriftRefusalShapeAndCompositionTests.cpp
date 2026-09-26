// SuperFAISS For Unreal 3.4 -- drift's refusal, shape, lifetime, and composition cells
// (the 3.4 drift-and-diversity plan §12 dims 1/2/3a/4/5/6/8/11,
// drift halves). 2026-08-08 completion pass.
//
// COMPILE STATUS: does not compile yet -- awaits Gate 6a's `GetDriftResultForTest()`,
// `DriftChunksProcessedForTest`, and cache-invalidation wiring. Authored against the exact
// seam signatures `Fixtures/SuperFAISSDriftDiversityTestContracts.h` specifies.
//
// Every geometry below reuses the one-hot, magnitude-10/13 construction Fixture A's own
// executed record already proves quantizes losslessly under Int8 (the test-design record
//  §1.2) -- no new
// quantization-exactness claim is introduced.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SSuperFAISSBankInspector.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "Fixtures/SuperFAISSDriftFixtures.h"
#include "Fixtures/SuperFAISSDriftDiversityTestContracts.h"

using namespace SuperFAISSDriftDiversityOracle;

namespace
{
	// A minimal, well-formed 2-row L2/Int8 bank (chanA/chanB, 32 dims) -- reused by every
	// cell below that just needs "a normal bank," not a specific fixture.
	USuperFAISSVectorBank* BakeOrdinaryBank(FAutomationTestBase& Test, const FString& DebugName)
	{
		TArray<float> Rows;
		BuildBaselineRows(Rows); // the 22-row scheme, Fixtures/SuperFAISSDriftFixtures.h
		return BakeL2Int8Asset(Test, Rows, kBaselineRowCount, DebugName);
	}
}

// ===========================================================================
// dim 2/dim 5/dim 11: quantization-mismatch refusal (§8.4) -- fires exactly when either
// bank is Float32, with the exact stated text; and does NOT fire when both are Int8
// (guard vitality's other side -- Fixture A's own ordinary path already proves this
// negative in `SuperFAISSDriftOracleTests.cpp`, cross-referenced here rather than
// duplicated).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftQuantizationRefusalTest,
	"SuperFAISS.D.DriftRefusal.QuantizationMismatch",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftQuantizationRefusalTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildBaselineRows(Rows);
	const TArray<FName> ChannelNames = {TEXT("chanA"), TEXT("chanB")};
	const TArray<int32> ChannelOffsets = {0, kChanLen};
	const TArray<int32> ChannelLengths = {kChanLen, kChanLen};

	USuperFAISSVectorBank* CurrentFloat32 = NewObject<USuperFAISSVectorBank>();
	FString Error1;
	const bool bOk1 = CurrentFloat32->InitFromSource(Rows, kBaselineRowCount, kDriftDims,
		ESuperFAISSBankMetric::L2, ESuperFAISSBankQuantization::Float32, {}, TEXT("QuantRefusal-current-f32"),
		Error1, ChannelNames, ChannelOffsets, ChannelLengths);
	AssertExactValue(*this, TEXT("(setup) Float32 current bakes"), bOk1, true);
	USuperFAISSVectorBank* BaselineInt8 = BakeL2Int8Asset(*this, Rows, kBaselineRowCount, TEXT("QuantRefusal-baseline-int8"));
	if (!bOk1 || BaselineInt8 == nullptr) { return true; }

	Inspector->SetBankForTest(CurrentFloat32);
	Inspector->SetComparisonBankForTest(BaselineInt8);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("quantization refusal fires"), Result.bQuantizationRefusal, true);
	AssertExactValue(*this, TEXT("quantization refusal text"), Result.QuantizationRefusalText,
		FString(TEXT("drift requires both banks quantized Int8; current is Float32")));
	AssertExactValue(*this, TEXT("no operator ran (chunk counter at zero)"), Inspector->DriftChunksProcessedForTest, 0);
	return true;
}

// ===========================================================================
// dim 2/dim 5/dim 11: Metric::Dot whole-panel refusal (§8.4) -- fires exactly
// on Metric::Dot, with the exact stated text, and no operator runs (G-25). The negative
// side (Cosine/L2 do not refuse) is already proven by every other test in this directory
// that builds a Cosine or L2 bank and gets a normal result.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftMetricDotRefusalTest,
	"SuperFAISS.D.DriftRefusal.MetricDot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftMetricDotRefusalTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildBaselineRows(Rows);
	const TArray<FName> ChannelNames = {TEXT("chanA"), TEXT("chanB")};
	const TArray<int32> ChannelOffsets = {0, kChanLen};
	const TArray<int32> ChannelLengths = {kChanLen, kChanLen};

	USuperFAISSVectorBank* CurrentDot = NewObject<USuperFAISSVectorBank>();
	FString Error;
	const bool bOk = CurrentDot->InitFromSource(Rows, kBaselineRowCount, kDriftDims,
		ESuperFAISSBankMetric::Dot, ESuperFAISSBankQuantization::Int8, {}, TEXT("DotRefusal-current"),
		Error, ChannelNames, ChannelOffsets, ChannelLengths);
	AssertExactValue(*this, TEXT("(setup) Dot current bakes"), bOk, true);
	USuperFAISSVectorBank* BaselineInt8 = BakeL2Int8Asset(*this, Rows, kBaselineRowCount, TEXT("DotRefusal-baseline"));
	if (!bOk || BaselineInt8 == nullptr) { return true; }

	Inspector->SetBankForTest(CurrentDot);
	Inspector->SetComparisonBankForTest(BaselineInt8);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("Dot refusal fires"), Result.bMetricDotRefusal, true);
	AssertExactValue(*this, TEXT("Dot refusal text"), Result.MetricDotRefusalText,
		FString(TEXT("drift is not available for Metric::Dot banks -- a raw dot product has no stable, "
			"bank-independent notion of distance to measure movement against.")));
	AssertExactValue(*this, TEXT("no operator ran (chunk counter at zero)"), Inspector->DriftChunksProcessedForTest, 0);
	return true;
}

// ===========================================================================
// dim 2/dim 5/dim 11: the zero-denominator refusal (§8.3.3) -- fires exactly when the
// composition's denominator (Spread(current)) is exactly 0, on a single-live-row current
// bank, with the exact stated text; the non-triggering side is already proven by every
// oracle test in this directory (every fixture there has a genuinely nonzero spread).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftZeroDenominatorRefusalTest,
	"SuperFAISS.D.DriftRefusal.ZeroDenominator",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftZeroDenominatorRefusalTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	// A single-live-row current bank: Spread(current) is structurally 0 (a lone row IS its
	// own centroid, analytics.cpp:363-381).
	TArray<float> SingleRow;
	SingleRow.SetNumZeroed(kDriftDims);
	SingleRow[0] = 10.0f; SingleRow[kChanLen] = 10.0f;
	USuperFAISSVectorBank* CurrentSingle = BakeL2Int8Asset(*this, SingleRow, 1, TEXT("ZeroDenom-current"));
	USuperFAISSVectorBank* Baseline = BakeOrdinaryBank(*this, TEXT("ZeroDenom-baseline"));
	if (CurrentSingle == nullptr || Baseline == nullptr) { return true; }

	Inspector->SetBankForTest(CurrentSingle);
	Inspector->SetComparisonBankForTest(Baseline);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("headline zero-denominator refusal fires"), Result.bHeadlineZeroDenominatorRefusal, true);
	AssertExactValue(*this, TEXT("headline raw Spread(current) is exactly 0"), Result.SpreadCurrent, 0.0f);
	return true;
}

// ===========================================================================
// dim 2/dim 5/dim 11: the ZeroNormQuery refusal (§8.3.4) -- fires exactly when a
// Metric::Cosine bank's whole-row selection pools to a zero-norm centroid (two exactly
// opposite rows). The refusal's own display-reset requirement (never the previous run's
// stale value) is proven by running a normal compute first, then the refusing one, and
// confirming the numeric fields are not left at the prior run's values.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftZeroNormQueryRefusalTest,
	"SuperFAISS.D.DriftRefusal.ZeroNormQuery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftZeroNormQueryRefusalTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	// A Cosine/Int8 bank of two exactly-opposite one-hot-ish rows: row0 = +10 on dim0,
	// row1 = -10 on dim0. Their sum is the zero vector -- pooling both into one centroid
	// (MakeCentroidCrossDevice) produces a net zero-norm centroid.
	TArray<float> OppositeRows;
	OppositeRows.SetNumZeroed(2 * kDriftDims);
	OppositeRows[0] = 10.0f; OppositeRows[kChanLen] = 10.0f;
	OppositeRows[kDriftDims + 0] = -10.0f; OppositeRows[kDriftDims + kChanLen] = -10.0f;
	USuperFAISSVectorBank* CurrentZeroNorm = BakeCosineInt8Asset(*this, OppositeRows, 2, TEXT("ZeroNormQuery-current"));

	TArray<float> BaselineRows;
	BuildBaselineRows(BaselineRows);
	const TArray<FName> ChannelNames = {TEXT("chanA"), TEXT("chanB")};
	const TArray<int32> ChannelOffsets = {0, kChanLen};
	const TArray<int32> ChannelLengths = {kChanLen, kChanLen};
	USuperFAISSVectorBank* BaselineOrdinary = NewObject<USuperFAISSVectorBank>();
	FString Error;
	const bool bBaselineOk = BaselineOrdinary->InitFromSource(BaselineRows, kBaselineRowCount, kDriftDims,
		ESuperFAISSBankMetric::Cosine, ESuperFAISSBankQuantization::Int8, {}, TEXT("ZeroNormQuery-baseline"),
		Error, ChannelNames, ChannelOffsets, ChannelLengths);
	AssertExactValue(*this, TEXT("(setup) Cosine baseline bakes"), bBaselineOk, true);

	// The stale value must be a real, NONZERO prior movement: a self-comparison
	// computes exactly 0.0f by §8.7, which is also the refusal's own reset default, so
	// `Movement == StaleMovement` would hold whether or not the panel resets. The prior compute
	// therefore runs between two DISTINCT Cosine banks -- Fixture C's rotated population as
	// current, the 22-row scheme as baseline -- before the current side is switched to the
	// antipodal-row construction.
	TArray<float> DistinctRows;
	FixtureC::BuildCurrentRows(DistinctRows);
	USuperFAISSVectorBank* CurrentDistinct = BakeCosineInt8Asset(*this, DistinctRows, FixtureC::kRowCount,
		TEXT("ZeroNormQuery-stale-current"));
	if (CurrentZeroNorm == nullptr || CurrentDistinct == nullptr || !bBaselineOk) { return true; }

	Inspector->SetBankForTest(CurrentDistinct);
	Inspector->SetComparisonBankForTest(BaselineOrdinary);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));
	const FSuperFAISSDriftResultForTest NormalResult = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("(setup) ordinary compute does not refuse"), NormalResult.bHeadlineZeroNormQueryRefusal, false);
	const float StaleMovement = NormalResult.Movement;
	AssertExactValue(*this, TEXT("(setup) stale movement is nonzero, so a missing reset is observable"),
		StaleMovement != 0.0f, true);

	Inspector->SetBankForTest(CurrentZeroNorm);
	// ComparisonArchive unchanged -- exercising the refusal on the CURRENT side.
	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("ZeroNormQuery refusal fires"), Result.bHeadlineZeroNormQueryRefusal, true);
	AssertExactValue(*this, TEXT("display state reset, not the prior run's stale movement"),
		Result.Movement == StaleMovement, false);
	return true;
}

// ===========================================================================
// dim 4/dim 8: self-comparison (§8.7) -- when primary and comparison resolve to the
// SAME underlying bank object, the identity block gains "Comparing bank to itself," and
// the movement number still renders (well-defined, exactly 0 for an identical bank).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftSelfComparisonTest,
	"SuperFAISS.D.DriftShape.SelfComparison",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftSelfComparisonTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Bank = BakeOrdinaryBank(*this, TEXT("SelfComparison"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetComparisonBankForTest(Bank); // the SAME object on both sides
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("self-comparison disclosed"), Result.Identity.bSelfComparison, true);
	AssertExactValue(*this, TEXT("movement is exactly 0 (identical bank)"), Result.Movement, 0.0f);
	return true;
}

// ===========================================================================
// dim 4: a live row zero-content on exactly one named channel (chanB), the bank's other
// rows and other channels nonzero (§8.3.3's Watch note) -- SpreadCrossDeviceChannel
// returns a finite, defined, nonzero value for that channel, neither refusal firing.
// Two rows on chanB: row0 = 10 (nonzero), row1 = 0 (zero-content). Their mean centroid on
// chanB is 5; each row's own distance-to-centroid on chanB is (10-5)^2 = 25 and
// (0-5)^2 = 25 -- mean = 25 exactly, a hand-derivable, nonzero closed form.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftZeroContentChannelTest,
	"SuperFAISS.D.DriftShape.ZeroContentChannelDeflatesNotZeroes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftZeroContentChannelTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	Rows.SetNumZeroed(2 * kDriftDims);
	// row0: chanA=10 (dim0), chanB=10 (dim kChanLen). row1: chanA=10 (dim0, so chanA is
	// NOT degenerate), chanB=0 (zero-content on chanB only).
	Rows[0] = 10.0f; Rows[kChanLen] = 10.0f;
	Rows[kDriftDims + 0] = 10.0f; // Rows[kDriftDims + kChanLen] left 0 -- zero-content chanB
	USuperFAISSVectorBank* Bank = BakeL2Int8Asset(*this, Rows, 2, TEXT("ZeroContentChannel"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetComparisonBankForTest(Bank);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("chanB row present"), Result.Channels.Num(), 2);
	if (Result.Channels.Num() == 2)
	{
		// Channels[1] = chanB, per this file's own construction order.
		AssertExactValue(*this, TEXT("chanB: neither refusal fires"), Result.Channels[1].bZeroDenominatorRefusal, false);
		AssertExactValue(*this, TEXT("chanB: neither refusal fires (ZeroNormQuery)"), Result.Channels[1].bZeroNormQueryRefusal, false);
		AssertExactValue(*this, TEXT("chanB: spread deflated but nonzero, hand-derived"), Result.Channels[1].SpreadCurrent, 25.0f);
	}
	return true;
}

// ===========================================================================
// dim 6: drift's four operators are deterministic -- the panel's displayed values are
// bit-for-bit reproducible across repeated computes with no bank change.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftRepeatComputeDeterminismTest,
	"SuperFAISS.D.DriftShape.RepeatComputeDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftRepeatComputeDeterminismTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Current = BakeOrdinaryBank(*this, TEXT("Determinism-current"));
	USuperFAISSVectorBank* Baseline = BakeOrdinaryBank(*this, TEXT("Determinism-baseline"));
	if (Current == nullptr || Baseline == nullptr) { return true; }

	Inspector->SetBankForTest(Current);
	Inspector->SetComparisonBankForTest(Baseline);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest First = Inspector->GetDriftResultForTest();
	const FSuperFAISSDriftResultForTest Second = Inspector->GetDriftResultForTest();

	AssertExactValue(*this, TEXT("repeat compute: Movement bit-identical"), Second.Movement, First.Movement);
	AssertExactValue(*this, TEXT("repeat compute: headline ratio bit-identical"), Second.HeadlineRatio, First.HeadlineRatio);
	AssertExactValue(*this, TEXT("repeat compute: MeanNN bit-identical"), Second.MeanNN, First.MeanNN);
	AssertExactValue(*this, TEXT("repeat compute: MaxNN bit-identical"), Second.MaxNN, First.MaxNN);
	return true;
}

// ===========================================================================
// dim 8: the shared comparison slot (§7) crossed with both features that read it -- a
// Correspondence partner loaded, then a Drift compute against the same bank, reads the
// same, current comparison bank -- no stale label. Proven via GetComparisonSource()'s own
// already-shipped identity (Gate 0a) plus the drift identity block's own consistency.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftSharedSlotCrossFeatureIdentityTest,
	"SuperFAISS.D.DriftComposition.SharedSlotCrossFeatureIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftSharedSlotCrossFeatureIdentityTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	USuperFAISSVectorBank* Primary = BakeOrdinaryBank(*this, TEXT("SharedSlot-primary"));
	USuperFAISSVectorBank* CompA = BakeOrdinaryBank(*this, TEXT("SharedSlot-compA"));
	USuperFAISSVectorBank* CompB = BakeOrdinaryBank(*this, TEXT("SharedSlot-compB"));
	if (Primary == nullptr || CompA == nullptr || CompB == nullptr) { return true; }

	Inspector->SetBankForTest(Primary);
	Inspector->SetComparisonBankForTest(CompA);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));
	const FSuperFAISSDriftResultForTest ResultA = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("comparison-A: identity comparison name matches CompA"),
		ResultA.Identity.ComparisonDisplayName, CompA->GetName());

	// Switch the comparison bank (the same slot both Drift and Correspondence read,
	// §7's own GetComparisonSource()) and confirm Drift's own identity block follows it,
	// never a label stale from CompA.
	Inspector->SetComparisonBankForTest(CompB);
	const FSuperFAISSDriftResultForTest ResultB = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("comparison-B: identity comparison name matches CompB"),
		ResultB.Identity.ComparisonDisplayName, CompB->GetName());
	return true;
}

// ===========================================================================
// dim 8: the scope-combo crossing (§8.2, Option A) -- scoping the analysis
// combo to one channel scopes Drift's result to that same channel identically to how
// Structure/Novelty already respond: a chanA-scoped compute reports chanA's own movement,
// not the whole-row one.
//
// Fixture B (current) against the 22-row baseline, so the three quantities differ: whole
// row 1156.00293, chanA 578.174744, chanB 576.485168 (FixtureB::kRaw*, re-derived by the
// offline probe (part 3) with CentroidDistanceCrossDevice / ...Channel). A build that ignores
// the combo shows the whole-row value under the chanA scope and fails; a build that scopes to
// the wrong channel shows chanB's and fails. (Two identical banks, the fixture this cell had
// before, make every value 0, so no build could fail it.)
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftScopeComboReadsCombTest,
	"SuperFAISS.D.DriftComposition.ScopeComboReadsCombo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftScopeComboReadsCombTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	TArray<float> CurrentRows;
	FixtureB::BuildCurrentRows(CurrentRows);
	USuperFAISSVectorBank* Current = BakeL2Int8Asset(*this, CurrentRows, kBaselineRowCount, TEXT("ScopeCombo-current"));
	USuperFAISSVectorBank* Baseline = BakeOrdinaryBank(*this, TEXT("ScopeCombo-baseline"));
	if (Current == nullptr || Baseline == nullptr) { return true; }

	Inspector->SetBankForTest(Current);
	Inspector->SetComparisonBankForTest(Baseline);

	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));
	const FSuperFAISSDriftResultForTest WholeRow = Inspector->GetDriftResultForTest();

	Inspector->SetAnalysisScopeForTest(TEXT("chanA"));
	const FSuperFAISSDriftResultForTest ChanAScoped = Inspector->GetDriftResultForTest();

	AssertExactValue(*this, TEXT("whole-row headline movement"), WholeRow.Movement, FixtureB::kRawMovement);
	AssertExactValue(*this, TEXT("whole-row compute reports 2 channels"), WholeRow.Channels.Num(), 2);
	if (WholeRow.Channels.Num() == 2)
	{
		AssertExactValue(*this, TEXT("whole-row compute: chanA movement"), WholeRow.Channels[0].Movement, FixtureB::kRawChanAMovement);
		AssertExactValue(*this, TEXT("whole-row compute: chanB movement"), WholeRow.Channels[1].Movement, FixtureB::kRawChanBMovement);
	}
	AssertExactValue(*this, TEXT("chanA-scoped headline movement is chanA's movement"), ChanAScoped.Movement, FixtureB::kRawChanAMovement);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
