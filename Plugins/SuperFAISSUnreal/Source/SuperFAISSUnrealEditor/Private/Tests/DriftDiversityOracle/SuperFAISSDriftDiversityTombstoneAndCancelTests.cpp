// SuperFAISS For Unreal 3.4 -- drift's tombstone threading and cancel/re-attribution
// (dim 2/dim 3a), diversity's tombstone-in-overfetch (dim 2/dim 8). 2026-08-08,
// D-SLM1830/D-SLM1832 completion passes.
//
// The two diversity tests below (`FSuperFAISSDiversityTombstoneExclusionTest`,
// `FSuperFAISSDiversityTieBreakDeterminismTest`) call Gate 6b's seams and are guarded by
// `SUPERFAISS_GATE6B_BUILT` (D-SLM3890), which this module's `.Build.cs` defines 1 now that
// Gate 6b is built. The three drift tests use only Gate 6a's seams and are unguarded.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SSuperFAISSBankInspector.h"
#include "SuperFAISSScratchBank.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "Fixtures/SuperFAISSDriftFixtures.h"
#include "Fixtures/SuperFAISSDiversityFixtures.h"
#include "Fixtures/SuperFAISSDriftDiversityTestContracts.h"

using namespace SuperFAISSDriftDiversityOracle;

namespace
{
	// Bakes a scratch archive from flat row data, tombstoning RowsToTombstone before
	// Save -- mirrors SuperFAISSTutorialBankFixture.h's own BakeAsArchiveBytes shape
	// exactly, generalized to an arbitrary geometry (metric/dims/channels) rather than the
	// tutorial bank's own fixed 8-dim Cosine/Float32 scheme.
	bool BakeArchiveBytes(FAutomationTestBase& Test, const TArray<float>& Rows, int32 RowCount, int32 Dims,
		ESuperFAISSBankMetric Metric, ESuperFAISSBankQuantization Quant, const TArray<int32>& RowsToTombstone,
		TArray<uint8>& OutBytes)
	{
		USuperFAISSScratchBank* Scratch = NewObject<USuperFAISSScratchBank>();
		const bool bInitOk = Scratch->Init(RowCount, Dims, Metric, Quant);
		AssertExactValue(Test, TEXT("(setup) BakeArchiveBytes: Init"), bInitOk, true);
		if (!bInitOk) { return false; }
		for (int32 i = 0; i < RowCount; ++i)
		{
			TArray<float> Row;
			Row.Append(&Rows[static_cast<int64>(i) * Dims], Dims);
			int32 OutIndex = INDEX_NONE;
			const bool bAppendOk = Scratch->Append(Row, OutIndex);
			AssertExactValue(Test, FString::Printf(TEXT("(setup) BakeArchiveBytes: Append row %d"), i), bAppendOk, true);
			if (!bAppendOk) { return false; }
		}
		for (const int32 Idx : RowsToTombstone)
		{
			const bool bRemoveOk = Scratch->Remove(Idx);
			AssertExactValue(Test, FString::Printf(TEXT("(setup) BakeArchiveBytes: Remove row %d"), Idx), bRemoveOk, true);
			if (!bRemoveOk) { return false; }
		}
		return Scratch->SaveToBytes(OutBytes);
	}
}

// ===========================================================================
// dim 2/dim 8, drift: both GetTombstoneWords() sides are threaded into every operator
// call (§8.8) -- a tombstoned row on the CURRENT side is excluded from the compute exactly
// as Correspondence already excludes it. The current side is Fixture A's current rows saved
// as a scratch archive with row 0 (one of the four perturbed rows) tombstoned; Movement is
// asserted exactly equal to CentroidDistanceCrossDevice over the 21 live rows
// (FixtureA::kExpectedMovementRow0Tombstoned, derived independently of the panel by the
// T-3008b probe). A build that ignores the tombstone displays 0.611961246 and fails.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftTombstoneThreadingTest,
	"SuperFAISS.D.DriftShape.TombstoneThreading",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftTombstoneThreadingTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> BaselineRows;
	BuildBaselineRows(BaselineRows);
	USuperFAISSVectorBank* Baseline = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("TombstoneThreading-baseline"));
	if (Baseline == nullptr) { return true; }

	TArray<float> CurrentRows;
	FixtureA::BuildCurrentRows(CurrentRows);
	TArray<uint8> ArchiveBytes;
	const bool bBaked = BakeArchiveBytes(*this, CurrentRows, kBaselineRowCount, kDriftDims,
		ESuperFAISSBankMetric::L2, ESuperFAISSBankQuantization::Int8, {0}, ArchiveBytes); // tombstone row 0 (one of the four perturbed X1-X4 rows)
	AssertExactValue(*this, TEXT("(setup) archive with row 0 tombstoned bakes"), bBaked, true);
	if (!bBaked) { return true; }
	const bool bOpened = Inspector->OpenScratchArchiveFromBytes(ArchiveBytes, TEXT("TombstoneThreading-current-archive"));
	AssertExactValue(*this, TEXT("(setup) tombstoned archive opens as primary"), bOpened, true);
	if (!bOpened) { return true; }

	Inspector->SetComparisonBankForTest(Baseline);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));
	const FSuperFAISSDriftResultForTest Result = Inspector->GetDriftResultForTest();

	AssertExactValue(*this, TEXT("Movement over the live rows only (row 0 tombstoned)"),
		Result.Movement, FixtureA::kExpectedMovementRow0Tombstoned);
	return true;
}

