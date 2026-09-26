// SuperFAISS For Unreal 3.4 -- diversity's refusal, lifetime, and composition cells
// (the 3.4 drift-and-diversity plan §12 dims 1/2/4/5/8/11,
// diversity halves).
//
// Every test below drives Gate 6b's diversity seams (`RunQueryForTest`,
// `SetDiversityLambdaForTest`, `SetQueryKForTest`, `GetLastMMRSelectionForTest`) and is
// guarded by `SUPERFAISS_GATE6B_BUILT`. Every query excludes the queried
// row, so candidate counts are the bank's live rows minus one. Every expected value
// is derived independently of the panel (probe
// a standalone derivation probe; fixtures and derivations in
// `Fixtures/SuperFAISSDiversityFixtures.h`).

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SSuperFAISSBankInspector.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "Fixtures/SuperFAISSDiversityFixtures.h"
#include "Fixtures/SuperFAISSDriftDiversityTestContracts.h"

using namespace SuperFAISSDriftDiversityOracle;

#if SUPERFAISS_GATE6B_BUILT

// ===========================================================================
// dim 2/dim 5/dim 11: the K clamp (§9.2's repair) -- a hostile K driven through
// SetQueryKForTest (which does not itself clamp, §9.7) is clamped to [1, kHardQueryKCap] at
// the over-fetch construction. K = 0 and K = -5 clamp to exactly 1 (an unclamped build asks
// the pool for 0 rows and shows none, probe §1); K = kHardQueryKCap + 1 on a bank with more
// live rows than the cap returns exactly kHardQueryKCap selections (an unclamped build returns
// kHardQueryKCap + 1, probe §1b); K = 1000000 on the 5-row bank returns its 4 candidates.
// Each clamped result is the correct selection, not merely a defined one.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityHostileKClampTest,
	"SuperFAISS.D.DiversityRefusal.HostileKClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityHostileKClampTest::RunTest(const FString& Parameters)
{
	{
		using namespace L2ChannellessDiversityFixture;
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		TArray<float> Rows;
		BuildRows(Rows);
		USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("HostileKClamp"));
		if (Bank == nullptr) { return true; }

		Inspector->SetBankForTest(Bank);
		Inspector->SetDiversityLambdaForTest(1.0f);

		Inspector->SetQueryKForTest(0);
		Inspector->RunQueryForTest(kQueryRow0);
		AssertSelection(*this, TEXT("K=0 clamped to 1"), Inspector->GetLastMMRSelectionForTest(), 1,
			kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);

		Inspector->SetQueryKForTest(-5);
		Inspector->RunQueryForTest(kQueryRow0);
		AssertSelection(*this, TEXT("K=-5 clamped to 1"), Inspector->GetLastMMRSelectionForTest(), 1,
			kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);

		Inspector->SetQueryKForTest(1000000);
		Inspector->RunQueryForTest(kQueryRow0);
		AssertSelection(*this, TEXT("K=1000000 clamped to the 4 live candidates"), Inspector->GetLastMMRSelectionForTest(),
			kCandidates, kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);
	}
	{
		using namespace HostileKBankFixture;
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		TArray<float> Rows;
		BuildRows(Rows);
		USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("HostileKAboveCap"));
		if (Bank == nullptr) { return true; }

		Inspector->SetBankForTest(Bank);
		Inspector->SetDiversityLambdaForTest(1.0f);
		Inspector->SetQueryKForTest(SSuperFAISSBankInspector::kHardQueryKCap + 1);
		Inspector->RunQueryForTest(kQueryRow0);
		const FSuperFAISSMMRSelectionForTest Result = Inspector->GetLastMMRSelectionForTest();
		AssertExactValue(*this, TEXT("K=kHardQueryKCap+1: no refusal"), Result.bMidSelectionRefusal, false);
		AssertExactValue(*this, TEXT("K=kHardQueryKCap+1: selection count is exactly kHardQueryKCap"),
			Result.SelectedIndices.Num(), SSuperFAISSBankInspector::kHardQueryKCap);
		AssertExactValue(*this, TEXT("K=kHardQueryKCap+1: relevance array sized to the clamp"),
			Result.Relevance.Num(), SSuperFAISSBankInspector::kHardQueryKCap);
		if (Result.SelectedIndices.Num() == SSuperFAISSBankInspector::kHardQueryKCap)
		{
			for (int32 i = 0; i < SSuperFAISSBankInspector::kHardQueryKCap; ++i)
			{
				AssertExactValue(*this, FString::Printf(TEXT("K=kHardQueryKCap+1, lambda=1: order step%d is the pool's own"), i),
					Result.SelectedIndices[i], i);
			}
		}
	}
	return true;
}

