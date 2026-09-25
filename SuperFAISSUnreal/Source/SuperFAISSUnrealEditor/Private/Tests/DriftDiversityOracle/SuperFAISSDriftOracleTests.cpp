// SuperFAISS For Unreal 3.4 -- the drift oracle (the 3.4 drift-and-diversity plan
//  §11.1, §12 dims 4/7/10). Gate 1 red suite,
// authored per §5's Gate 1 row and the test author's own test-design record
// (the test-design record).
//
// COMPILE STATUS: does not compile yet. Drives the real panel through
// `SSuperFAISSBankInspector::GetDriftResultForTest()` (§8.9), which does not exist until
// Gate 6a builds the drift panel -- its required return shape is specified at
// `Fixtures/SuperFAISSDriftDiversityTestContracts.h` (`FSuperFAISSDriftResultForTest`).
// `SetBankForTest`/`SetComparisonBankForTest`/`SetAnalysisScopeForTest` already exist
// (Gate 0a). red-unimplemented at the COMPILE stage, not merely the assertion stage --
// exactly the shape `StandardsDocument.md` §5.4/red-first TDD predicts for a compiled
// language: the missing symbol IS this cell's "red," until Gate 6a supplies it.
//
// Every comparison below routes through AssertExactValue (D-INSP-64) except Fixture A's
// four raw-operator lines, which use AssertWithinQuantizationTolerance -- §11's one named
// exception, Fixture A's own pre-existing, separately-governed int8-quantization bound.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SSuperFAISSBankInspector.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "Fixtures/SuperFAISSDriftFixtures.h"
#include "Fixtures/SuperFAISSDriftDiversityTestContracts.h"

using namespace SuperFAISSDriftDiversityOracle;

// ===========================================================================
// Fixture A -- dim 10(a): the four raw operator values equal their closed-form expected
// values within the established int8 error bound (§11.1's Fixture A paragraph). This is
// deliberately NOT the composed-ratio assertion (§11.1's own scoping: Fixture A's three
// compositions barely diverge, so it stays scoped to the raw operator pin it was always
// tight enough for).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftFixtureARawOperatorsTest,
	"SuperFAISS.D.DriftOracle.FixtureA.RawOperators",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftFixtureARawOperatorsTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> BaselineRows;
	BuildBaselineRows(BaselineRows);
	USuperFAISSVectorBank* BaselineBank = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("FixtureA-baseline"));
	TArray<float> CurrentRows;
	FixtureA::BuildCurrentRows(CurrentRows);
	USuperFAISSVectorBank* CurrentBank = BakeL2Int8Asset(*this, CurrentRows, kBaselineRowCount, TEXT("FixtureA-current"));
	if (BaselineBank == nullptr || CurrentBank == nullptr) { return true; }

	Inspector->SetBankForTest(CurrentBank);
	Inspector->SetComparisonBankForTest(BaselineBank);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();

	AssertWithinQuantizationTolerance(*this, TEXT("Fixture A: Movement"),
		Result.Movement, FixtureA::kExpectedMovement, FixtureA::kWholeRowBound);
	AssertWithinQuantizationTolerance(*this, TEXT("Fixture A: Spread(current)"),
		Result.SpreadCurrent, FixtureA::kExpectedSpreadCurrent, FixtureA::kWholeRowBound);
	AssertWithinQuantizationTolerance(*this, TEXT("Fixture A: Spread(baseline)"),
		Result.SpreadBaseline, FixtureA::kExpectedSpreadBaseline, FixtureA::kWholeRowBound);
	AssertWithinQuantizationTolerance(*this, TEXT("Fixture A: MeanNN (typical)"),
		Result.MeanNN, FixtureA::kExpectedMeanNN, FixtureA::kNNBound);
	AssertWithinQuantizationTolerance(*this, TEXT("Fixture A: MaxNN (worst-case)"),
		Result.MaxNN, FixtureA::kExpectedMaxNN, FixtureA::kNNBound);

	AssertExactValue(*this, TEXT("Fixture A: per-channel row count"), Result.Channels.Num(), 2);
	if (Result.Channels.Num() == 2)
	{
		AssertWithinQuantizationTolerance(*this, TEXT("Fixture A: chanA movement"),
			Result.Channels[0].Movement, FixtureA::kExpectedChanAMovement, FixtureA::kWholeRowBound);
		AssertWithinQuantizationTolerance(*this, TEXT("Fixture A: chanB movement"),
			Result.Channels[1].Movement, FixtureA::kExpectedChanBMovement, FixtureA::kWholeRowBound);
	}
	return true;
}

