// SuperFAISS For Unreal 3.4 -- `ScoreXdPairSegmented`'s own public-boundary surface
// (the 3.4 drift-and-diversity plan §6.2, §12 dims 2/5/6/8/11).
// Gate 1 red suite.
//
// COMPILE STATUS: compiles and runs TODAY. `ScoreXdPairSegmented` (analytics.h) and
// `ValidateSegments` (validate.h) are already vendored, built, and CLOSED at Gate 0b
// -- these cells need no panel seam and no widget code, only the
// already-shipped core the plugin's ThirdParty tree already vendors. This file is the
// governed suite's one immediately-green cluster; every other governed file in this
// directory awaits Gate 6a/6b's panel symbols.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "superfaiss/analytics.h"
#include "superfaiss/validate.h"
#include "superfaiss/bake.h"
#include "superfaiss/kernels.h"
#include "superfaiss/types.h"

using namespace superfaiss;

namespace
{
	// A well-formed 32-dim (2x16) Int8 XdQuery pair, reused across every cell below --
	// two one-hot-ish payloads with distinct, nonzero content on both channels, so a
	// segment list that scores "only chanA" or "only chanB" produces a genuinely
	// different, nonzero value from the whole-row score (distinguishing a correctly
	// scoped partial score from an accidentally-whole-row one).
	struct FPairFixture
	{
		static constexpr int32 kDims = 32;
		int32 PaddedDims = 0;
		TArray<uint8, TAlignedHeapAllocator<16>> PayloadA;
		TArray<uint8, TAlignedHeapAllocator<16>> PayloadB;
		XdQuery A;
		XdQuery B;

		void Build(Metric InMetric)
		{
			PaddedDims = PaddedDims_(kDims);
			TArray<float> RowA, RowB;
			RowA.SetNumZeroed(kDims);
			RowB.SetNumZeroed(kDims);
			RowA[0] = 6.0f; RowA[16] = 8.0f;   // chanA=6 on dim0, chanB=8 on dim16
			RowB[1] = 3.0f; RowB[17] = 4.0f;   // chanA on dim1, chanB on dim17 -- different dims than A
			PayloadA.SetNumZeroed(PaddedDims);
			PayloadB.SetNumZeroed(PaddedDims);
			float ScaleA = 0.0f, ScaleB = 0.0f;
			QuantizeRowsInt8(RowA.GetData(), 1, kDims, PaddedDims, reinterpret_cast<int8_t*>(PayloadA.GetData()), &ScaleA);
			QuantizeRowsInt8(RowB.GetData(), 1, kDims, PaddedDims, reinterpret_cast<int8_t*>(PayloadB.GetData()), &ScaleB);
			A.q8 = reinterpret_cast<const int8_t*>(PayloadA.GetData());
			A.scale = static_cast<double>(ScaleA);
			B.q8 = reinterpret_cast<const int8_t*>(PayloadB.GetData());
			B.scale = static_cast<double>(ScaleB);
			A.sqSum = SelfDot(PayloadA);
			B.sqSum = SelfDot(PayloadB);
		}

		int64_t SelfDot(const TArray<uint8, TAlignedHeapAllocator<16>>& Payload) const
		{
			int64_t Sum = 0;
			for (int32 d = 0; d < PaddedDims; ++d)
			{
				const int64_t v = static_cast<int8_t>(Payload[d]);
				Sum += v * v;
			}
			return Sum;
		}

		static int32 PaddedDims_(int32 Dims)
		{
			return superfaiss::PaddedDims(Dims, Quantization::Int8);
		}
	};
}