// ===========================================================================
// dim 3a, drift: cancelling the compute before completion leaves no partial DriftResult
// and reports status "cancelled"; after a cancel, switching the comparison bank and
// re-triggering produces a result correctly attributed to the newly-selected bank, not
// the cancelled attempt's stale state. Mirrors
// FSuperFAISSInspectorCorrespondenceCancelTest's own existing shape (coverage audit G-3a).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDriftCancelAndReattributeTest,
	"SuperFAISS.D.DriftShape.CancelAndReattribute",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDriftCancelAndReattributeTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> BaselineRows;
	BuildBaselineRows(BaselineRows);
	USuperFAISSVectorBank* CompA = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("Cancel-compA"));
	TArray<float> FixtureBRows;
	FixtureB::BuildCurrentRows(FixtureBRows);
	USuperFAISSVectorBank* CompB = BakeL2Int8Asset(*this, FixtureBRows, kBaselineRowCount, TEXT("Cancel-compB"));
	USuperFAISSVectorBank* Primary = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("Cancel-primary"));
	if (CompA == nullptr || CompB == nullptr || Primary == nullptr) { return true; }

	Inspector->SetBankForTest(Primary);
	Inspector->SetComparisonBankForTest(CompA);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	// Cancel before the first chunk boundary is even reached.
	Inspector->DebugCancelAfterChunks = 0;
	const FSuperFAISSDriftResultForTest CancelledResult = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("cancelled compute: bCancelled set"), CancelledResult.bCancelled, true);
	AssertExactValue(*this, TEXT("cancelled compute: no partial Movement left behind"), CancelledResult.Movement, 0.0f);

	// Re-arm (no further forced cancel) and switch the comparison bank -- the
	// re-triggered result must reflect CompB, not CompA and not the cancelled attempt.
	Inspector->DebugCancelAfterChunks = -1;
	Inspector->SetComparisonBankForTest(CompB);
	const FSuperFAISSDriftResultForTest ReattributedResult = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("re-triggered compute attributed to CompB"), ReattributedResult.Identity.ComparisonDisplayName, CompB->GetName());
	return true;
}

#if SUPERFAISS_GATE6B_BUILT

// ===========================================================================
// dim 2/dim 8, diversity: the over-fetch pool excludes tombstoned rows identically to the
// plain query at the same K, through the same QueryScratch mechanism (§9.3, G-5a/G-5b).
// Archive-source-only (Asset sources carry no tombstone concept, §9.3). Proven by
// tombstoning one of L2ChannellessDiversityFixture's own candidates (bank row 3) and
// confirming the selection holds exactly the 3 remaining live candidates (bank rows 1, 2, 4
// -- row 0 is the queried row, excluded from its own pool, D-SLM7837), not 4: a build that
// failed to thread the tombstone into the over-fetch construction would build its pool from
// all 4 non-query rows, and at K=5 (requesting more than either count) would return 4
// selections rather than the tombstone-respecting 3.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityTombstoneExclusionTest,
	"SuperFAISS.D.DiversityShape.TombstoneExclusion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityTombstoneExclusionTest::RunTest(const FString& Parameters)
{
	using namespace L2ChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildRows(Rows);
	TArray<uint8> ArchiveBytes;
	const bool bBaked = BakeArchiveBytes(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2,
		ESuperFAISSBankQuantization::Int8, {3}, ArchiveBytes); // tombstone bank row 3 (raw score 110)
	AssertExactValue(*this, TEXT("(setup) archive with row 3 tombstoned bakes"), bBaked, true);
	if (!bBaked) { return true; }
	const bool bOpened = Inspector->OpenScratchArchiveFromBytes(ArchiveBytes, TEXT("TombstoneExclusion-archive"));
	AssertExactValue(*this, TEXT("(setup) tombstoned archive opens"), bOpened, true);
	if (!bOpened) { return true; }

	Inspector->SetDiversityLambdaForTest(0.5f);
	Inspector->SetQueryKForTest(kRows); // K=5, requested above BOTH the tombstoned-inclusive (4) and tombstoned-exclusive (3) candidate counts
	Inspector->RunQueryForTest(kQueryRow0);

	const FSuperFAISSMMRSelectionForTest Result = Inspector->GetLastMMRSelectionForTest();
	// Exactly 3 live, non-query candidates remain (bank rows 1, 2, 4) -- an EXACT count,
	// since the small-bank second clamp (§9.2, FSuperFAISSDiversitySmallBankSecondClampTest)
	// clamps K to the pool's own live-row-limited candidateCount; a build that does not
	// thread the tombstone would report 4, not 3.
	AssertExactValue(*this, TEXT("tombstoned-row exclusion: no mid-selection refusal"), Result.bMidSelectionRefusal, false);
	AssertExactValue(*this, TEXT("tombstoned-row exclusion: selection count is exactly the 3 remaining live candidates"),
		Result.SelectedIndices.Num(), 3);
	return true;
}