// ===========================================================================
// Fixture B -- dim 10(b), the crux: the panel's COMPOSED, DISPLAYED ratios (headline,
// worst-case, typical, each per-channel row) equal the hand-derived composed values under
// §8.3.1's Metric::L2 composition (sqrt(Movement)/sqrt(Spread(current))). A build
// composing Movement/Spread(baseline), or omitting the sqrt, fails this cell -- the exact
// mutation-provability §12 dim 7 requires and Gate 0c already proved by execution against
// these same numbers (mutation-execution ledger rows 99-124).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftFixtureBComposedRatioTest,
	"SuperFAISS.D.DriftOracle.FixtureB.ComposedRatio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftFixtureBComposedRatioTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> BaselineRows;
	BuildBaselineRows(BaselineRows);
	USuperFAISSVectorBank* BaselineBank = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("FixtureB-baseline"));
	TArray<float> CurrentRows;
	FixtureB::BuildCurrentRows(CurrentRows);
	USuperFAISSVectorBank* CurrentBank = BakeL2Int8Asset(*this, CurrentRows, kBaselineRowCount, TEXT("FixtureB-current"));
	if (BaselineBank == nullptr || CurrentBank == nullptr) { return true; }

	Inspector->SetBankForTest(CurrentBank);
	Inspector->SetComparisonBankForTest(BaselineBank);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();

	AssertExactValue(*this, TEXT("Fixture B: headline ratio"), Result.HeadlineRatio, FixtureB::kExpectedHeadlineRatio);
	AssertExactValue(*this, TEXT("Fixture B: worst-case ratio"), Result.WorstCaseRatio, FixtureB::kExpectedWorstCaseRatio);
	AssertExactValue(*this, TEXT("Fixture B: typical ratio"), Result.TypicalRatio, FixtureB::kExpectedTypicalRatio);
	AssertExactValue(*this, TEXT("Fixture B: per-channel row count"), Result.Channels.Num(), 2);
	if (Result.Channels.Num() == 2)
	{
		AssertExactValue(*this, TEXT("Fixture B: chanA ratio"), Result.Channels[0].ComposedRatio, FixtureB::kExpectedChanARatio);
		AssertExactValue(*this, TEXT("Fixture B: chanB ratio"), Result.Channels[1].ComposedRatio, FixtureB::kExpectedChanBRatio);
	}

	// dim 6/dim 11's "Reduce::Mean pin" cell rides on the same raw values this fixture's
	// own composed lines are built from -- asserted here too since GetDriftResultForTest()
	// exposes the raw operator set alongside the composed one (§8.9).
	AssertExactValue(*this, TEXT("Fixture B: raw Movement"), Result.Movement, FixtureB::kRawMovement);
	AssertExactValue(*this, TEXT("Fixture B: raw Spread(current)"), Result.SpreadCurrent, FixtureB::kRawSpreadCurrentWhole);
	AssertExactValue(*this, TEXT("Fixture B: raw Spread(baseline)"), Result.SpreadBaseline, FixtureB::kRawSpreadBaselineWhole);
	return true;
}