// ===========================================================================
// dim 6: segmentCount==0 (and, separately, segments==nullptr with a positive count) score
// bit-identical to ScoreXdPair (§6.2's own load-bearing degenerate-identity claim, dim 6's
// G-23 cell -- "no cell requires it" before this test).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSScoreXdPairSegmentedDegenerateIdentityTest,
	"SuperFAISS.D.ScoreXdPairSegmented.DegenerateIdentityBitExact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSScoreXdPairSegmentedDegenerateIdentityTest::RunTest(const FString& Parameters)
{
	const Metric Metrics[3] = { Metric::Dot, Metric::L2, Metric::Cosine };
	const TCHAR* MetricNames[3] = { TEXT("Dot"), TEXT("L2"), TEXT("Cosine") };
	for (int32 m = 0; m < 3; ++m)
	{
		FPairFixture Fixture;
		Fixture.Build(Metrics[m]);

		float ScoreUnsegmented = 0.0f;
		const Status StatusUnsegmented = ScoreXdPair(Fixture.A, Fixture.B, Fixture.PaddedDims, Metrics[m], &ScoreUnsegmented);
		AssertExactValue(*this, FString::Printf(TEXT("%s: ScoreXdPair status Ok"), MetricNames[m]),
			static_cast<int32>(StatusUnsegmented), static_cast<int32>(Status::Ok));

		float ScoreZeroCount = 0.0f;
		const Status StatusZeroCount = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metrics[m],
			nullptr, 0, &ScoreZeroCount);
		AssertExactValue(*this, FString::Printf(TEXT("%s: segmentCount=0 status Ok"), MetricNames[m]),
			static_cast<int32>(StatusZeroCount), static_cast<int32>(Status::Ok));
		AssertExactValue(*this, FString::Printf(TEXT("%s: segmentCount=0 bit-identical to ScoreXdPair"), MetricNames[m]),
			ScoreZeroCount, ScoreUnsegmented);

		// segments==nullptr with a POSITIVE segmentCount also takes the degenerate path
		// ("never dereferenced") -- a distinct trigger from segmentCount==0.
		float ScoreNullSegments = 0.0f;
		const Status StatusNullSegments = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metrics[m],
			nullptr, 3, &ScoreNullSegments);
		AssertExactValue(*this, FString::Printf(TEXT("%s: null segments, count=3 status Ok"), MetricNames[m]),
			static_cast<int32>(StatusNullSegments), static_cast<int32>(Status::Ok));
		AssertExactValue(*this, FString::Printf(TEXT("%s: null segments bit-identical to ScoreXdPair"), MetricNames[m]),
			ScoreNullSegments, ScoreUnsegmented);
	}
	return true;
}

// ===========================================================================
// dim 6: two calls with byte-identical inputs return a bit-identical score, per metric
// (determinism -- a new primitive, not an inherited already-proven claim the way drift's
// reuse of ScoreXdPair is).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSScoreXdPairSegmentedDeterminismTest,
	"SuperFAISS.D.ScoreXdPairSegmented.RepeatCallBitIdentical",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSScoreXdPairSegmentedDeterminismTest::RunTest(const FString& Parameters)
{
	const Metric Metrics[3] = { Metric::Dot, Metric::L2, Metric::Cosine };
	const TCHAR* MetricNames[3] = { TEXT("Dot"), TEXT("L2"), TEXT("Cosine") };
	const QuerySegment Segments[2] = { {0, 16, 1.0f}, {16, 16, 1.5f} };
	for (int32 m = 0; m < 3; ++m)
	{
		FPairFixture Fixture;
		Fixture.Build(Metrics[m]);
		float First = 0.0f, Second = 0.0f;
		const Status St1 = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metrics[m], Segments, 2, &First);
		const Status St2 = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metrics[m], Segments, 2, &Second);
		AssertExactValue(*this, FString::Printf(TEXT("%s: repeat call status Ok (1st)"), MetricNames[m]),
			static_cast<int32>(St1), static_cast<int32>(Status::Ok));
		AssertExactValue(*this, FString::Printf(TEXT("%s: repeat call status Ok (2nd)"), MetricNames[m]),
			static_cast<int32>(St2), static_cast<int32>(Status::Ok));
		AssertExactValue(*this, FString::Printf(TEXT("%s: repeat call bit-identical"), MetricNames[m]), Second, First);
	}
	return true;
}

