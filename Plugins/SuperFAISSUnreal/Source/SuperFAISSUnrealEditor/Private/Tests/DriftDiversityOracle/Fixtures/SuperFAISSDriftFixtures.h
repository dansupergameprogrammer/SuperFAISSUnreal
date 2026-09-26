// SuperFAISS For Unreal 3.4 -- drift oracle Fixtures A, B, C (the 3.4 drift-and-diversity plan
//  §11.1). Every geometry and every expected
// numeric constant below reproduces, verbatim, the hand-derivation and executed values
// already produced and checked at Gate 0c (MET) -- this header invents no new
// number; it re-expresses the fixture construction and its already-executed outputs as
// UE bank assets, so the oracle test in this directory can drive the real panel against
// them once Gate 6a exists. Sources:
//   - Geometry + Fixture A closed-form/executed table:
//     the test-design record §1.1-1.4.
//   - Fixture B/C's re-executed raw operator values (2026-08-08):
//     a standalone derivation probe,
//     `.../fixtureC_mutation_output.txt`, and the row geometry in
//     `.../fixtureB_mutation_probe.cpp`, `.../fixtureC_mutation_probe.cpp`.
//   - The composed, DISPLAYED expected ratios (the actual oracle assertion target, §8.3.1's
//     composition applied to the raw values above): the test-design record
//      rows 99-150 (the `Expected` column
//     of the `sqrt-omission-*`/`wrong-denominator` rows IS the correct composed value; the
//     `Actual` column is the named mutant's wrong output, not reused here).
//
// Every bank is `Quantization::Int8`, 32 dims (2 channels of 16 -- the Int8 16-ELEMENT grid
// correction §1.1 of the oracle test-design record states; the source 3.3.1 sidecar's 4-dim
// channels are a Float32 16-BYTE-grid artifact that does not carry over).
//
// Not production code: compiled only inside WITH_DEV_AUTOMATION_TESTS translation units.

#pragma once

#include "SuperFAISSVectorBank.h"
#include "Misc/AutomationTest.h"

namespace SuperFAISSDriftDiversityOracle
{
	constexpr int32 kChanLen = 16;
	constexpr int32 kChanCount = 2;
	constexpr int32 kDriftDims = kChanCount * kChanLen; // 32

	inline void AppendActiveRow(TArray<float>& Rows, int32 ChanADir, int32 ChanBDir, float Magnitude)
	{
		const int32 Base = Rows.Num();
		// SetNumZeroed sets the array's ABSOLUTE size, not a delta -- the fixed form grows
		// the array by kDriftDims (appending a new row), where the original
		// `SetNumZeroed(kDriftDims)` truncated it back to exactly kDriftDims elements on
		// every call after the first, corrupting every row from the second onward
		// (execution-confirmed crash, Gate 6a build:
		// "Array index out of bounds: 32 into an array of size 32" in
		// BuildBaselineRows() -- exactly the second row's write).
		Rows.SetNumZeroed(Base + kDriftDims);
		Rows[Base + ChanADir] = Magnitude;
		Rows[Base + kChanLen + ChanBDir] = Magnitude;
	}

	// The 22-row "Primary" direction scheme (TutorialBankGeometry.csv), re-baked to the
	// Int8 32-dim grid at magnitude 10 (oracle test-design record §1.1, `kDirs` table
	// reproduced from `fixtureB_mutation_probe.cpp:59-75`).
	inline constexpr int32 kBaselineRowCount = 22;
	inline constexpr int32 kBaselineDirs[kBaselineRowCount][2] = {
		{0,0},{0,0},{0,0},{0,0},
		{1,1},{1,1},{1,1},
		{2,0},{2,0},{2,0},
		{3,2},
		{0,1},
		{1,2},
		{3,3},
		{3,3},
		{0,0},
		{0,2},
		{2,3},
		{1,3},
		{3,0},
		{0,3},
		{3,1},
	};

	inline void BuildBaselineRows(TArray<float>& OutRows)
	{
		OutRows.Reset();
		OutRows.Reserve(kBaselineRowCount * kDriftDims);
		for (int32 i = 0; i < kBaselineRowCount; ++i)
		{
			AppendActiveRow(OutRows, kBaselineDirs[i][0], kBaselineDirs[i][1], 10.0f);
		}
	}

