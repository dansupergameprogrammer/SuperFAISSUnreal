// SuperFAISS For Unreal 3.4 -- diversity oracle fixtures (the 3.4 drift-and-diversity plan
//  §9, §6.2, §11.2, §12 dims 1/2/4/5/6/7/8/10/11).
//
// Every expected value below is the panel's REAL query path as the plan specifies it, derived
// independently of the panel by one standalone probe against the plugin's vendored core
// (T-3008): a standalone derivation probe
// , output `t3008_panel_path_probe_output.txt` (cited per fixture
// below as "probe §N"). That probe bakes each bank the way InitFromSource does, queries the
// queried row's own centroid through the core Query (Exactness::PerDevice, the queried row
// excluded -- D-SLM7837 -- and k = K x 4, §9.3), takes each candidate's relevance from the pool's
// own Hit.score and its redundancy payload from its own row (§6.2; lifted with QuantizeQueryXd
// on a Float32 bank, D-SLM7841), runs the real SelectDiverseMMR, and cross-checks every output
// bit against an independent reference MMR written from §6.2's formulas. It also runs, on the
// same inputs, the mutants each cell exists to catch (§12 dim 7/11, the included-query-row
// build, a stale or channel-scoped L, the missing K clamp) and confirms each changes at least
// one asserted line.
//
// Every query in the governed suite uses kQueryRow0 ("#0" -- the index form; bare text is a
// row id and these fixture banks carry no ids, D-SLM7826). Candidate counts exclude the
// queried row (D-SLM7837). SelectedIndices are pool positions; the pool is in relevance
// order, so the lambda = 1 identity (§9.4) is always {0, 1, ..., K-1}.
//
// Float32 fixtures (D-SLM7840/7841) are per-device with no bit-identity claim. Their geometry
// is built so that every value asserted is exact in float/double arithmetic whatever the
// summation order (integer coordinates; Cosine rows of four unit entries, which normalize to
// exact 0.5s), so the asserted values do not depend on this device. Where that construction is
// impossible (the channelled Cosine bank: a channel sub-norm and the whole-row norm cannot
// both be powers of two) only the selection order is asserted, on a geometry whose smallest
// winner/runner-up margin is recorded in the probe output.
//
// Not production code: compiled only inside WITH_DEV_AUTOMATION_TESTS translation units.

#pragma once

#include "SuperFAISSVectorBank.h"
#include "Misc/AutomationTest.h"
#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "SuperFAISSDriftDiversityTestContracts.h"

namespace SuperFAISSDriftDiversityOracle
{
	// The query text every diversity cell runs: row 0 by index (D-SLM7826).
	inline const TCHAR* const kQueryRow0 = TEXT("#0");

	// A bank baked through the production InitFromSource path at any metric/quantization, with
	// an optional channel table.
	inline USuperFAISSVectorBank* BakeMetricAsset(FAutomationTestBase& Test, const TArray<float>& Rows,
		int32 RowCount, int32 Dims, ESuperFAISSBankMetric Metric, ESuperFAISSBankQuantization Quantization,
		const FString& DebugName, const TArray<FName>& ChannelNames = {}, const TArray<int32>& ChannelOffsets = {},
		const TArray<int32>& ChannelLengths = {})
	{
		USuperFAISSVectorBank* Bank = NewObject<USuperFAISSVectorBank>();
		FString Error;
		const bool bOk = Bank->InitFromSource(Rows, RowCount, Dims, Metric, Quantization, {}, DebugName, Error,
			ChannelNames, ChannelOffsets, ChannelLengths);
		AssertExactValue(Test, FString::Printf(TEXT("(setup) %s bakes"), *DebugName), bOk, true);
		return bOk ? Bank : nullptr;
	}

	inline USuperFAISSVectorBank* BakeMetricInt8Asset(FAutomationTestBase& Test, const TArray<float>& Rows,
		int32 RowCount, int32 Dims, ESuperFAISSBankMetric Metric, const FString& DebugName)
	{
		return BakeMetricAsset(Test, Rows, RowCount, Dims, Metric, ESuperFAISSBankQuantization::Int8, DebugName);
	}

	// Two-channel variant (chanA [0, ChanLen), chanB [ChanLen, 2*ChanLen)), for fixtures that
	// drive `SetChannelWeightForTest`.
	inline USuperFAISSVectorBank* BakeMetricInt8AssetTwoChannels(FAutomationTestBase& Test, const TArray<float>& Rows,
		int32 RowCount, int32 Dims, int32 ChanLen, ESuperFAISSBankMetric Metric, const FString& DebugName)
	{
		return BakeMetricAsset(Test, Rows, RowCount, Dims, Metric, ESuperFAISSBankQuantization::Int8, DebugName,
			{TEXT("chanA"), TEXT("chanB")}, {0, ChanLen}, {ChanLen, ChanLen});
	}

