// SuperFAISS For Unreal 3.4 -- the diversity oracle (the 3.4 drift-and-diversity plan
//  §11.2, §12 dims 4/7/8/10).
//
// Every test in this file drives the real panel through Gate 6b's seams
// (`RunQueryForTest`/`SetDiversityLambdaForTest`/`SetQueryKForTest`/
// `GetLastMMRSelectionForTest`) and is guarded by `SUPERFAISS_GATE6B_BUILT`.
// Per coverage audit G-1 (§11.2's own pin): the assertion drives the ACTUAL panel query path,
// never a hand-fed direct call to SelectDiverseMMR -- proving AC-3 (the widget's over-fetch
// construction, channel binding, lambda threading, and the queried row's exclusion),
// not only AC-2 (the kernel's own arithmetic, proven at the core level by Gate
// 0b's suite).
//
// Expected values: every one is the real query path's output as the plan specifies
// it -- relevance is the pool's own Hit.score, redundancy the kernel over each candidate's own
// row -- derived independently of the panel by
// a standalone derivation probe
// (see `Fixtures/SuperFAISSDiversityFixtures.h` for each fixture's derivation and the mutants
// each lambda < 1 cell is confirmed to catch).
//
// Scope: all three metrics' channelless fixtures at lambda = 1 (the §9.4 identity, with the
// kernel's displayed values) and lambda < 1, the channel-weight sweep at the sliders' default
// and a non-default weight, and the below-onset resolution-collapse cell in its panel-driven
// form.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SSuperFAISSBankInspector.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "Fixtures/SuperFAISSDiversityFixtures.h"
#include "Fixtures/SuperFAISSDriftDiversityTestContracts.h"

using namespace SuperFAISSDriftDiversityOracle;

#if SUPERFAISS_GATE6B_BUILT