// ===========================================================================
// Fixture C -- dim 10(c): the Cosine, no-sqrt branch (§8.3.1: Movement/Spread(current)),
// closing the metric-branch gap Fixture A/B's Metric::L2 pin leaves open (dim 4's
// {Metric::L2, Metric::Cosine} shape-matrix requirement). A build applying the L2 sqrt
// transform to a Cosine bank fails this cell (mutation-execution ledger rows 125-134).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftFixtureCComposedRatioTest,
	"SuperFAISS.D.DriftOracle.FixtureC.ComposedRatio",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftFixtureCComposedRatioTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> BaselineRows;
	FixtureC::BuildBaselineRows(BaselineRows);
	USuperFAISSVectorBank* BaselineBank = BakeCosineInt8Asset(*this, BaselineRows, FixtureC::kRowCount, TEXT("FixtureC-baseline"));
	TArray<float> CurrentRows;
	FixtureC::BuildCurrentRows(CurrentRows);
	USuperFAISSVectorBank* CurrentBank = BakeCosineInt8Asset(*this, CurrentRows, FixtureC::kRowCount, TEXT("FixtureC-current"));
	if (BaselineBank == nullptr || CurrentBank == nullptr) { return true; }

	Inspector->SetBankForTest(CurrentBank);
	Inspector->SetComparisonBankForTest(BaselineBank);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();

	AssertExactValue(*this, TEXT("Fixture C: headline ratio"), Result.HeadlineRatio, FixtureC::kExpectedHeadlineRatio);
	AssertExactValue(*this, TEXT("Fixture C: worst-case ratio"), Result.WorstCaseRatio, FixtureC::kExpectedWorstCaseRatio);
	AssertExactValue(*this, TEXT("Fixture C: typical ratio"), Result.TypicalRatio, FixtureC::kExpectedTypicalRatio);
	AssertExactValue(*this, TEXT("Fixture C: per-channel row count"), Result.Channels.Num(), 2);
	if (Result.Channels.Num() == 2)
	{
		AssertExactValue(*this, TEXT("Fixture C: chanA ratio"), Result.Channels[0].ComposedRatio, FixtureC::kExpectedChanARatio);
		AssertExactValue(*this, TEXT("Fixture C: chanB ratio"), Result.Channels[1].ComposedRatio, FixtureC::kExpectedChanBRatio);
	}
	return true;
}

// ===========================================================================
// dim 7 (G-31): the shared identity block is populated on every path that yields a drift
// result -- proven here on Fixture B's own asset/asset path (the archive/archive and
// self-comparison paths are covered by SuperFAISSDriftRefusalAndShapeTests.cpp's own
// identity-adjacent cells, §8.7).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftIdentityBlockPopulatedTest,
	"SuperFAISS.D.DriftOracle.IdentityBlockPopulated",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftIdentityBlockPopulatedTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> BaselineRows;
	BuildBaselineRows(BaselineRows);
	USuperFAISSVectorBank* BaselineBank = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("Identity-baseline"));
	TArray<float> CurrentRows;
	FixtureB::BuildCurrentRows(CurrentRows);
	USuperFAISSVectorBank* CurrentBank = BakeL2Int8Asset(*this, CurrentRows, kBaselineRowCount, TEXT("Identity-current"));
	if (BaselineBank == nullptr || CurrentBank == nullptr) { return true; }

	Inspector->SetBankForTest(CurrentBank);
	Inspector->SetComparisonBankForTest(BaselineBank);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();

	AssertExactValue(*this, TEXT("identity: primary row count"), Result.Identity.PrimaryLiveRowCount, kBaselineRowCount);
	AssertExactValue(*this, TEXT("identity: comparison row count"), Result.Identity.ComparisonLiveRowCount, kBaselineRowCount);
	AssertExactValue(*this, TEXT("identity: primary dims"), Result.Identity.PrimaryDims, kDriftDims);
	AssertExactValue(*this, TEXT("identity: comparison dims"), Result.Identity.ComparisonDims, kDriftDims);
	AssertExactValue(*this, TEXT("identity: primary display name non-empty"), Result.Identity.PrimaryDisplayName.IsEmpty(), false);
	AssertExactValue(*this, TEXT("identity: comparison display name non-empty"), Result.Identity.ComparisonDisplayName.IsEmpty(), false);
	AssertExactValue(*this, TEXT("identity: not a self-comparison"), Result.Identity.bSelfComparison, false);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
