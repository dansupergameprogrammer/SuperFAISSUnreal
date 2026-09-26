// SuperFAISS For Unreal 3.4 -- diversity on Float32 primary banks (supported, no
// bit-identity claim: each candidate row lifted to an int8 cross-device image with
// QuantizeQueryXd, a channelled bank's channels re-laid onto the int8 16-element grid, and the
// Metric::L2 scale L the mean squared distance to the live centroid, in double). The Coverage
// Model's diversity dimensions as they apply to the Float32 path (the 3.4 drift-and-diversity plan
//  §12): dim 10's feature oracle per metric at
// lambda = 1 and lambda < 1; dim 4's channel-weight axis on a channel table whose offsets sit
// OFF the int8 grid (the case the lift exists for), with dim 8's same-weight-vector pairing;
// dims 2/5/11's zero-scale guard on the Float32 L; and dims 5/10/11's mid-selection fallback
// through lifted payloads.
//
// Every expected value is derived independently of the panel by
// a standalone derivation probe
// (probe §7, §8, §13, §14). Per-device: the geometry is built so every value asserted is exact
// in float/double arithmetic whatever the summation order (see
// `Fixtures/SuperFAISSDiversityFixtures.h`, Float32DiversityFixtures), so no assertion here
// depends on this device's rounding; the channelled Cosine leg, whose values cannot be made
// exact, asserts the selection order only.
//
// Guarded by `SUPERFAISS_GATE6B_BUILT`.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SSuperFAISSBankInspector.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "Fixtures/SuperFAISSDiversityFixtures.h"
#include "Fixtures/SuperFAISSDriftDiversityTestContracts.h"

using namespace SuperFAISSDriftDiversityOracle;

#if SUPERFAISS_GATE6B_BUILT

namespace
{
	using namespace SuperFAISSDriftDiversityOracle::Float32DiversityFixtures;

	USuperFAISSVectorBank* BakeFloat32(FAutomationTestBase& Test, const TArray<float>& Rows, int32 Dims,
		ESuperFAISSBankMetric Metric, const FString& DebugName, bool bChannelled = false)
	{
		if (bChannelled)
		{
			return BakeMetricAsset(Test, Rows, kRows, Dims, Metric, ESuperFAISSBankQuantization::Float32, DebugName,
				{TEXT("chanA"), TEXT("chanB")}, {0, Channelled::kChanLen}, {Channelled::kChanLen, Channelled::kChanLen});
		}
		return BakeMetricAsset(Test, Rows, kRows, Dims, Metric, ESuperFAISSBankQuantization::Float32, DebugName);
	}

	// One channelless metric case: lambda = 1 is the identity with the derived values; lambda
	// = 0.5 reorders with the derived values.
	void RunChannellessCase(FAutomationTestBase& Test, const TCHAR* Name, USuperFAISSVectorBank* Bank,
		const float* Lambda1Relevance, const float* Lambda1Redundancy,
		const int32* Lambda05Order, const float* Lambda05Relevance, const float* Lambda05Redundancy)
	{
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		Inspector->SetBankForTest(Bank);
		Inspector->SetQueryKForTest(kCandidates);

		Inspector->SetDiversityLambdaForTest(1.0f);
		Inspector->RunQueryForTest(kQueryRow0);
		AssertSelection(Test, FString::Printf(TEXT("Float32 %s lambda=1"), Name), Inspector->GetLastMMRSelectionForTest(),
			kCandidates, kIdentityOrder, Lambda1Relevance, Lambda1Redundancy);

		Inspector->SetDiversityLambdaForTest(kLambda);
		Inspector->RunQueryForTest(kQueryRow0);
		AssertSelection(Test, FString::Printf(TEXT("Float32 %s lambda=0.5"), Name), Inspector->GetLastMMRSelectionForTest(),
			kCandidates, Lambda05Order, Lambda05Relevance, Lambda05Redundancy);
	}
}