	inline USuperFAISSVectorBank* BakeL2Int8Asset(FAutomationTestBase& Test, const TArray<float>& Rows,
		int32 RowCount, const FString& DebugName)
	{
		const TArray<FName> ChannelNames = {TEXT("chanA"), TEXT("chanB")};
		const TArray<int32> ChannelOffsets = {0, kChanLen};
		const TArray<int32> ChannelLengths = {kChanLen, kChanLen};
		USuperFAISSVectorBank* Bank = NewObject<USuperFAISSVectorBank>();
		FString Error;
		const bool bOk = Bank->InitFromSource(Rows, RowCount, kDriftDims, ESuperFAISSBankMetric::L2,
			ESuperFAISSBankQuantization::Int8, {}, DebugName, Error, ChannelNames, ChannelOffsets, ChannelLengths);
		AssertExactValue(Test, FString::Printf(TEXT("(setup) %s bakes"), *DebugName), bOk, true);
		return bOk ? Bank : nullptr;
	}

	inline USuperFAISSVectorBank* BakeCosineInt8Asset(FAutomationTestBase& Test, const TArray<float>& Rows,
		int32 RowCount, const FString& DebugName)
	{
		const TArray<FName> ChannelNames = {TEXT("chanA"), TEXT("chanB")};
		const TArray<int32> ChannelOffsets = {0, kChanLen};
		const TArray<int32> ChannelLengths = {kChanLen, kChanLen};
		USuperFAISSVectorBank* Bank = NewObject<USuperFAISSVectorBank>();
		FString Error;
		const bool bOk = Bank->InitFromSource(Rows, RowCount, kDriftDims, ESuperFAISSBankMetric::Cosine,
			ESuperFAISSBankQuantization::Int8, {}, DebugName, Error, ChannelNames, ChannelOffsets, ChannelLengths);
		AssertExactValue(Test, FString::Printf(TEXT("(setup) %s bakes"), *DebugName), bOk, true);
		return bOk ? Bank : nullptr;
	}

	// ===================================================================================
	// Fixture A -- the closed-form perturbation (oracle test-design record §1.2). Baseline:
	// the 22-row scheme above. Current: rows 0-3 (the X1-X4 cluster, both dirs 0) rescaled
	// from magnitude 10 to 13. Fixture A asserts the FOUR RAW OPERATOR VALUES ONLY (§11.1's
	// own scoping -- Fixture A is deliberately not the fixture the composed-ratio
	// assertions run against), under the pre-existing, separately-governed quantization
	// tolerance (the one exception §11's assertion-convention paragraph names).
	// ===================================================================================
	namespace FixtureA
	{
		inline void BuildCurrentRows(TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.Reserve(kBaselineRowCount * kDriftDims);
			for (int32 i = 0; i < kBaselineRowCount; ++i)
			{
				const float Magnitude = (i < 4) ? 13.0f : 10.0f;
				AppendActiveRow(OutRows, kBaselineDirs[i][0], kBaselineDirs[i][1], Magnitude);
			}
		}

		// Executed (real operators, `fixtureAB_probe.cpp`, oracle test-design record §1.2's
		// "Executed" table). The quantization-rounding error bound the record states.
		inline constexpr float kExpectedMovement = 0.611961f;
		inline constexpr float kExpectedSpreadCurrent = 160.281830f;
		inline constexpr float kExpectedSpreadBaseline = 144.216019f;
		inline constexpr float kExpectedMeanNN = 3.272728f;
		inline constexpr float kExpectedMaxNN = 18.000006f;
		inline constexpr float kExpectedChanAMovement = 0.298183f;
		inline constexpr float kExpectedChanBMovement = 0.298641f;
		// §1.2's stated bound: whole-row/per-channel values within +/-0.02 absolute; NN
		// values (never requantize a pooled centroid) within +/-0.00001 absolute.
		inline constexpr float kWholeRowBound = 0.02f;
		inline constexpr float kNNBound = 0.00001f;

		// The tombstone-threading cell (§8.8): the current side is Fixture A's current rows
		// saved as a scratch archive with row 0 tombstoned, the baseline the 22-row scheme.
		// Movement = CentroidDistanceCrossDevice over the 21 live current rows, derived
		// independently of the panel by a standalone derivation probe
		//  Part 2 (the archive built through the core ScratchBank
		// exactly as the test builds it: Create, Append, Remove(0), Save, Load, Snapshot; the
		// snapshot's tombstone words threaded into excludeBitsA). Exact, not the quantization
		// bound above: it is the real int8 operator's output on the real archive. A build that
		// ignores the tombstone computes 0.611961246 (probe Part 2) and fails the cell.
		inline constexpr float kExpectedMovementRow0Tombstoned = 0.0822869167f;
	}