// ===========================================================================
// dim 2/dim 5: the local segment-list validator (§6.2) refuses a malformed, unsorted,
// overlapping, or over-kMaxSegments list -- and ONLY those, never a well-formed one.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSScoreXdPairSegmentedSegmentValidationTest,
	"SuperFAISS.D.ScoreXdPairSegmented.SegmentListValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSScoreXdPairSegmentedSegmentValidationTest::RunTest(const FString& Parameters)
{
	FPairFixture Fixture;
	Fixture.Build(Metric::Dot);
	float Score = 0.0f;

	// Well-formed, in-bound: must NOT refuse (the boundary's other side).
	{
		const QuerySegment Ok[2] = { {0, 16, 1.0f}, {16, 16, 1.0f} };
		const Status St = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metric::Dot, Ok, 2, &Score);
		AssertExactValue(*this, TEXT("well-formed in-bound list: status Ok"), static_cast<int32>(St), static_cast<int32>(Status::Ok));
	}
	// Off-grid offset (not a multiple of 16 -- the Int8 element grid).
	{
		const QuerySegment Bad[1] = { {3, 16, 1.0f} };
		const Status St = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metric::Dot, Bad, 1, &Score);
		AssertExactValue(*this, TEXT("off-grid offset: status InvalidArgument"), static_cast<int32>(St), static_cast<int32>(Status::InvalidArgument));
	}
	// Unsorted (descending offset order).
	{
		const QuerySegment Bad[2] = { {16, 16, 1.0f}, {0, 16, 1.0f} };
		const Status St = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metric::Dot, Bad, 2, &Score);
		AssertExactValue(*this, TEXT("unsorted list: status InvalidArgument"), static_cast<int32>(St), static_cast<int32>(Status::InvalidArgument));
	}
	// Overlapping ranges.
	{
		const QuerySegment Bad[2] = { {0, 20, 1.0f}, {16, 16, 1.0f} };
		const Status St = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metric::Dot, Bad, 2, &Score);
		AssertExactValue(*this, TEXT("overlapping ranges: status InvalidArgument"), static_cast<int32>(St), static_cast<int32>(Status::InvalidArgument));
	}
	// Ends beyond paddedDims.
	{
		const QuerySegment Bad[1] = { {16, 32, 1.0f} }; // 16+32=48 > paddedDims (32)
		const Status St = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metric::Dot, Bad, 1, &Score);
		AssertExactValue(*this, TEXT("range ends beyond paddedDims: status InvalidArgument"), static_cast<int32>(St), static_cast<int32>(Status::InvalidArgument));
	}
	// Over kMaxSegments.
	{
		QuerySegment TooMany[kMaxSegments + 1];
		for (int32 i = 0; i <= kMaxSegments; ++i) { TooMany[i] = {0, 0, 1.0f}; }
		const Status St = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metric::Dot, TooMany, kMaxSegments + 1, &Score);
		AssertExactValue(*this, TEXT("segmentCount > kMaxSegments: status InvalidArgument"), static_cast<int32>(St), static_cast<int32>(Status::InvalidArgument));
	}
	// Negative, finite weight -- this primitive's own STRICTER-than-ValidateSegments rule
	// (dim 8's own named departure).
	{
		const QuerySegment Bad[1] = { {0, 16, -1.0f} };
		const Status StDot = ScoreXdPairSegmented(Fixture.A, Fixture.B, Fixture.PaddedDims, Metric::Dot, Bad, 1, &Score);
		AssertExactValue(*this, TEXT("negative weight (Dot, unaffected metric anyway): status InvalidArgument"),
			static_cast<int32>(StDot), static_cast<int32>(Status::InvalidArgument));
		FPairFixture CosineFixture;
		CosineFixture.Build(Metric::Cosine);
		const Status StCosine = ScoreXdPairSegmented(CosineFixture.A, CosineFixture.B, CosineFixture.PaddedDims,
			Metric::Cosine, Bad, 1, &Score);
		AssertExactValue(*this, TEXT("negative weight (Cosine): status InvalidArgument"),
			static_cast<int32>(StCosine), static_cast<int32>(Status::InvalidArgument));
		FPairFixture L2Fixture;
		L2Fixture.Build(Metric::L2);
		const Status StL2 = ScoreXdPairSegmented(L2Fixture.A, L2Fixture.B, L2Fixture.PaddedDims,
			Metric::L2, Bad, 1, &Score);
		AssertExactValue(*this, TEXT("negative weight (L2): status InvalidArgument"),
			static_cast<int32>(StL2), static_cast<int32>(Status::InvalidArgument));
	}
	return true;
}