	// Asserts one displayed selection (order, relevance, redundancy) against its expected
	// arrays, exactly, and that neither refusal state is set. A null value array skips that
	// array's values (its count is still asserted).
	inline void AssertSelection(FAutomationTestBase& Test, const FString& Label, const FSuperFAISSMMRSelectionForTest& Actual,
		int32 Count, const int32* ExpectedOrder, const float* ExpectedRelevance, const float* ExpectedRedundancy)
	{
		AssertExactValue(Test, Label + TEXT(": no mid-selection refusal"), Actual.bMidSelectionRefusal, false);
		AssertExactValue(Test, Label + TEXT(": no whole-panel refusal"), Actual.bWholePanelRefusal, false);
		AssertExactValue(Test, Label + TEXT(": selection count"), Actual.SelectedIndices.Num(), Count);
		AssertExactValue(Test, Label + TEXT(": relevance count"), Actual.Relevance.Num(), Count);
		AssertExactValue(Test, Label + TEXT(": redundancy count"), Actual.Redundancy.Num(), Count);
		if (Actual.SelectedIndices.Num() != Count || Actual.Relevance.Num() != Count || Actual.Redundancy.Num() != Count)
		{
			return;
		}
		for (int32 i = 0; i < Count; ++i)
		{
			AssertExactValue(Test, FString::Printf(TEXT("%s: order step%d"), *Label, i), Actual.SelectedIndices[i], ExpectedOrder[i]);
			if (ExpectedRelevance != nullptr)
			{
				AssertExactValue(Test, FString::Printf(TEXT("%s: relevance step%d"), *Label, i), Actual.Relevance[i], ExpectedRelevance[i]);
			}
			if (ExpectedRedundancy != nullptr)
			{
				AssertExactValue(Test, FString::Printf(TEXT("%s: redundancy step%d"), *Label, i), Actual.Redundancy[i], ExpectedRedundancy[i]);
			}
		}
	}

	// The lambda = 1 identity order for up to 4 selections (§9.4): the pool's own order.
	inline constexpr int32 kIdentityOrder[4] = { 0, 1, 2, 3 };

	// ===================================================================================
	// L2ChannellessDiversityFixture -- 5 one-hot rows (kDims=5, the Gate 0d L2 geometry),
	// doubling as the primary bank (for L = sqrt(Spread(current))) and the candidate pool.
	// Queried from row 0; the pool is rows 2, 4, 1, 3 (Hit.score 125, 164, 200, 325). L =
	// 9.06863976. Probe §1.
	// ===================================================================================
	namespace L2ChannellessDiversityFixture
	{
		inline constexpr int32 kDims = 5;
		inline constexpr int32 kRows = 5;
		inline constexpr int32 kCandidates = kRows - 1;
		inline constexpr float kMag[kRows] = { 10.0f, 10.0f, 5.0f, 15.0f, 8.0f };

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			for (int32 i = 0; i < kRows; ++i) { OutRows[i * kDims + i] = kMag[i]; }
		}

		// lambda = 1, K = 4: the identity order, with the kernel's displayed values.
		inline constexpr float kLambda1Relevance[kCandidates] = { -0.232857421f, -0.412146568f, -0.559454978f, -0.987922847f };
		inline constexpr float kLambda1Redundancy[kCandidates] = { 0.0f, -0.0402862392f, -0.322501987f, -0.868679583f };