	// ===================================================================================
	// Fixture B -- Metric::L2, a population shift WITH a dispersion change (oracle
	// test-design record §1.3; re-executed 2026-08-08,
	// a standalone derivation probe). Current: every
	// baseline row transformed by the affine map v_current = C_base + s*(v_base - C_base)
	// + T, s=1.5, T=+12 on every active position (§7.1's own re-execution record). This is
	// the fixture the COMPOSED, DISPLAYED ratio assertions run against.
	// ===================================================================================
	namespace FixtureB
	{
		inline void BuildCurrentRows(TArray<float>& OutRows)
		{
			// v_current = C_base + s*(v_base - C_base) + T, applied per-active-position
			// (mirrors fixtureB_mutation_probe.cpp's BuildFixtureB exactly: centroid
			// computed from the baseline rows, s=1.5 applied to every dim, +12 added only
			// on the 8 "active" positions -- the 4 used directions of each 16-wide channel).
			TArray<float> Baseline;
			BuildBaselineRows(Baseline);
			double Centroid[kDriftDims] = {0.0};
			for (int32 r = 0; r < kBaselineRowCount; ++r)
			{
				for (int32 d = 0; d < kDriftDims; ++d)
				{
					Centroid[d] += Baseline[r * kDriftDims + d];
				}
			}
			for (int32 d = 0; d < kDriftDims; ++d)
			{
				Centroid[d] /= kBaselineRowCount;
			}
			constexpr double s = 1.5;
			constexpr double t = 12.0;
			OutRows.Reset();
			OutRows.SetNumZeroed(kBaselineRowCount * kDriftDims);
			for (int32 r = 0; r < kBaselineRowCount; ++r)
			{
				for (int32 d = 0; d < kDriftDims; ++d)
				{
					const bool bActive = (d % kChanLen) < 4;
					const double u = Baseline[r * kDriftDims + d] - Centroid[d];
					const double tt = bActive ? t : 0.0;
					OutRows[r * kDriftDims + d] = static_cast<float>(Centroid[d] + s * u + tt);
				}
			}
		}

		// Raw operator values, executed 2026-08-08 (fixtureB_mutation_output.txt:19-20).
		inline constexpr float kRawMovement = 1156.00293f;
		inline constexpr float kRawSpreadCurrentWhole = 324.998169f;
		inline constexpr float kRawSpreadBaselineWhole = 144.216019f;
		inline constexpr float kRawMeanNN = 1187.75439f;
		inline constexpr float kRawMaxNN = 1197.36133f;
		inline constexpr float kRawChanAMovement = 578.174744f;
		inline constexpr float kRawChanBMovement = 576.485168f;
		inline constexpr float kRawSpreadCurrentA = 165.042953f;
		inline constexpr float kRawSpreadCurrentB = 159.949615f;
		inline constexpr float kRawSpreadBaselineA = 73.1409302f;
		inline constexpr float kRawSpreadBaselineB = 71.0749054f;

		// The composed, DISPLAYED ratio §8.3.1 specifies for Metric::L2:
		// sqrt(Movement)/sqrt(Spread(current)) -- equivalently sqrt(Movement/Spread(current))
		// -- denominator Spread(current). These five values are the fixture's
		// own hand-derived expected DISPLAY values, reproduced verbatim from the mutation-
		// execution ledger's own `Expected` column (rows 99-103, the `sqrt-omission-l2`/
		// `wrong-denominator` mutant rows against FixtureB -- both mutants' `Expected`
		// column is the correct composed value; only the `Actual` (mutant) columns differ
		// and are not reused here).
		inline constexpr float kExpectedHeadlineRatio = 1.88598837f;
		inline constexpr float kExpectedTypicalRatio = 1.91171376f;
		inline constexpr float kExpectedWorstCaseRatio = 1.91942946f;
		inline constexpr float kExpectedChanARatio = 1.87167769f;
		inline constexpr float kExpectedChanBRatio = 1.89846445f;
	}

	// ===================================================================================
	// Fixture C -- Metric::Cosine, the bimodal-rotation composition fixture (oracle
	// test-design record §1.4; re-executed 2026-08-08). 25 rows, 2 channels of 16, bimodal
	// (20 rows at +u_k, 5 at -u_k per channel's own orthonormal (u_k,f_k) pair), +/-0.01
	// per-dim jitter, current = baseline rotated by beta=0.3 rad within each channel's own
	// plane. Reproduces `fixtureC_mutation_probe.cpp`'s geometry exactly (same jitter hash,
	// same channel frames, same beta).
	// ===================================================================================
	namespace FixtureC
	{
		inline constexpr int32 kRowCount = 25;
		inline constexpr int32 kMinorityRows = 5;
		inline constexpr double kBeta = 0.3;