// ===========================================================================
// dim 2/dim 5/dim 11: Metric::Cosine's own ZeroNormQuery trigger -- fires exactly when
// either operand's AGGREGATE weighted self-norm over the live ranges is exactly zero, and
// only then. A row zero-content on only ONE of several weighted channels (nonzero
// aggregate) must NOT refuse -- the side of the boundary that exists only because this
// primitive applies no separate per-segment zero-sub-norm rule (coverage-model audit G-28,
// correcting the pre-fold `ValidateSegments`-call framing).
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSScoreXdPairSegmentedCosineZeroNormTest,
	"SuperFAISS.D.ScoreXdPairSegmented.CosineWeightedZeroNorm",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSScoreXdPairSegmentedCosineZeroNormTest::RunTest(const FString& Parameters)
{
	constexpr int32 kDims = 32;
	const int32 Pd = PaddedDims(kDims, Quantization::Int8);

	// A: nonzero on BOTH channels. B: nonzero on chanA only (chanB all-zero) -- zero
	// sub-norm on exactly one of two weighted channels.
	TArray<float> RowA, RowB;
	RowA.SetNumZeroed(kDims); RowA[0] = 6.0f; RowA[16] = 8.0f;
	RowB.SetNumZeroed(kDims); RowB[1] = 3.0f; // chanB (16..31) left all-zero

	TArray<uint8, TAlignedHeapAllocator<16>> PayloadA, PayloadB;
	PayloadA.SetNumZeroed(Pd);
	PayloadB.SetNumZeroed(Pd);
	float ScaleA = 0.0f, ScaleB = 0.0f;
	QuantizeRowsInt8(RowA.GetData(), 1, kDims, Pd, reinterpret_cast<int8_t*>(PayloadA.GetData()), &ScaleA);
	QuantizeRowsInt8(RowB.GetData(), 1, kDims, Pd, reinterpret_cast<int8_t*>(PayloadB.GetData()), &ScaleB);

	XdQuery A, B;
	A.q8 = reinterpret_cast<const int8_t*>(PayloadA.GetData()); A.scale = static_cast<double>(ScaleA);
	B.q8 = reinterpret_cast<const int8_t*>(PayloadB.GetData()); B.scale = static_cast<double>(ScaleB);
	int64_t SqA = 0, SqB = 0;
	for (int32 d = 0; d < Pd; ++d)
	{
		SqA += static_cast<int64_t>(static_cast<int8_t>(PayloadA[d])) * static_cast<int8_t>(PayloadA[d]);
		SqB += static_cast<int64_t>(static_cast<int8_t>(PayloadB[d])) * static_cast<int8_t>(PayloadB[d]);
	}
	A.sqSum = SqA; B.sqSum = SqB;

	const QuerySegment BothChannels[2] = { {0, 16, 1.0f}, {16, 16, 1.0f} };
	const QuerySegment ChanAOnly[1] = { {0, 16, 1.0f} };
	const QuerySegment ChanBOnly[1] = { {16, 16, 1.0f} };
	float Score = 0.0f;

	// Weighted over BOTH channels: B's aggregate weighted self-norm is nonzero (chanA
	// alone is nonzero) -- must NOT refuse, the load-bearing side of this boundary.
	const Status StBoth = ScoreXdPairSegmented(A, B, Pd, Metric::Cosine, BothChannels, 2, &Score);
	AssertExactValue(*this, TEXT("zero-content on one of two weighted channels: status Ok (not refused)"),
		static_cast<int32>(StBoth), static_cast<int32>(Status::Ok));

	// Weighted over chanA only: B's own chanA sub-vector is nonzero -- still Ok.
	const Status StChanA = ScoreXdPairSegmented(A, B, Pd, Metric::Cosine, ChanAOnly, 1, &Score);
	AssertExactValue(*this, TEXT("chanA-only scope on B's live channel: status Ok"),
		static_cast<int32>(StChanA), static_cast<int32>(Status::Ok));

	// Weighted over chanB only: B's own chanB sub-vector IS all-zero, and this is now the
	// AGGREGATE (the only range scored) -- must refuse ZeroNormQuery.
	const Status StChanB = ScoreXdPairSegmented(A, B, Pd, Metric::Cosine, ChanBOnly, 1, &Score);
	AssertExactValue(*this, TEXT("chanB-only scope on B's zero-content channel: status ZeroNormQuery"),
		static_cast<int32>(StChanB), static_cast<int32>(Status::ZeroNormQuery));

	// Weight driven to exactly 0 on every channel B carries live content on (chanA,
	// weight 0) while chanB (also zero-content on B) carries a nonzero weight -- the
	// aggregate weighted self-norm is exactly zero via the weight, not the content.
	const QuerySegment ZeroWeightOnLiveChannel[2] = { {0, 16, 0.0f}, {16, 16, 1.0f} };
	const Status StZeroWeight = ScoreXdPairSegmented(A, B, Pd, Metric::Cosine, ZeroWeightOnLiveChannel, 2, &Score);
	AssertExactValue(*this, TEXT("weight=0 on B's only live channel: status ZeroNormQuery"),
		static_cast<int32>(StZeroWeight), static_cast<int32>(Status::ZeroNormQuery));

	return true;
}