#endif // SUPERFAISS_GATE6B_BUILT

// ===========================================================================
// dim 1: the shared comparison slot (§7) read by Drift then by Correspondence against the
// SAME loaded comparison bank -- no stale cache from the other feature's last read.
// Correspondence's own MatchPairResults and CorrespondenceStatus are already public
// accessors (P-14 predates this plan); Drift's own compute reads the identical
// GetComparisonSource(), §7's own accessor.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSComparisonSlotDriftCorrespondenceLifetimeTest,
	"SuperFAISS.D.DriftComposition.ComparisonSlotDriftCorrespondenceLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSComparisonSlotDriftCorrespondenceLifetimeTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> BaselineRows;
	BuildBaselineRows(BaselineRows);
	USuperFAISSVectorBank* Primary = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("SlotLifetime-primary"));
	USuperFAISSVectorBank* Comparison = BakeL2Int8Asset(*this, BaselineRows, kBaselineRowCount, TEXT("SlotLifetime-comparison"));
	if (Primary == nullptr || Comparison == nullptr) { return true; }

	Inspector->SetBankForTest(Primary);
	Inspector->SetComparisonBankForTest(Comparison);
	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));

	// Correspondence first, against this comparison bank.
	Inspector->ComputeCorrespondence();
	const FString FirstCorrespondenceStatus = Inspector->GetCorrespondenceStatus();

	// Drift next, against the SAME loaded comparison bank -- must read the current
	// slot, not any state Correspondence's own compute left behind.
	const FSuperFAISSDriftResultForTest DriftResult = Inspector->GetDriftResultForTest();
	AssertExactValue(*this, TEXT("drift reads the same comparison bank Correspondence just used"),
		DriftResult.Identity.ComparisonDisplayName, Comparison->GetName());

	// Correspondence again -- its own status must be recomputed fresh, not left over
	// from the drift compute that ran in between (the reverse direction of §7's own
	// shared-read-no-staleness requirement).
	Inspector->ComputeCorrespondence();
	const FString SecondCorrespondenceStatus = Inspector->GetCorrespondenceStatus();
	AssertExactValue(*this, TEXT("correspondence status stable across an interleaved drift compute (both reads see the same bank)"),
		SecondCorrespondenceStatus, FirstCorrespondenceStatus);
	return true;
}

#if SUPERFAISS_GATE6B_BUILT

// ===========================================================================
// dim 6: diversity's MMR selection is deterministic within its own tier -- the pinned
// ascending-Hit.index tie-break produces the SAME, correct selection across repeated runs on a
// candidate set containing a genuine score tie. Rows 1 and 2 score an identical 136 against
// the queried row 0 (probe §10); the pool ties on ascending bank row (row 1 first) and so does
// SelectDiverseMMR, so at lambda < 1 -- where the argmax itself meets the tie at step 0 -- the
// order is {0, 1} with the derived values, on the first run and on the repeat.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityTieBreakDeterminismTest,
	"SuperFAISS.D.DiversityShape.TieBreakDeterminism",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityTieBreakDeterminismTest::RunTest(const FString& Parameters)
{
	using namespace SuperFAISSDriftDiversityOracle::TieBreakFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = SuperFAISSDriftDiversityOracle::BakeMetricInt8Asset(
		*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("TieBreakDeterminism"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->SetQueryKForTest(kCandidates);

	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("tie-break, first run"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kOrder, kRelevance, kRedundancy);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("tie-break, repeat run"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kOrder, kRelevance, kRedundancy);
	return true;
}

#endif // SUPERFAISS_GATE6B_BUILT

#endif // WITH_DEV_AUTOMATION_TESTS