// ===========================================================================
// dim 2/dim 5/dim 11: the second clamp (plan-temper C-10) -- at the control's default K
// (kDefaultQueryK = 12, pool request 48) on a bank with 4 live candidates, the selection is
// clamped to the pool's own size and is the correct selection: identical to the K = 4 run.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversitySmallBankSecondClampTest,
	"SuperFAISS.D.DiversityRefusal.SmallBankSecondClamp",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversitySmallBankSecondClampTest::RunTest(const FString& Parameters)
{
	using namespace L2ChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("SmallBankClamp"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->SetQueryKForTest(SSuperFAISSBankInspector::kDefaultQueryK);
	Inspector->RunQueryForTest(kQueryRow0);

	AssertSelection(*this, TEXT("default K on a 4-candidate bank"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kLambda05Order, kLambda05Relevance, kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 2/dim 5/dim 11: Metric::L2's zero-scale guard (§6.2), both sides of the boundary.
//   - All live rows identical: Spread(current) = 0 exactly. At lambda < 1 the whole panel
//     refuses with §6.2's own text, SelectDiverseMMR is never called, and the plain ranking
//     renders (the pool's prefix, pool scores, redundancy 0). This is a WHOLE-PANEL refusal,
//     not §9.5a's mid-selection one. At lambda = 1 nothing refuses (§9.1a gates
//     the refusal on lambda < 1) and the same plain ranking renders.
//   - A small but nonzero spread (L = 0.0787401572): no refusal of either kind, the kernel
//     runs, and every displayed value is the finite value derived for it.
// The whole-panel fields need the contract's bWholePanelRefusal/WholePanelRefusalText.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityL2ZeroScaleRefusalTest,
	"SuperFAISS.D.DiversityRefusal.L2ZeroScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityL2ZeroScaleRefusalTest::RunTest(const FString& Parameters)
{
	{
		using namespace ZeroScaleIdenticalRows;
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		TArray<float> Rows;
		BuildRows(Rows);
		USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("L2ZeroScale"));
		if (Bank == nullptr) { return true; }

		Inspector->SetBankForTest(Bank);
		Inspector->SetQueryKForTest(kCandidates);

		Inspector->SetDiversityLambdaForTest(0.5f);
		Inspector->RunQueryForTest(kQueryRow0);
		const FSuperFAISSMMRSelectionForTest Refused = Inspector->GetLastMMRSelectionForTest();
		AssertExactValue(*this, TEXT("zero scale, lambda<1: whole-panel refusal"), Refused.bWholePanelRefusal, true);
		AssertExactValue(*this, TEXT("zero scale, lambda<1: §6.2's refusal text"), Refused.WholePanelRefusalText,
			FString(kExpectedZeroScaleRefusalText));
		AssertExactValue(*this, TEXT("zero scale, lambda<1: not a mid-selection refusal"), Refused.bMidSelectionRefusal, false);
		AssertExactValue(*this, TEXT("zero scale, lambda<1: no mid-selection note"), Refused.MidSelectionRefusalNoteText, FString());
		AssertExactValue(*this, TEXT("zero scale, lambda<1: plain ranking count"), Refused.SelectedIndices.Num(), kCandidates);
		AssertExactValue(*this, TEXT("zero scale, lambda<1: relevance count"), Refused.Relevance.Num(), kCandidates);
		AssertExactValue(*this, TEXT("zero scale, lambda<1: redundancy count"), Refused.Redundancy.Num(), kCandidates);
		if (Refused.SelectedIndices.Num() == kCandidates && Refused.Relevance.Num() == kCandidates &&
			Refused.Redundancy.Num() == kCandidates)
		{
			for (int32 i = 0; i < kCandidates; ++i)
			{
				AssertExactValue(*this, FString::Printf(TEXT("zero scale, lambda<1: plain order step%d"), i), Refused.SelectedIndices[i], kIdentityOrder[i]);
				AssertExactValue(*this, FString::Printf(TEXT("zero scale, lambda<1: relevance step%d is the pool score"), i), Refused.Relevance[i], kPlainRelevance[i]);
				AssertExactValue(*this, FString::Printf(TEXT("zero scale, lambda<1: redundancy step%d is 0"), i), Refused.Redundancy[i], kPlainRedundancy[i]);
			}
		}

		Inspector->SetDiversityLambdaForTest(1.0f);
		Inspector->RunQueryForTest(kQueryRow0);
		AssertSelection(*this, TEXT("zero scale, lambda=1: plain ranking, no refusal"), Inspector->GetLastMMRSelectionForTest(),
			kCandidates, kIdentityOrder, kPlainRelevance, kPlainRedundancy);
	}
	{
		using namespace SmallNonzeroScale;
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		TArray<float> Rows;
		BuildRows(Rows);
		USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("L2SmallScale"));
		if (Bank == nullptr) { return true; }

		Inspector->SetBankForTest(Bank);
		Inspector->SetQueryKForTest(kCandidates);
		Inspector->SetDiversityLambdaForTest(kLambda);
		Inspector->RunQueryForTest(kQueryRow0);
		AssertSelection(*this, TEXT("small nonzero scale, lambda<1: kernel runs, finite values"), Inspector->GetLastMMRSelectionForTest(),
			kCandidates, kLambda05Order, kLambda05Relevance, kLambda05Redundancy);
	}
	return true;
}

// ===========================================================================
// dim 1: Metric::L2's bank-intrinsic scale L (§6.2) is cached per primary bank; a primary-bank
// re-select recomputes it. After bank 1 (L = 9.06863976) the panel is re-selected to a bank
// with L = 14.2434435, and its display equals that bank's own derived values; the same pool
// ranked with bank 1's stale L changes every line (probe §6).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityL2CacheRecomputesOnReselectTest,
	"SuperFAISS.D.DiversityLifetime.L2CacheRecomputesOnReselect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityL2CacheRecomputesOnReselectTest::RunTest(const FString& Parameters)
{
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows1;
	L2ChannellessDiversityFixture::BuildRows(Rows1);
	USuperFAISSVectorBank* Bank1 = BakeMetricInt8Asset(*this, Rows1, L2ChannellessDiversityFixture::kRows,
		L2ChannellessDiversityFixture::kDims, ESuperFAISSBankMetric::L2, TEXT("L2Cache-bank1"));
	TArray<float> Rows2;
	L2ReselectBankFixture::BuildRows(Rows2);
	USuperFAISSVectorBank* Bank2 = BakeMetricInt8Asset(*this, Rows2, L2ReselectBankFixture::kRows,
		L2ReselectBankFixture::kDims, ESuperFAISSBankMetric::L2, TEXT("L2Cache-bank2"));
	if (Bank1 == nullptr || Bank2 == nullptr) { return true; }

	Inspector->SetBankForTest(Bank1);
	Inspector->SetDiversityLambdaForTest(0.5f);
	Inspector->SetQueryKForTest(L2ChannellessDiversityFixture::kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("bank1 (L cached)"), Inspector->GetLastMMRSelectionForTest(),
		L2ChannellessDiversityFixture::kCandidates, L2ChannellessDiversityFixture::kLambda05Order,
		L2ChannellessDiversityFixture::kLambda05Relevance, L2ChannellessDiversityFixture::kLambda05Redundancy);

	Inspector->SetBankForTest(Bank2);
	Inspector->SetDiversityLambdaForTest(L2ReselectBankFixture::kLambda);
	Inspector->SetQueryKForTest(L2ReselectBankFixture::kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("bank2 after re-select (its own L)"), Inspector->GetLastMMRSelectionForTest(),
		L2ReselectBankFixture::kCandidates, L2ReselectBankFixture::kLambda05Order,
		L2ReselectBankFixture::kLambda05Relevance, L2ReselectBankFixture::kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 1: on one warm panel instance, a query at large K followed by a query at small K on the
// same bank leaves the second query's arrays sized to the small K and holding exactly the
// small-K selection -- the 2-step prefix of the full selection (greedy MMR is prefix-stable,
// probe §1) -- with nothing carried over from the larger pool.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityKSizedArrayReuseTest,
	"SuperFAISS.D.DiversityLifetime.KSizedArrayReuseLargeToSmall",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityKSizedArrayReuseTest::RunTest(const FString& Parameters)
{
	using namespace L2ChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("KArrayReuse"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversityLambdaForTest(kLambda);

	Inspector->SetQueryKForTest(kCandidates); // large: the full 4-candidate pool
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("large K"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kLambda05Order, kLambda05Relevance, kLambda05Redundancy);

	Inspector->SetQueryKForTest(2); // small
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("small K after large K"), Inspector->GetLastMMRSelectionForTest(), 2,
		kLambda05Order, kLambda05Relevance, kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 1: repeated slider moves within one query session never leak a prior selection's
// values into a new one -- lambda = 1, then lambda = 0.5, then lambda = 1 again on one warm
// instance each display exactly their own derived arrays.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityLambdaSessionReuseTest,
	"SuperFAISS.D.DiversityLifetime.LambdaSessionReuseNoLeak",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityLambdaSessionReuseTest::RunTest(const FString& Parameters)
{
	using namespace L2ChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("LambdaSessionReuse"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetQueryKForTest(kCandidates);

	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("first lambda=1"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);

	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("lambda=0.5 after lambda=1"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kLambda05Order, kLambda05Relevance, kLambda05Redundancy);

	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("lambda=1 after lambda=0.5"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);
	return true;
}

// ===========================================================================
// dim 8: Metric::L2's scale L and the shared analysis-scope combo do NOT interact -- on a
// channelled L2 bank, scoping the combo to chanA changes no diversity result: both scopes
// display exactly the whole-row derived values. A build whose L read the combo's scope would
// use the chanA-scoped L (7.87764263 instead of 9.95831013) and change every line (probe §4).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityL2ScaleDoesNotCrossScopeComboTest,
	"SuperFAISS.D.DiversityComposition.L2ScaleDoesNotCrossScopeCombo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityL2ScaleDoesNotCrossScopeComboTest::RunTest(const FString& Parameters)
{
	using namespace ChannelWeightFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8AssetTwoChannels(*this, Rows, kRows, kDims, kChanLen,
		ESuperFAISSBankMetric::L2, TEXT("ScaleVsScope"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->SetQueryKForTest(kCandidates);

	Inspector->SetAnalysisScopeForTest(TEXT("(whole row)"));
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("scope (whole row)"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, L2::kDefaultRelevance, L2::kDefaultRedundancy);

	Inspector->SetAnalysisScopeForTest(TEXT("chanA"));
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("scope chanA (L never reads the combo)"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, L2::kDefaultRelevance, L2::kDefaultRedundancy);
	return true;
}

// ===========================================================================
// dim 8 (and dim 1, G-21): within-feature relevance/redundancy weight-vector pairing on one
// warm instance -- the non-default weight first, then the default (the reverse of the oracle
// sweep's direction), each query displaying exactly its own weight's derived arrays, never the
// other weight's and never a mixture. A build reading a stale or default weight copy for one
// operand changes a line at the non-default weight (probe §4, unweighted-redundancy mutant).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityWithinFeatureWeightVectorPairingTest,
	"SuperFAISS.D.DiversityComposition.WithinFeatureWeightVectorPairing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityWithinFeatureWeightVectorPairingTest::RunTest(const FString& Parameters)
{
	using namespace ChannelWeightFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8AssetTwoChannels(*this, Rows, kRows, kDims, kChanLen,
		ESuperFAISSBankMetric::L2, TEXT("WeightVectorPairing"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->SetDiversityLambdaForTest(kLambda);

	Inspector->SetChannelWeightForTest(0, kNonDefaultWeight.W0);
	Inspector->SetChannelWeightForTest(1, kNonDefaultWeight.W1);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("non-default weight first"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, L2::kNonDefaultRelevance, L2::kNonDefaultRedundancy);

	Inspector->SetChannelWeightForTest(0, kDefaultWeight.W0);
	Inspector->SetChannelWeightForTest(1, kDefaultWeight.W1);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("default weight second"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, L2::kDefaultRelevance, L2::kDefaultRedundancy);
	return true;
}

// ===========================================================================
// dim 4/dim 8: a channel table with a GAP (not tiling paddedDims), re-scoped from dim 8's own
// BuildScanRanges-agreement cell to this outcome-level proof. Two banks differ
// only in the gap's content, with every row's int8 scale held fixed; chanB is
// weighted 0 (the weight-0 segment G-19 requires).
//   - Dot: the two banks display identical order, relevance and redundancy, equal to the
//     derived values -- the gap is read by neither operand.
//   - L2: §6.2 computes L whole-row, so the gap moves L and every displayed value; each bank
//     displays exactly its own derived values, and the probe confirms that at one shared L the
//     two banks' values coincide (probe §12).
// A build scoring the gap in the redundancy operand only changes a line on both metrics.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityGappedChannelTableConsistencyTest,
	"SuperFAISS.D.DiversityComposition.GappedChannelTableConsistency",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityGappedChannelTableConsistencyTest::RunTest(const FString& Parameters)
{
	using namespace GappedChannelFixture;

	auto RunFixture = [this](ESuperFAISSBankMetric Metric, bool bFillGap) -> FSuperFAISSMMRSelectionForTest
	{
		TArray<float> Rows;
		BuildRows(bFillGap, Rows);
		USuperFAISSVectorBank* Bank = BakeMetricAsset(*this, Rows, kRows, kDims, Metric, ESuperFAISSBankQuantization::Int8,
			FString::Printf(TEXT("GappedChannelTable-%s-%s"), Metric == ESuperFAISSBankMetric::Dot ? TEXT("Dot") : TEXT("L2"),
				bFillGap ? TEXT("filled") : TEXT("zero")),
			{TEXT("chanA"), TEXT("chanB")}, {0, 32}, {kChanLen, kChanLen});
		if (Bank == nullptr) { return FSuperFAISSMMRSelectionForTest(); }
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		Inspector->SetBankForTest(Bank);
		Inspector->SetChannelWeightForTest(0, 1.0f); // chanA
		Inspector->SetChannelWeightForTest(1, 0.0f); // chanB -- the weight-0 segment
		Inspector->SetDiversityLambdaForTest(kLambda);
		Inspector->SetQueryKForTest(kCandidates);
		Inspector->RunQueryForTest(kQueryRow0);
		return Inspector->GetLastMMRSelectionForTest();
	};

	AssertSelection(*this, TEXT("Dot, gap zero"), RunFixture(ESuperFAISSBankMetric::Dot, false), kCandidates,
		kOrder, kDotRelevance, kDotRedundancy);
	AssertSelection(*this, TEXT("Dot, gap filled (identical to gap zero)"), RunFixture(ESuperFAISSBankMetric::Dot, true), kCandidates,
		kOrder, kDotRelevance, kDotRedundancy);
	AssertSelection(*this, TEXT("L2, gap zero"), RunFixture(ESuperFAISSBankMetric::L2, false), kCandidates,
		kOrder, kL2GapZeroRelevance, kL2GapZeroRedundancy);
	AssertSelection(*this, TEXT("L2, gap filled (its own whole-row L)"), RunFixture(ESuperFAISSBankMetric::L2, true), kCandidates,
		kOrder, kL2GapFilledRelevance, kL2GapFilledRedundancy);
	return true;
}

#endif // SUPERFAISS_GATE6B_BUILT

#endif // WITH_DEV_AUTOMATION_TESTS