// ===========================================================================
// dim 8: ScoreXdPairSegmented's own local segment-list validator agrees with
// `ValidateSegments` (validate.h) on their SHARED structural rules -- offset/length
// positivity, alignment-grid membership, ascending/non-overlapping order, in-bounds
// ending, weight finiteness -- excluding the two deliberate departures dim 8's own text
// names (segmentCount==0, and negative-weight strictness), which get their own cells
// above and are not asserted as "agreement" here.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSScoreXdPairSegmentedAgreesWithValidateSegmentsTest,
	"SuperFAISS.D.ScoreXdPairSegmented.AgreesWithValidateSegmentsOnSharedRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSScoreXdPairSegmentedAgreesWithValidateSegmentsTest::RunTest(const FString& Parameters)
{
	// ValidateSegments (validate.h:29-33) takes a BankView + a PADDED QUERY float array --
	// a different call shape from ScoreXdPairSegmented's own XdQuery-pair signature (the
	// plan's own analytics.h comment states this exactly: "Validates locally, not via
	// ValidateSegments (which takes a BankView/paddedQuery this primitive has neither)").
	// Both are built here from the same underlying row content so the two functions are
	// validating the identical geometry.
	constexpr int32 kDims = 32;
	const int32 Pd = PaddedDims(kDims, Quantization::Int8);

	TArray<float> QueryRow;
	QueryRow.SetNumZeroed(Pd);
	QueryRow[0] = 6.0f; QueryRow[16] = 8.0f;

	TArray<uint8, TAlignedHeapAllocator<16>> BankPayload;
	BankPayload.SetNumZeroed(Pd);
	float BankScale = 0.0f;
	TArray<float> BankRow;
	BankRow.SetNumZeroed(kDims);
	BankRow[1] = 3.0f; BankRow[17] = 4.0f;
	QuantizeRowsInt8(BankRow.GetData(), 1, kDims, Pd, reinterpret_cast<int8_t*>(BankPayload.GetData()), &BankScale);

	BankView Bank;
	Bank.rows = BankPayload.GetData();
	Bank.scales = &BankScale;
	Bank.count = 1;
	Bank.dims = kDims;
	Bank.paddedDims = Pd;
	Bank.quant = Quantization::Int8;
	Bank.metric = Metric::Dot;
	Bank.channels = nullptr;
	Bank.channelCount = 0;

	XdQuery BankRowAsQuery;
	BankRowAsQuery.q8 = reinterpret_cast<const int8_t*>(BankPayload.GetData());
	BankRowAsQuery.scale = static_cast<double>(BankScale);
	int64_t Sq = 0;
	for (int32 d = 0; d < Pd; ++d)
	{
		const int64_t v = static_cast<int8_t>(BankPayload[d]);
		Sq += v * v;
	}
	BankRowAsQuery.sqSum = Sq;

	float ScaleQ = 0.0f;
	TArray<uint8, TAlignedHeapAllocator<16>> QueryPayload;
	QueryPayload.SetNumZeroed(Pd);
	QuantizeRowsInt8(QueryRow.GetData(), 1, kDims, Pd, reinterpret_cast<int8_t*>(QueryPayload.GetData()), &ScaleQ);
	XdQuery QueryAsXdQuery;
	QueryAsXdQuery.q8 = reinterpret_cast<const int8_t*>(QueryPayload.GetData());
	QueryAsXdQuery.scale = static_cast<double>(ScaleQ);
	int64_t SqQ = 0;
	for (int32 d = 0; d < Pd; ++d)
	{
		const int64_t v = static_cast<int8_t>(QueryPayload[d]);
		SqQ += v * v;
	}
	QueryAsXdQuery.sqSum = SqQ;

	struct FCase { const TCHAR* Name; QuerySegment Segs[2]; int32 Count; };
	const FCase Cases[] = {
		{ TEXT("well-formed"), { {0, 16, 1.0f}, {16, 16, 1.0f} }, 2 },
		{ TEXT("off-grid offset"), { {3, 16, 1.0f}, {0,0,0} }, 1 },
		{ TEXT("unsorted"), { {16, 16, 1.0f}, {0, 16, 1.0f} }, 2 },
		{ TEXT("overlapping"), { {0, 20, 1.0f}, {16, 16, 1.0f} }, 2 },
		{ TEXT("out of bounds"), { {16, 32, 1.0f}, {0,0,0} }, 1 },
	};
	float Score = 0.0f;
	for (const FCase& C : Cases)
	{
		const Status ScoreStatus = ScoreXdPairSegmented(QueryAsXdQuery, BankRowAsQuery, Pd, Metric::Dot,
			C.Segs, C.Count, &Score);
		const Status ValidateStatus = ValidateSegments(Bank, QueryRow.GetData(), C.Segs, C.Count);
		const bool bScoreRejects = (ScoreStatus == Status::InvalidArgument);
		const bool bValidateRejects = (ValidateStatus == Status::InvalidArgument);
		AssertExactValue(*this, FString::Printf(TEXT("%s: ScoreXdPairSegmented and ValidateSegments agree on rejection"), C.Name),
			bScoreRejects, bValidateRejects);
	}
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