		// lambda = 0.5, K = 4: diversity reorders the pool. Redundancy at step 0 is 0 by
		// convention (§6.2). Every §12 dim 7 L2 mutant (sqrt-omission, L-omission, and both
		// half-applied transforms) and the included-query-row build change an asserted line
		// (probe §1).
		inline constexpr float kLambda = 0.5f;
		inline constexpr int32 kLambda05Order[kCandidates] = { 0, 3, 2, 1 };
		inline constexpr float kLambda05Relevance[kCandidates] = { -0.232857421f, -0.987922847f, -0.559454978f, -0.412146568f };
		inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, -0.743523777f, -0.610390127f, -0.442341626f };
	}

	// ===================================================================================
	// L2ReselectBankFixture -- a second L2 bank with a different Spread(current) (one-hot
	// {16, 9, 24, 13}, the Gate 0d below-onset bank's geometry), for the cache-recompute cell
	// (§12 dim 1). Queried from row 0 at K = 3, lambda = 0.5: L = 14.2434435. Run with the
	// L2ChannellessDiversityFixture bank's L (9.06863976) instead -- the stale-cache build --
	// every line changes (probe §6).
	// ===================================================================================
	namespace L2ReselectBankFixture
	{
		inline constexpr int32 kDims = 4;
		inline constexpr int32 kRows = 4;
		inline constexpr int32 kCandidates = kRows - 1;
		inline constexpr float kMag[kRows] = { 16.0f, 9.0f, 24.0f, 13.0f };

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			for (int32 i = 0; i < kRows; ++i) { OutRows[i * kDims + i] = kMag[i]; }
		}

		inline constexpr float kLambda = 0.5f;
		inline constexpr int32 kLambda05Order[kCandidates] = { 0, 2, 1 };
		inline constexpr float kLambda05Relevance[kCandidates] = { -0.288842797f, -1.02510095f, -0.447369665f };
		inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, -0.799565613f, -0.51319015f };
	}

	// ===================================================================================
	// L2DiversityFixtureBelowOnsetPanel -- the below-onset resolution-collapse fixture
	// (§12 dim 4's shape extreme, §11.4 row 12), built by
	// a standalone derivation probe so a real
	// quantized bank carries two candidates whose float32(1 - sqrt(x)/L) collide while their
	// x stay distinct; that probe also executed the resolution-collapse mutant (order {1,0,2,3}).
	// Row 0 is the query; rows 1-4 are the pool. The closer of the colliding pair is bank row 2
	// (the larger index), so a build that ties on the collapsed float picks row 1 first.
	//
	// Re-derived on the real query path by probe §5: pool rows 2, 1, 3, 4 (Hit.score 4.05578566,
	// 4.05578947, 3700, 6500), L = 40.2309685, both colliding candidates below the onset
	// (u < 0.706631), their displayed relevance equal and their scores distinct. Step 2's
	// relevance is -0.511960268: the earlier constant (-0.511960328) came from scoring the
	// candidate with ScoreXdPairSegmented in place of the query's own Hit.score and is 1 ULP
	// off the real path.
	// ===================================================================================
	namespace L2DiversityFixtureBelowOnsetPanel
	{
		inline constexpr int32 kDims = 4;
		inline constexpr int32 kRows = 5; // row0 = query, rows1-4 = candidates
		inline constexpr int32 kCandidates = kRows - 1;
		inline constexpr float kQueryMag = 10.0f;
		inline constexpr float kFartherMag = 7.98610115f;  // row1 (bank index 1) -- x=4.05578947
		inline constexpr float kCloserMag = 7.98610163f;   // row2 (bank index 2) -- x=4.05578566
		inline constexpr float kFillerMag1 = 60.0f;         // row3 (bank index 3), dim1
		inline constexpr float kFillerMag2 = 80.0f;         // row4 (bank index 4), dim2

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			OutRows[0 * kDims + 0] = kQueryMag;
			OutRows[1 * kDims + 0] = kFartherMag;
			OutRows[2 * kDims + 0] = kCloserMag;
			OutRows[3 * kDims + 1] = kFillerMag1;
			OutRows[4 * kDims + 2] = kFillerMag2;
		}

		// lambda = 1: the identity order; the displayed values per selection step.
		inline constexpr float kExpectedRelevance[kCandidates] = { 0.949941576f, 0.949941576f, -0.511960268f, -1.00399292f };
		inline constexpr float kExpectedRedundancy[kCandidates] = { 0.0f, 1.0f, -0.504541218f, -1.16081667f };
	}

	// ===================================================================================
	// CosineChannellessDiversityFixture -- 5 rows, 8 dims (the Gate 0d Cosine geometry):
	// magnitude 10 on one chanA-half dim and one chanB-half dim, plus a deterministic
	// per-row/per-dim jitter. Queried from row 0; the pool is rows 1, 2, 3, 4 (row 1 shares
	// both of row 0's directions). Probe §2.
	// ===================================================================================
	namespace CosineChannellessDiversityFixture
	{
		inline constexpr int32 kDims = 8;
		inline constexpr int32 kRows = 5;
		inline constexpr int32 kCandidates = kRows - 1;
		inline constexpr int32 kDirA[kRows] = { 0, 0, 2, 0, 1 };
		inline constexpr int32 kDirB[kRows] = { 0, 0, 0, 1, 1 };

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			for (int32 i = 0; i < kRows; ++i)
			{
				OutRows[i * kDims + kDirA[i]] = 10.0f;
				OutRows[i * kDims + 4 + kDirB[i]] = 10.0f;
				for (int32 d = 0; d < kDims; ++d)
				{
					OutRows[i * kDims + d] += 0.01f * static_cast<float>(((i * 7 + d * 3 + 1) % 11) - 5);
				}
			}
		}

		inline constexpr float kLambda1Relevance[kCandidates] = { 1.00045443f, 0.497276664f, 0.493088722f, 0.0f };
		inline constexpr float kLambda1Redundancy[kCandidates] = { 0.0f, 0.497996628f, 0.243069977f, 0.164029106f };

		// lambda = 0.5: diversity reorders; the Cosine polarity mutant (raw 1 - cos as
		// redundancy) and the included-query-row build change an asserted line (probe §2).
		inline constexpr float kLambda = 0.5f;
		inline constexpr int32 kLambda05Order[kCandidates] = { 0, 3, 1, 2 };
		inline constexpr float kLambda05Relevance[kCandidates] = { 1.00045443f, 0.0f, 0.497276664f, 0.493088722f };
		inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, -0.00787365437f, 0.248998314f, 0.328700304f };
	}

	// ===================================================================================
	// DotChannellessDiversityFixture -- 5 rows, 8 dims: q = 10e0, A = 9e0 + 6e1, B = 8e0 + 6e1
	// (A's near-duplicate), C = 7e0 + 7e2, D = 5e0 + 7e3. The Gate 0d Dot bank had 3 rows -- 2
	// candidates once the queried row is excluded, too few for an order to differ -- so this
	// geometry replaces it. Queried from row 0; the pool is rows 1, 2, 3, 4. Probe §3.
	// ===================================================================================
	namespace DotChannellessDiversityFixture
	{
		inline constexpr int32 kDims = 8;
		inline constexpr int32 kRows = 5;
		inline constexpr int32 kCandidates = kRows - 1;

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			OutRows[0 * kDims + 0] = 10.0f;
			OutRows[1 * kDims + 0] = 9.0f; OutRows[1 * kDims + 1] = 6.0f;
			OutRows[2 * kDims + 0] = 8.0f; OutRows[2 * kDims + 1] = 6.0f;
			OutRows[3 * kDims + 0] = 7.0f; OutRows[3 * kDims + 2] = 7.0f;
			OutRows[4 * kDims + 0] = 5.0f; OutRows[4 * kDims + 3] = 7.0f;
		}

		inline constexpr float kLambda1Relevance[kCandidates] = { 90.0000076f, 80.0f, 70.0f, 50.1574783f };
		inline constexpr float kLambda1Redundancy[kCandidates] = { 0.0f, 108.046875f, 59.5f, 40.1259842f };

		// lambda = 0.5: the near-duplicate B drops to last.
		inline constexpr float kLambda = 0.5f;
		inline constexpr int32 kLambda05Order[kCandidates] = { 0, 2, 3, 1 };
		inline constexpr float kLambda05Relevance[kCandidates] = { 90.0000076f, 70.0f, 50.1574783f, 80.0f };
		inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, 63.0000038f, 40.1259842f, 68.0576172f };
	}

	// ===================================================================================
	// ChannelWeightFixture -- one 4-row bank (row0 = query, rows 1..3 = candidates), 32 dims
	// (2 channels of 16), the Gate 0d channel-weight geometry. Driven at the sliders' default
	// weight (1.0, 1.0) and a non-default weight (0.5, 1.5) -- both sum to 2 (S != 1, the
	// Cosine fixed-1-recovery mutant's construction requirement, §12 dim 7). Shared across all
	// three metrics (the fixture is a geometry, baked at each metric in turn). Probe §4.
	//
	// On this geometry lambda = 0.5 keeps the relevance order, so the lambda = 1 and lambda = 0.5
	// displays are the same arrays. What the cell discriminates is the redundancy VALUE: at the
	// non-default weight the unweighted-redundancy mutant, on Cosine the fixed-1 recovery and
	// polarity mutants, on L2 every transform mutant and a channel-scoped L, and at both weights
	// the included-query-row build each change an asserted line (probe §4).
	// ===================================================================================
	namespace ChannelWeightFixture
	{
		inline constexpr int32 kChanLen = 16;
		inline constexpr int32 kDims = 2 * kChanLen; // 32
		inline constexpr int32 kRows = 4; // row0 query, rows1-3 candidates
		inline constexpr int32 kCandidates = kRows - 1;
		inline constexpr int32 kDirA[kRows] = { 0, 0, 1, 2 };
		inline constexpr int32 kDirB[kRows] = { 0, 0, 0, 2 };

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			for (int32 i = 0; i < kRows; ++i)
			{
				OutRows[i * kDims + kDirA[i]] = 10.0f;
				OutRows[i * kDims + kChanLen + kDirB[i]] = 10.0f;
				for (int32 d = 0; d < kDims; ++d)
				{
					OutRows[i * kDims + d] += 0.01f * static_cast<float>(((i * 11 + d * 5 + 2) % 13) - 6);
				}
			}
		}

		inline constexpr float kLambda = 0.5f;

		struct FWeightSetting { float W0; float W1; bool bDefault; };
		inline constexpr FWeightSetting kDefaultWeight = { 1.0f, 1.0f, true };
		inline constexpr FWeightSetting kNonDefaultWeight = { 0.5f, 1.5f, false };

		// Per metric, per weight setting: relevance and redundancy per selection step, in the
		// identity order {0, 1, 2}, at lambda = 1 and lambda = 0.5 alike.
		namespace L2
		{
			inline constexpr float kDefaultRelevance[kCandidates] = { 0.95818913f, -0.411833584f, -1.00485611f };
			inline constexpr float kDefaultRedundancy[kCandidates] = { 0.0f, -0.410154104f, -0.999454498f };
			inline constexpr float kNonDefaultRelevance[kCandidates] = { 0.95672828f, 0.00105945533f, -1.00683546f };
			inline constexpr float kNonDefaultRedundancy[kCandidates] = { 0.0f, 0.0024964849f, -0.995487809f };
		}
		namespace Dot
		{
			inline constexpr float kDefaultRelevance[kCandidates] = { 198.801605f, 100.253227f, 0.787398398f };
			inline constexpr float kDefaultRedundancy[kCandidates] = { 0.0f, 100.089256f, 1.55966032f };
			inline constexpr float kNonDefaultRelevance[kCandidates] = { 198.795441f, 149.608688f, 0.393699199f };
			inline constexpr float kNonDefaultRedundancy[kCandidates] = { 0.0f, 149.35199f, 2.34607267f };
		}
		namespace Cosine
		{
			inline constexpr float kDefaultRelevance[kCandidates] = { 1.99913239f, 1.00712693f, 0.00787059963f };
			inline constexpr float kDefaultRedundancy[kCandidates] = { 0.0f, 1.00749874f, 0.0156172514f };
			inline constexpr float kNonDefaultRelevance[kCandidates] = { 1.99907041f, 1.50294375f, 0.00393529981f };
			inline constexpr float kNonDefaultRedundancy[kCandidates] = { 0.0f, 1.50337756f, 0.0234878063f };
		}
	}

	// ===================================================================================
	// Zero-scale boundary (§6.2's guard, §12 dims 2/5/11). Probe §7.
	//   - ZeroScaleIdenticalRows: 3 identical L2 rows (10, 5, 0, 0): Spread(current) = 0,
	//     L = 0. At lambda < 1 the whole panel refuses and the plain ranking renders; at
	//     lambda = 1 nothing refuses (§9.1a gates the refusal on lambda < 1) and the same plain
	//     ranking renders. Pool Hit.score is 1.38777878e-15 for both candidates (the per-device
	//     score of a dequantized row against its own copy).
	//   - SmallNonzeroScale: 4 rows (10, 0, 0, 0) with 0.1 added on dims 1/2 of rows 1-3:
	//     L = 0.0787401572 > 0, so the kernel runs and every value is finite.
	// ===================================================================================
	namespace ZeroScaleIdenticalRows
	{
		inline constexpr int32 kDims = 4;
		inline constexpr int32 kRows = 3;
		inline constexpr int32 kCandidates = kRows - 1;

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			for (int32 i = 0; i < kRows; ++i) { OutRows[i * kDims + 0] = 10.0f; OutRows[i * kDims + 1] = 5.0f; }
		}

		inline constexpr float kPlainRelevance[kCandidates] = { 1.38777878e-15f, 1.38777878e-15f };
		inline constexpr float kPlainRedundancy[kCandidates] = { 0.0f, 0.0f };
	}

	namespace SmallNonzeroScale
	{
		inline constexpr int32 kDims = 4;
		inline constexpr int32 kRows = 4;
		inline constexpr int32 kCandidates = kRows - 1;

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			for (int32 i = 0; i < kRows; ++i) { OutRows[i * kDims + 0] = 10.0f; }
			OutRows[1 * kDims + 1] = 0.1f;
			OutRows[2 * kDims + 2] = 0.1f;
			OutRows[3 * kDims + 1] = 0.1f;
			OutRows[3 * kDims + 2] = 0.1f;
		}

		inline constexpr float kLambda = 0.5f;
		inline constexpr int32 kLambda05Order[kCandidates] = { 0, 1, 2 };
		inline constexpr float kLambda05Relevance[kCandidates] = { 5.52972779e-09f, 5.52972779e-09f, -0.414213568f };
		inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, -0.414213568f, 5.52972779e-09f };
	}

	// §6.2's zero-scale refusal line, verbatim from the plan (the independent source; the
	// production copy is SSuperFAISSBankInspector::DiversityL2ZeroScaleRefusalNote()).
	inline const TCHAR* const kExpectedZeroScaleRefusalText =
		TEXT("diversity is not available: this bank has no internal variation to measure candidates against "
			"(single live row, or all live rows identical).");

	// §9.5a's one shared mid-selection note, verbatim from the plan.
	inline const TCHAR* const kExpectedMidSelectionNote =
		TEXT("Diversity unavailable for this query: could not evaluate how similar the "
			"results are to each other. Showing the plain relevance-ranked results instead.");

	// ===================================================================================
	// Mid-selection refusal fixtures (§9.5a, §12 dims 5/10/11). Both triggers need a
	// redundancy evaluation, which SelectDiverseMMR first makes at step 1 -- so each bank
	// carries two candidates once the queried row is excluded, and every cell runs K = 2
	// (D-SLM7829: the earlier K = 1 on a 2-row bank never evaluated redundancy). Probe §8/§9.
	//
	// WeightedZeroNorm (Cosine, 2 x 16 channels, weights chanA 0 / chanB 1): row 0 the query
	// (live on both channels), row 1 live on both, row 2 live on chanA only -- its weighted
	// self-norm is exactly 0, so scoring it against row 1 at step 1 returns ZeroNormQuery.
	// Pool: rows 1, 2 (Hit.score 0.892540276, 0).
	//
	// OrdinaryChannelled (Cosine, 2 x 16 channels, default weights): any well-formed bank;
	// the malformed (unsorted) segment list injected through SetDiversitySegmentOverrideForTest
	// makes step 1's ScoreXdPairSegmented return InvalidArgument. Pool: rows 1, 2 (Hit.score
	// 1.58723521, 0.286655754). With no override the kernel runs: order {0, 1}, redundancy
	// {0, 1.41421354}.
	// ===================================================================================
	namespace MidSelectionRefusalFixtures
	{
		inline constexpr int32 kChanLen = 16;
		inline constexpr int32 kDims = 2 * kChanLen;
		inline constexpr int32 kRows = 3;
		inline constexpr int32 kCandidates = kRows - 1;

		inline void BuildWeightedZeroNormRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			OutRows[0 * kDims + 0] = 10.0f; OutRows[0 * kDims + kChanLen] = 10.0f;
			OutRows[1 * kDims + 0] = 8.0f; OutRows[1 * kDims + 1] = 4.0f;
			OutRows[1 * kDims + kChanLen] = 6.0f; OutRows[1 * kDims + kChanLen + 1] = 3.0f;
			OutRows[2 * kDims + 0] = 10.0f; OutRows[2 * kDims + 2] = 5.0f;
		}
		inline constexpr float kWeightedZeroNormFallbackRelevance[kCandidates] = { 0.892540276f, 0.0f };

		inline void BuildOrdinaryChannelledRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			OutRows[0 * kDims + 0] = 10.0f; OutRows[0 * kDims + 1] = 3.0f; OutRows[0 * kDims + kChanLen] = 8.0f;
			OutRows[1 * kDims + 0] = 6.0f; OutRows[1 * kDims + 1] = 6.0f;
			OutRows[1 * kDims + kChanLen] = 4.0f; OutRows[1 * kDims + kChanLen + 1] = 4.0f;
			OutRows[2 * kDims + 1] = 9.0f; OutRows[2 * kDims + kChanLen + 1] = 7.0f;
		}
		inline constexpr float kOrdinaryFallbackRelevance[kCandidates] = { 1.58723521f, 0.286655754f };
		inline constexpr int32 kOrdinaryLambda05Order[kCandidates] = { 0, 1 };
		inline constexpr float kOrdinaryLambda05Relevance[kCandidates] = { 1.58723521f, 0.286655754f };
		inline constexpr float kOrdinaryLambda05Redundancy[kCandidates] = { 0.0f, 1.41421354f };

		// The §9.5a fallback redundancy: 0 throughout (nothing computed).
		inline constexpr float kZeroRedundancy[kCandidates] = { 0.0f, 0.0f };
	}

	// ===================================================================================
	// TieBreakFixture -- 3 L2 rows: row 0 = 10e0 (the query), rows 1 and 2 = 6e1 and 6e2, a
	// genuine Hit.score tie (136 and 136). The pool ties on ascending bank row (row 1 first)
	// and so does SelectDiverseMMR (§6.2: ties on Hit.index), so the order is {0, 1} at every
	// lambda. Probe §10.
	// ===================================================================================
	namespace TieBreakFixture
	{
		inline constexpr int32 kDims = 3;
		inline constexpr int32 kRows = 3;
		inline constexpr int32 kCandidates = kRows - 1;

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			OutRows[0 * kDims + 0] = 10.0f;
			OutRows[1 * kDims + 1] = 6.0f;
			OutRows[2 * kDims + 2] = 6.0f;
		}

		inline constexpr float kLambda = 0.5f;
		inline constexpr int32 kOrder[kCandidates] = { 0, 1 };
		inline constexpr float kRelevance[kCandidates] = { -0.886301756f, -0.886301756f };
		inline constexpr float kRedundancy[kCandidates] = { 0.0f, -0.372486115f };
	}

	// ===================================================================================
	// HostileKBankFixture -- 150 L2 rows x 8 dims, more live rows than kHardQueryKCap (100),
	// so a K of kHardQueryKCap + 1 is distinguishable from its clamped value: the clamped
	// build returns exactly 100 selections, a build without the clamp returns 101 (probe §1b).
	// ===================================================================================
	namespace HostileKBankFixture
	{
		inline constexpr int32 kDims = 8;
		inline constexpr int32 kRows = 150;

		inline void BuildRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			for (int32 i = 0; i < kRows; ++i)
			{
				OutRows[i * kDims + (i % kDims)] = 1.0f + static_cast<float>(i / kDims);
				OutRows[i * kDims + ((i + 3) % kDims)] = 2.0f;
			}
		}
	}

	// ===================================================================================
	// GappedChannelFixture -- chanA [0,16), a GAP [16,32) no channel names, chanB [32,48);
	// weights chanA 1 / chanB 0 (the weight-0 segment G-19 requires). Two banks differ only in
	// the gap (all zero, or filled with 1.0 + 0.5 * ((d - 16 + 3r) mod 16) <= 8.5). Every row's
	// max |value| is 10 and sits in a channel, so the gap fill leaves every row's int8 scale --
	// and every channel value's quantization -- unchanged (D-SLM7831). K = 3, lambda = 0.5.
	// Probe §12.
	//   - Dot: no bank-intrinsic scale, so the two banks display identical order, relevance and
	//     redundancy -- the gap is read by neither operand.
	//   - L2: L = sqrt(Spread(current)) is whole-row by §6.2, so the gap moves L (9.05147362 ->
	//     12.5400553) and with it every displayed value, although the pool scores are identical.
	//     Each bank's own values are asserted; at the gap-zero bank's L the gap-filled bank
	//     reproduces the gap-zero values exactly (probe §12).
	//   A build that scores the gap in the redundancy operand only changes an asserted line on
	//   both metrics (probe §12).
	// ===================================================================================
	namespace GappedChannelFixture
	{
		inline constexpr int32 kChanLen = 16;
		inline constexpr int32 kDims = 48;
		inline constexpr int32 kRows = 4;
		inline constexpr int32 kCandidates = kRows - 1;

		inline void BuildRows(bool bFillGap, TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRows * kDims);
			OutRows[0 * kDims + 0] = 10.0f; OutRows[0 * kDims + 1] = 5.0f; OutRows[0 * kDims + 32] = 10.0f;
			OutRows[1 * kDims + 0] = 8.0f;  OutRows[1 * kDims + 1] = 10.0f; OutRows[1 * kDims + 33] = 6.0f;
			OutRows[2 * kDims + 0] = 10.0f; OutRows[2 * kDims + 2] = 7.0f; OutRows[2 * kDims + 34] = 9.0f;
			OutRows[3 * kDims + 1] = 10.0f; OutRows[3 * kDims + 2] = 4.0f; OutRows[3 * kDims + 32] = 5.0f;
			if (bFillGap)
			{
				for (int32 r = 0; r < kRows; ++r)
				{
					for (int32 d = 16; d < 32; ++d)
					{
						OutRows[r * kDims + d] = 1.0f + 0.5f * static_cast<float>((d - 16 + 3 * r) % 16);
					}
				}
			}
		}

		inline constexpr float kLambda = 0.5f;
		inline constexpr int32 kOrder[kCandidates] = { 0, 1, 2 };
		inline constexpr float kDotRelevance[kCandidates] = { 130.708664f, 100.0f, 50.3936996f };
		inline constexpr float kDotRedundancy[kCandidates] = { 0.0f, 80.3149567f, 64.0709305f };
		inline constexpr float kL2GapZeroRelevance[kCandidates] = { 0.410379618f, 0.0463807434f, -0.310630769f };
		inline constexpr float kL2GapZeroRedundancy[kCandidates] = { 0.0f, -0.366487205f, -0.294523239f };
		inline constexpr float kL2GapFilledRelevance[kCandidates] = { 0.574409127f, 0.311672926f, 0.0539802648f };
		inline constexpr float kL2GapFilledRedundancy[kCandidates] = { 0.0f, 0.0136628095f, 0.065606758f };
	}

	// ===================================================================================
	// Float32 diversity fixtures (D-SLM7840/7841). Each bank has 4 rows (row 0 the query) so
	// the Float32 L -- the mean squared distance to the live centroid, in double -- divides by
	// a power of two and is exact. Probe §7 (zero scale), §8 (refusal), §13, §14.
	// ===================================================================================
	namespace Float32DiversityFixtures
	{
		inline constexpr int32 kRows = 4;
		inline constexpr int32 kCandidates = kRows - 1;
		inline constexpr float kLambda = 0.5f;

		// L2, 4 dims: q = 0, A = 3e0 (x 9), B = 3e0 + 1e1 (x 10, A's near-duplicate), C = 4e2
		// (x 16). L = 2.33184481. At lambda = 0.5 the near-duplicate drops to last. Every §12
		// dim 7 L2 mutant and the included-query-row build change an asserted line.
		namespace L2Channelless
		{
			inline constexpr int32 kDims = 4;
			inline void BuildRows(TArray<float>& OutRows)
			{
				OutRows.Reset();
				OutRows.SetNumZeroed(kRows * kDims);
				OutRows[1 * kDims + 0] = 3.0f;
				OutRows[2 * kDims + 0] = 3.0f; OutRows[2 * kDims + 1] = 1.0f;
				OutRows[3 * kDims + 2] = 4.0f;
			}
			inline constexpr float kLambda1Relevance[kCandidates] = { -0.286535025f, -0.356126994f, -0.715380013f };
			inline constexpr float kLambda1Redundancy[kCandidates] = { 0.0f, 0.574531734f, -1.16512716f };
			inline constexpr int32 kLambda05Order[kCandidates] = { 0, 2, 1 };
			inline constexpr float kLambda05Relevance[kCandidates] = { -0.286535025f, -0.715380013f, -0.356126994f };
			inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, -1.144225f, -0.305748791f };
		}

		// Cosine, 16 dims, every row four unit entries (normalizes to exact 0.5s): q = e0..e3,
		// A = e0+e1+e2+e4 (cos 0.75), B = e0+e1+e4+e5 (cos 0.5, cos(A) 0.75), C = e3+e8+e9+e10
		// (cos 0.25, orthogonal to A and B).
		namespace CosineChannelless
		{
			inline constexpr int32 kDims = 16;
			inline void BuildRows(TArray<float>& OutRows)
			{
				OutRows.Reset();
				OutRows.SetNumZeroed(kRows * kDims);
				for (int32 d : { 0, 1, 2, 3 }) { OutRows[0 * kDims + d] = 1.0f; }
				for (int32 d : { 0, 1, 2, 4 }) { OutRows[1 * kDims + d] = 1.0f; }
				for (int32 d : { 0, 1, 4, 5 }) { OutRows[2 * kDims + d] = 1.0f; }
				for (int32 d : { 3, 8, 9, 10 }) { OutRows[3 * kDims + d] = 1.0f; }
			}
			inline constexpr float kLambda1Relevance[kCandidates] = { 0.75f, 0.5f, 0.25f };
			inline constexpr float kLambda1Redundancy[kCandidates] = { 0.0f, 0.75f, 0.0f };
			inline constexpr int32 kLambda05Order[kCandidates] = { 0, 2, 1 };
			inline constexpr float kLambda05Relevance[kCandidates] = { 0.75f, 0.25f, 0.5f };
			inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, 0.0f, 0.375f };
		}

		// Dot, 4 dims: q = 10e0, A = 9e0 + 6e1, B = 8e0 + 6e1, C = 7e0 + 7e2.
		namespace DotChannelless
		{
			inline constexpr int32 kDims = 4;
			inline void BuildRows(TArray<float>& OutRows)
			{
				OutRows.Reset();
				OutRows.SetNumZeroed(kRows * kDims);
				OutRows[0 * kDims + 0] = 10.0f;
				OutRows[1 * kDims + 0] = 9.0f; OutRows[1 * kDims + 1] = 6.0f;
				OutRows[2 * kDims + 0] = 8.0f; OutRows[2 * kDims + 1] = 6.0f;
				OutRows[3 * kDims + 0] = 7.0f; OutRows[3 * kDims + 2] = 7.0f;
			}
			inline constexpr float kLambda1Relevance[kCandidates] = { 90.0f, 80.0f, 70.0f };
			inline constexpr float kLambda1Redundancy[kCandidates] = { 0.0f, 108.046875f, 59.5f };
			inline constexpr int32 kLambda05Order[kCandidates] = { 0, 2, 1 };
			inline constexpr float kLambda05Relevance[kCandidates] = { 90.0f, 70.0f, 80.0f };
			inline constexpr float kLambda05Redundancy[kCandidates] = { 0.0f, 63.0f, 82.0234375f };
		}

		// Channelled with offsets OFF the int8 16-element grid: 40 dims, chanA [0,20), chanB
		// [20,40) (Float32 grid 4). The panel lifts each channel to its own 16-grid range
		// (D-SLM7841); a build that addressed the unlifted Float32 offsets would hand
		// SelectDiverseMMR an off-grid segment list and refuse mid-selection (probe §14).
		// Integer coordinates, tie-free at both weights: q = 4e0 + 4e20, A = 4e0 + 3e21,
		// B = 2e0 + 3e1 + 4e20, C = 3e0 + 1e2 + 2e20 + 2e22.
		namespace Channelled
		{
			inline constexpr int32 kDims = 40;
			inline constexpr int32 kChanLen = 20;
			inline void BuildRows(TArray<float>& OutRows)
			{
				OutRows.Reset();
				OutRows.SetNumZeroed(kRows * kDims);
				OutRows[0 * kDims + 0] = 4.0f; OutRows[0 * kDims + 20] = 4.0f;
				OutRows[1 * kDims + 0] = 4.0f; OutRows[1 * kDims + 21] = 3.0f;
				OutRows[2 * kDims + 0] = 2.0f; OutRows[2 * kDims + 1] = 3.0f; OutRows[2 * kDims + 20] = 4.0f;
				OutRows[3 * kDims + 0] = 3.0f; OutRows[3 * kDims + 2] = 1.0f;
				OutRows[3 * kDims + 20] = 2.0f; OutRows[3 * kDims + 22] = 2.0f;
			}
			inline constexpr float kDefaultW[2] = { 1.0f, 1.0f };
			inline constexpr float kNonDefaultW[2] = { 0.5f, 1.5f };

			// L2 (L = 2.78388214): lambda = 0.5 keeps the relevance order at both weights, so
			// the lambda = 1 and lambda = 0.5 displays are the same arrays.
			inline constexpr float kL2DefaultRelevance[kCandidates] = { -0.135923684f, -0.295152277f, -0.796053052f };
			inline constexpr float kL2DefaultRedundancy[kCandidates] = { 0.0f, -0.561885059f, -0.887756824f };
			inline constexpr float kL2NonDefaultRelevance[kCandidates] = { 0.0841890499f, -0.295152277f, -1.19970679f };
			inline constexpr float kL2NonDefaultRedundancy[kCandidates] = { 0.0f, -0.500672102f, -1.11451828f };

			// Dot.
			inline constexpr float kDotDefaultLambda1Relevance[kCandidates] = { 24.0f, 20.0f, 16.0f };
			inline constexpr float kDotDefaultLambda1Redundancy[kCandidates] = { 0.0f, 14.0787401f, 10.031496f };
			inline constexpr int32 kDotDefaultLambda05Order[kCandidates] = { 0, 2, 1 };
			inline constexpr float kDotDefaultLambda05Relevance[kCandidates] = { 24.0f, 16.0f, 20.0f };
			inline constexpr float kDotDefaultLambda05Redundancy[kCandidates] = { 0.0f, 8.0629921f, 13.0393696f };
			inline constexpr float kDotNonDefaultLambda1Relevance[kCandidates] = { 28.0f, 18.0f, 8.0f };
			inline constexpr float kDotNonDefaultLambda1Redundancy[kCandidates] = { 0.0f, 15.0708666f, 5.01574802f };
			inline constexpr int32 kDotNonDefaultLambda05Order[kCandidates] = { 0, 2, 1 };
			inline constexpr float kDotNonDefaultLambda05Relevance[kCandidates] = { 28.0f, 8.0f, 18.0f };
			inline constexpr float kDotNonDefaultLambda05Redundancy[kCandidates] = { 0.0f, 4.03149605f, 10.5354328f };

			// Cosine: order only (values inexact, see the header note). Smallest winner/
			// runner-up margin 0.0505 (default) and 0.0058 (non-default), probe §14.
			inline constexpr int32 kCosineDefaultLambda05Order[kCandidates] = { 0, 1, 2 };
			inline constexpr int32 kCosineNonDefaultLambda05Order[kCandidates] = { 0, 2, 1 };
		}

		// All four rows identical integers (3, 1, 0, 0): the Float32 L is exactly 0, so
		// lambda < 1 refuses the whole panel; the plain ranking renders (pool scores 0).
		namespace ZeroScale
		{
			inline constexpr int32 kDims = 4;
			inline void BuildRows(TArray<float>& OutRows)
			{
				OutRows.Reset();
				OutRows.SetNumZeroed(kRows * kDims);
				for (int32 i = 0; i < kRows; ++i) { OutRows[i * kDims + 0] = 3.0f; OutRows[i * kDims + 1] = 1.0f; }
			}
			inline constexpr float kPlainRelevance[kCandidates] = { 0.0f, 0.0f, 0.0f };
			inline constexpr float kPlainRedundancy[kCandidates] = { 0.0f, 0.0f, 0.0f };
		}
	}
}