// ===========================================================================
// dim 10, Float32 Metric::L2, channelless: the Float32 L (2.33184481) and the lifted redundancy.
// Every §12 dim 7 L2 mutant and the included-query-row build change an asserted line (probe §13).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityFloat32L2ChannellessTest,
	"SuperFAISS.D.DiversityFloat32.L2Channelless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityFloat32L2ChannellessTest::RunTest(const FString& Parameters)
{
	TArray<float> Rows;
	L2Channelless::BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeFloat32(*this, Rows, L2Channelless::kDims, ESuperFAISSBankMetric::L2, TEXT("Float32L2"));
	if (Bank == nullptr) { return true; }
	RunChannellessCase(*this, TEXT("L2"), Bank, L2Channelless::kLambda1Relevance, L2Channelless::kLambda1Redundancy,
		L2Channelless::kLambda05Order, L2Channelless::kLambda05Relevance, L2Channelless::kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 10, Float32 Metric::Cosine, channelless (rows normalize to exact 0.5s). The Cosine
// polarity mutant and the included-query-row build change an asserted line (probe §13).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityFloat32CosineChannellessTest,
	"SuperFAISS.D.DiversityFloat32.CosineChannelless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityFloat32CosineChannellessTest::RunTest(const FString& Parameters)
{
	TArray<float> Rows;
	CosineChannelless::BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeFloat32(*this, Rows, CosineChannelless::kDims, ESuperFAISSBankMetric::Cosine, TEXT("Float32Cosine"));
	if (Bank == nullptr) { return true; }
	RunChannellessCase(*this, TEXT("Cosine"), Bank, CosineChannelless::kLambda1Relevance, CosineChannelless::kLambda1Redundancy,
		CosineChannelless::kLambda05Order, CosineChannelless::kLambda05Relevance, CosineChannelless::kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 10, Float32 Metric::Dot, channelless.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityFloat32DotChannellessTest,
	"SuperFAISS.D.DiversityFloat32.DotChannelless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityFloat32DotChannellessTest::RunTest(const FString& Parameters)
{
	TArray<float> Rows;
	DotChannelless::BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeFloat32(*this, Rows, DotChannelless::kDims, ESuperFAISSBankMetric::Dot, TEXT("Float32Dot"));
	if (Bank == nullptr) { return true; }
	RunChannellessCase(*this, TEXT("Dot"), Bank, DotChannelless::kLambda1Relevance, DotChannelless::kLambda1Redundancy,
		DotChannelless::kLambda05Order, DotChannelless::kLambda05Relevance, DotChannelless::kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 4/dim 8/dim 10, Float32 channelled with channel offsets OFF the int8 16-element grid
// (chanA [0,20), chanB [20,40)), three metrics x {default (1, 1), non-default (0.5, 1.5)}
// weights, lambda = 1 and lambda = 0.5, on one warm instance per metric. The lift re-lays each
// channel on the 16-grid; a build addressing the unlifted offsets refuses mid-selection
// (probe §14), which the no-refusal assertions catch. At the non-default weight the
// unweighted-redundancy mutant changes a line on every metric, and on Cosine the fixed-1
// recovery changes a line at both weights (probe §14). The Cosine leg asserts order only.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityFloat32ChannelledOffGridSweepTest,
	"SuperFAISS.D.DiversityFloat32.ChannelledOffGridSweep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityFloat32ChannelledOffGridSweepTest::RunTest(const FString& Parameters)
{
	using namespace Channelled;
	TArray<float> Rows;
	BuildRows(Rows);

	struct FSettingExpect
	{
		const float* Weights;
		const TCHAR* Name;
		const float* Lambda1Relevance; const float* Lambda1Redundancy;
		const int32* Lambda05Order; const float* Lambda05Relevance; const float* Lambda05Redundancy;
	};
	struct FMetricCase
	{
		const TCHAR* Name;
		ESuperFAISSBankMetric Metric;
		FSettingExpect Settings[2];
	};
	const FMetricCase Cases[3] = {
		{ TEXT("L2"), ESuperFAISSBankMetric::L2, {
			{ kDefaultW, TEXT("default"), kL2DefaultRelevance, kL2DefaultRedundancy, kIdentityOrder, kL2DefaultRelevance, kL2DefaultRedundancy },
			{ kNonDefaultW, TEXT("non-default"), kL2NonDefaultRelevance, kL2NonDefaultRedundancy, kIdentityOrder, kL2NonDefaultRelevance, kL2NonDefaultRedundancy } } },
		{ TEXT("Dot"), ESuperFAISSBankMetric::Dot, {
			{ kDefaultW, TEXT("default"), kDotDefaultLambda1Relevance, kDotDefaultLambda1Redundancy,
				kDotDefaultLambda05Order, kDotDefaultLambda05Relevance, kDotDefaultLambda05Redundancy },
			{ kNonDefaultW, TEXT("non-default"), kDotNonDefaultLambda1Relevance, kDotNonDefaultLambda1Redundancy,
				kDotNonDefaultLambda05Order, kDotNonDefaultLambda05Relevance, kDotNonDefaultLambda05Redundancy } } },
		{ TEXT("Cosine"), ESuperFAISSBankMetric::Cosine, {
			{ kDefaultW, TEXT("default"), nullptr, nullptr, kCosineDefaultLambda05Order, nullptr, nullptr },
			{ kNonDefaultW, TEXT("non-default"), nullptr, nullptr, kCosineNonDefaultLambda05Order, nullptr, nullptr } } },
	};

	for (const FMetricCase& C : Cases)
	{
		USuperFAISSVectorBank* Bank = BakeFloat32(*this, Rows, kDims, C.Metric,
			FString::Printf(TEXT("Float32Channelled-%s"), C.Name), true);
		if (Bank == nullptr) { continue; }
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		Inspector->SetBankForTest(Bank);
		Inspector->SetQueryKForTest(kCandidates);

		for (const FSettingExpect& S : C.Settings)
		{
			Inspector->SetChannelWeightForTest(0, S.Weights[0]);
			Inspector->SetChannelWeightForTest(1, S.Weights[1]);

			Inspector->SetDiversityLambdaForTest(1.0f);
			Inspector->RunQueryForTest(kQueryRow0);
			AssertSelection(*this, FString::Printf(TEXT("Float32 channelled %s %s weight, lambda=1"), C.Name, S.Name),
				Inspector->GetLastMMRSelectionForTest(), kCandidates, kIdentityOrder, S.Lambda1Relevance, S.Lambda1Redundancy);

			Inspector->SetDiversityLambdaForTest(kLambda);
			Inspector->RunQueryForTest(kQueryRow0);
			AssertSelection(*this, FString::Printf(TEXT("Float32 channelled %s %s weight, lambda=0.5"), C.Name, S.Name),
				Inspector->GetLastMMRSelectionForTest(), kCandidates, S.Lambda05Order, S.Lambda05Relevance, S.Lambda05Redundancy);
		}
	}
	return true;
}

// ===========================================================================
// dims 2/5/11, Float32 Metric::L2 zero-scale guard: four identical rows make the Float32 L
// exactly 0, so a lambda < 1 query refuses the whole panel with §6.2's text and renders the
// plain ranking (pool scores 0, redundancy 0) -- never a mid-selection refusal (probe §7).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityFloat32L2ZeroScaleTest,
	"SuperFAISS.D.DiversityFloat32.L2ZeroScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityFloat32L2ZeroScaleTest::RunTest(const FString& Parameters)
{
	TArray<float> Rows;
	ZeroScale::BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeFloat32(*this, Rows, ZeroScale::kDims, ESuperFAISSBankMetric::L2, TEXT("Float32L2ZeroScale"));
	if (Bank == nullptr) { return true; }

	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	Inspector->SetBankForTest(Bank);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->RunQueryForTest(kQueryRow0);

	const FSuperFAISSMMRSelectionForTest Result = Inspector->GetLastMMRSelectionForTest();
	AssertExactValue(*this, TEXT("Float32 zero scale: whole-panel refusal"), Result.bWholePanelRefusal, true);
	AssertExactValue(*this, TEXT("Float32 zero scale: §6.2's refusal text"), Result.WholePanelRefusalText,
		FString(kExpectedZeroScaleRefusalText));
	AssertExactValue(*this, TEXT("Float32 zero scale: not a mid-selection refusal"), Result.bMidSelectionRefusal, false);
	AssertExactValue(*this, TEXT("Float32 zero scale: plain ranking count"), Result.SelectedIndices.Num(), kCandidates);
	AssertExactValue(*this, TEXT("Float32 zero scale: relevance count"), Result.Relevance.Num(), kCandidates);
	AssertExactValue(*this, TEXT("Float32 zero scale: redundancy count"), Result.Redundancy.Num(), kCandidates);
	if (Result.SelectedIndices.Num() == kCandidates && Result.Relevance.Num() == kCandidates && Result.Redundancy.Num() == kCandidates)
	{
		for (int32 i = 0; i < kCandidates; ++i)
		{
			AssertExactValue(*this, FString::Printf(TEXT("Float32 zero scale: plain order step%d"), i), Result.SelectedIndices[i], kIdentityOrder[i]);
			AssertExactValue(*this, FString::Printf(TEXT("Float32 zero scale: relevance step%d is the pool score"), i),
				Result.Relevance[i], ZeroScale::kPlainRelevance[i]);
			AssertExactValue(*this, FString::Printf(TEXT("Float32 zero scale: redundancy step%d is 0"), i),
				Result.Redundancy[i], ZeroScale::kPlainRedundancy[i]);
		}
	}
	return true;
}

// ===========================================================================
// dims 5/10/11, the §9.5a fallback through lifted payloads: the weighted-zero-norm bank
// (MidSelectionRefusalFixtures) baked Float32. With chanA weighted 0, the lifted chanA-only
// candidate's weighted self-norm is exactly 0, so step 1 returns ZeroNormQuery and the panel
// falls back (probe §8). The fallback's order, zero redundancy and note are asserted; the pool
// relevance is a Float32 Cosine score (inexact) and is not.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityFloat32WeightedZeroNormRefusalTest,
	"SuperFAISS.D.DiversityFloat32.WeightedZeroNormRefusal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityFloat32WeightedZeroNormRefusalTest::RunTest(const FString& Parameters)
{
	namespace M = SuperFAISSDriftDiversityOracle::MidSelectionRefusalFixtures;
	TArray<float> Rows;
	M::BuildWeightedZeroNormRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricAsset(*this, Rows, M::kRows, M::kDims, ESuperFAISSBankMetric::Cosine,
		ESuperFAISSBankQuantization::Float32, TEXT("Float32MidSelectionRefusal"),
		{TEXT("chanA"), TEXT("chanB")}, {0, M::kChanLen}, {M::kChanLen, M::kChanLen});
	if (Bank == nullptr) { return true; }

	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
	Inspector->SetBankForTest(Bank);
	Inspector->SetChannelWeightForTest(0, 0.0f);
	Inspector->SetChannelWeightForTest(1, 1.0f);
	Inspector->SetDiversityLambdaForTest(0.5f);
	Inspector->SetQueryKForTest(M::kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);

	const FSuperFAISSMMRSelectionForTest Result = Inspector->GetLastMMRSelectionForTest();
	AssertExactValue(*this, TEXT("Float32 weighted-zero-norm: mid-selection refusal fires"), Result.bMidSelectionRefusal, true);
	AssertExactValue(*this, TEXT("Float32 weighted-zero-norm: not a whole-panel refusal"), Result.bWholePanelRefusal, false);
	AssertExactValue(*this, TEXT("Float32 weighted-zero-norm: the shared note"), Result.MidSelectionRefusalNoteText,
		FString(kExpectedMidSelectionNote));
	AssertExactValue(*this, TEXT("Float32 weighted-zero-norm: fallback count"), Result.SelectedIndices.Num(), M::kCandidates);
	AssertExactValue(*this, TEXT("Float32 weighted-zero-norm: redundancy count"), Result.Redundancy.Num(), M::kCandidates);
	if (Result.SelectedIndices.Num() == M::kCandidates && Result.Redundancy.Num() == M::kCandidates)
	{
		for (int32 i = 0; i < M::kCandidates; ++i)
		{
			AssertExactValue(*this, FString::Printf(TEXT("Float32 weighted-zero-norm: fallback order step%d"), i),
				Result.SelectedIndices[i], kIdentityOrder[i]);
			AssertExactValue(*this, FString::Printf(TEXT("Float32 weighted-zero-norm: redundancy step%d is 0"), i),
				Result.Redundancy[i], M::kZeroRedundancy[i]);
		}
	}
	return true;
}

#endif // SUPERFAISS_GATE6B_BUILT

#endif // WITH_DEV_AUTOMATION_TESTS