// ===========================================================================
// dim 10, Metric::L2, lambda = 1: the structural identity (§9.4) -- at the slider's default
// position the diversified result is the plain relevance ranking (the pool's own order), and
// the displayed relevance/redundancy are the kernel's values for that order. The queried row
// is absent: a build that leaves it in the pool selects it first with relevance 1 (probe §1).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityL2ChannellessLambda1Test,
	"SuperFAISS.D.DiversityOracle.L2Channelless.Lambda1Identity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityL2ChannellessLambda1Test::RunTest(const FString& Parameters)
{
	using namespace L2ChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims,
		ESuperFAISSBankMetric::L2, TEXT("L2Channelless"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);

	AssertSelection(*this, TEXT("L2 lambda=1"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);
	return true;
}

// ===========================================================================
// dim 10, Metric::L2, the required lambda < 1 case (§11.2's crux paragraph): the MMR selection
// order and every relevance/redundancy value equal the independently derived expected values.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityL2ChannellessLambdaLessThan1Test,
	"SuperFAISS.D.DiversityOracle.L2Channelless.LambdaLessThan1",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityL2ChannellessLambdaLessThan1Test::RunTest(const FString& Parameters)
{
	using namespace L2ChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims,
		ESuperFAISSBankMetric::L2, TEXT("L2Channelless"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->SetQueryKForTest(kCandidates);
	Inspector->RunQueryForTest(kQueryRow0);

	AssertSelection(*this, TEXT("L2 lambda=0.5"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kLambda05Order, kLambda05Relevance, kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 10, Metric::Cosine, lambda = 1 identity + the required lambda < 1 case.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityCosineChannellessTest,
	"SuperFAISS.D.DiversityOracle.CosineChannelless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityCosineChannellessTest::RunTest(const FString& Parameters)
{
	using namespace CosineChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims,
		ESuperFAISSBankMetric::Cosine, TEXT("CosineChannelless"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetQueryKForTest(kCandidates);

	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("Cosine lambda=1"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);

	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("Cosine lambda=0.5"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kLambda05Order, kLambda05Relevance, kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 10, Metric::Dot, lambda = 1 identity + the required lambda < 1 case.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityDotChannellessTest,
	"SuperFAISS.D.DiversityOracle.DotChannelless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityDotChannellessTest::RunTest(const FString& Parameters)
{
	using namespace DotChannellessDiversityFixture;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims,
		ESuperFAISSBankMetric::Dot, TEXT("DotChannelless"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetQueryKForTest(kCandidates);

	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("Dot lambda=1"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, kLambda1Relevance, kLambda1Redundancy);

	Inspector->SetDiversityLambdaForTest(kLambda);
	Inspector->RunQueryForTest(kQueryRow0);
	AssertSelection(*this, TEXT("Dot lambda=0.5"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kLambda05Order, kLambda05Relevance, kLambda05Redundancy);
	return true;
}

// ===========================================================================
// dim 4/dim 7/dim 8/dim 10, the channel-weight sweep, all three metrics: at the sliders'
// default weight AND a non-default weight (0.5, 1.5 -- sum 2 != 1, discriminating for the
// Cosine fixed-1-recovery mutant), the panel's order/relevance/redundancy equal the
// independently derived values at lambda = 1 and lambda = 0.5 -- relevance and redundancy
// computed from the SAME weight vector (dim 8), redundancy tracking the weighted quantity at
// every setting (dim 7).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityChannelWeightSweepTest,
	"SuperFAISS.D.DiversityOracle.ChannelWeightSweep",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityChannelWeightSweepTest::RunTest(const FString& Parameters)
{
	using namespace ChannelWeightFixture;

	struct FMetricCase
	{
		const TCHAR* Name;
		ESuperFAISSBankMetric Metric;
		const float* DefaultRelevance; const float* DefaultRedundancy;
		const float* NonDefaultRelevance; const float* NonDefaultRedundancy;
	};
	const FMetricCase Cases[3] = {
		{ TEXT("L2"), ESuperFAISSBankMetric::L2,
			L2::kDefaultRelevance, L2::kDefaultRedundancy, L2::kNonDefaultRelevance, L2::kNonDefaultRedundancy },
		{ TEXT("Dot"), ESuperFAISSBankMetric::Dot,
			Dot::kDefaultRelevance, Dot::kDefaultRedundancy, Dot::kNonDefaultRelevance, Dot::kNonDefaultRedundancy },
		{ TEXT("Cosine"), ESuperFAISSBankMetric::Cosine,
			Cosine::kDefaultRelevance, Cosine::kDefaultRedundancy, Cosine::kNonDefaultRelevance, Cosine::kNonDefaultRedundancy },
	};

	for (const FMetricCase& C : Cases)
	{
		TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);
		TArray<float> Rows;
		BuildRows(Rows);
		USuperFAISSVectorBank* Bank = BakeMetricInt8AssetTwoChannels(*this, Rows, kRows, kDims, kChanLen,
			C.Metric, FString::Printf(TEXT("ChannelWeight-%s"), C.Name));
		if (Bank == nullptr) { continue; }

		Inspector->SetBankForTest(Bank);
		Inspector->SetQueryKForTest(kCandidates);

		for (int32 Setting = 0; Setting < 2; ++Setting)
		{
			const FWeightSetting& W = (Setting == 0) ? kDefaultWeight : kNonDefaultWeight;
			const float* ExpectedRelevance = (Setting == 0) ? C.DefaultRelevance : C.NonDefaultRelevance;
			const float* ExpectedRedundancy = (Setting == 0) ? C.DefaultRedundancy : C.NonDefaultRedundancy;
			const TCHAR* SettingName = W.bDefault ? TEXT("default") : TEXT("non-default");

			Inspector->SetChannelWeightForTest(0, W.W0);
			Inspector->SetChannelWeightForTest(1, W.W1);

			Inspector->SetDiversityLambdaForTest(1.0f);
			Inspector->RunQueryForTest(kQueryRow0);
			AssertSelection(*this, FString::Printf(TEXT("%s %s weight, lambda=1"), C.Name, SettingName),
				Inspector->GetLastMMRSelectionForTest(), kCandidates, kIdentityOrder, ExpectedRelevance, ExpectedRedundancy);

			Inspector->SetDiversityLambdaForTest(kLambda);
			Inspector->RunQueryForTest(kQueryRow0);
			AssertSelection(*this, FString::Printf(TEXT("%s %s weight, lambda=0.5"), C.Name, SettingName),
				Inspector->GetLastMMRSelectionForTest(), kCandidates, kIdentityOrder, ExpectedRelevance, ExpectedRedundancy);
		}
	}
	return true;
}

// ===========================================================================
// dim 4/dim 10, Metric::L2, the reopened lambda = 1 clause, PANEL-DRIVEN form (§11.4 row 12,
// §12 dim 4's below-onset shape extreme). Every candidate score is produced by the real,
// Int8-quantized retrieval path; the two colliding candidates sit below the onset with equal
// displayed relevance and distinct scores (probe §5), so the panel's order is the ranking-key
// order the remedy specifies, not the collapsed-float tie-break the resolution-collapse mutant
// takes (a standalone derivation probe).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityL2BelowOnsetPanelTest,
	"SuperFAISS.D.DiversityOracle.L2BelowOnsetPanel.Lambda1ResolutionAware",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityL2BelowOnsetPanelTest::RunTest(const FString& Parameters)
{
	using namespace L2DiversityFixtureBelowOnsetPanel;
	TSharedRef<SSuperFAISSBankInspector> Inspector = SNew(SSuperFAISSBankInspector);

	TArray<float> Rows;
	BuildRows(Rows);
	USuperFAISSVectorBank* Bank = BakeMetricInt8Asset(*this, Rows, kRows, kDims, ESuperFAISSBankMetric::L2, TEXT("L2BelowOnsetPanel"));
	if (Bank == nullptr) { return true; }

	Inspector->SetBankForTest(Bank);
	Inspector->SetDiversityLambdaForTest(1.0f);
	Inspector->SetQueryKForTest(kCandidates); // rows 1-4, the full candidate pool
	Inspector->RunQueryForTest(kQueryRow0);

	AssertSelection(*this, TEXT("below-onset panel"), Inspector->GetLastMMRSelectionForTest(), kCandidates,
		kIdentityOrder, kExpectedRelevance, kExpectedRedundancy);
	return true;
}

#endif // SUPERFAISS_GATE6B_BUILT

#endif // WITH_DEV_AUTOMATION_TESTS