		inline double Jitter(int32 Row, int32 Dim)
		{
			const uint32 h = static_cast<uint32>(Row * 2654435761u + Dim * 40503u);
			return 0.02 * (static_cast<double>(h % 1000) / 1000.0 - 0.5);
		}

		inline void ChannelFrame(int32 k, double* u, double* f)
		{
			for (int32 d = 0; d < kChanLen; ++d) { u[d] = 0.0; f[d] = 0.0; }
			static const double up[2][8] = {{1,1,1,1,2,0,1,1},{2,1,0,1,1,1,2,0}};
			static const double fp[2][8] = {{1,-1,1,-1,0,2,1,-1},{0,1,2,-1,1,-2,0,1}};
			double un = 0.0, fn = 0.0;
			for (int32 d = 0; d < 8; ++d) { un += up[k][d] * up[k][d]; fn += fp[k][d] * fp[k][d]; }
			un = FMath::Sqrt(un); fn = FMath::Sqrt(fn);
			for (int32 d = 0; d < 8; ++d) { u[d] = up[k][d] / un; f[8 + d] = fp[k][d] / fn; }
		}

		inline void BuildRows(double Beta, TArray<float>& OutRows)
		{
			OutRows.Reset();
			OutRows.SetNumZeroed(kRowCount * kDriftDims);
			const double cb = FMath::Cos(Beta), sb = FMath::Sin(Beta);
			for (int32 r = 0; r < kRowCount; ++r)
			{
				const double sign = (r < kRowCount - kMinorityRows) ? 1.0 : -1.0;
				for (int32 k = 0; k < kChanCount; ++k)
				{
					double u[kChanLen], f[kChanLen], v[kChanLen];
					ChannelFrame(k, u, f);
					for (int32 d = 0; d < kChanLen; ++d) { v[d] = sign * u[d] + Jitter(r, k * kChanLen + d); }
					double vu = 0.0, vf = 0.0;
					for (int32 d = 0; d < kChanLen; ++d) { vu += v[d] * u[d]; vf += v[d] * f[d]; }
					const double nu = cb * vu - sb * vf;
					const double nf = sb * vu + cb * vf;
					for (int32 d = 0; d < kChanLen; ++d)
					{
						const double rest = v[d] - vu * u[d] - vf * f[d];
						OutRows[r * kDriftDims + k * kChanLen + d] = static_cast<float>(rest + nu * u[d] + nf * f[d]);
					}
				}
			}
		}

		inline void BuildBaselineRows(TArray<float>& OutRows) { BuildRows(0.0, OutRows); }
		inline void BuildCurrentRows(TArray<float>& OutRows) { BuildRows(kBeta, OutRows); }

		// Raw operator values, executed 2026-08-08 (fixtureC_mutation_output.txt:19-20).
		inline constexpr float kRawMovement = 0.0442762524f;
		inline constexpr float kRawSpreadCurrentWhole = 0.400172204f;
		inline constexpr float kRawSpreadBaselineWhole = 0.400173128f;
		inline constexpr float kRawMeanNN = 0.0421571955f;
		inline constexpr float kRawMaxNN = 0.0464820266f;
		inline constexpr float kRawChanAMovement = 0.0433656834f;
		inline constexpr float kRawChanBMovement = 0.0446693711f;
		inline constexpr float kRawSpreadCurrentA = 0.400166959f;
		inline constexpr float kRawSpreadCurrentB = 0.400164455f;
		inline constexpr float kRawSpreadBaselineA = 0.40016216f;
		inline constexpr float kRawSpreadBaselineB = 0.400168687f;

		// The composed, DISPLAYED ratio §8.3.1 specifies for Metric::Cosine: no sqrt,
		// Movement/Spread(current). Reproduced verbatim from the mutation-execution
		// ledger's `Expected` column (rows 125-134).
		inline constexpr float kExpectedHeadlineRatio = 0.110642998f;
		inline constexpr float kExpectedTypicalRatio = 0.105347636f;
		inline constexpr float kExpectedWorstCaseRatio = 0.116155061f;
		inline constexpr float kExpectedChanARatio = 0.108368976f;
		inline constexpr float kExpectedChanBRatio = 0.111627533f;
	}
}
