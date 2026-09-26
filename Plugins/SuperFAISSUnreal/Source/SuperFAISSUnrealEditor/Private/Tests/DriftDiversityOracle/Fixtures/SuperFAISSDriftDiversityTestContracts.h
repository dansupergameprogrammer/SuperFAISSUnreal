// SuperFAISS For Unreal 3.4 -- the return-shape CONTRACTS §8.9/§9.7 of the 3.4 drift-and-diversity plan
//  specify for the drift/diversity test seams on
// `SSuperFAISSBankInspector` (`GetDriftResultForTest()`, built at Gate 6a;
// `GetLastMMRSelectionForTest()`, built at Gate 6b). This header states, in one place, the
// exact shape the governed test files in this directory are authored against, so the built
// seams and the tests agree on one definition rather than two independently maintained ones.
//
// This header is NOT itself the implementation: the seams live in
// `SSuperFAISSBankInspector.h`/`.cpp` and return exactly these types.
//
// Not production code: compiled only inside WITH_DEV_AUTOMATION_TESTS translation units.

#pragma once

#include "CoreMinimal.h"
#include "superfaiss/types.h" // superfaiss::QuerySegment

// §8.9's "the displayed set, not the operator set" -- one composed line per §8.3.2, plus the
// four raw operator values and both whole-row spread numbers, plus the per-channel table
// (movement/spread per channel, per P-2a), plus each line's own zero-denominator-refusal
// state (§8.3.3) and ZeroNormQuery-refusal state (§8.3.4), plus the whole-panel refusal state
// (quantization mismatch §8.4, Metric::Dot §8.4) and the identity block (§7.2, dim-7 G-31).
struct FSuperFAISSDriftPerChannelResultForTest
{
	FName ChannelName;
	float Movement = 0.0f;
	float SpreadCurrent = 0.0f;
	float SpreadBaseline = 0.0f;
	float ComposedRatio = 0.0f;
	bool bZeroDenominatorRefusal = false;
	bool bZeroNormQueryRefusal = false;
};

struct FSuperFAISSDriftIdentityRecordForTest
{
	FString PrimaryDisplayName;
	int32 PrimaryLiveRowCount = 0;
	int32 PrimaryDims = 0;
	FString PrimaryMetric;
	FString PrimaryQuantization;
	FString ComparisonDisplayName;
	int32 ComparisonLiveRowCount = 0;
	int32 ComparisonDims = 0;
	FString ComparisonMetric;
	FString ComparisonQuantization;
	bool bSelfComparison = false; // §8.7
};

struct FSuperFAISSDriftResultForTest
{
	// Whole-panel refusal states (§8.4). When either is set, no operator ran (asserted via
	// DriftChunksProcessedForTest == 0, §8.9) and every field below is default/unpopulated.
	bool bQuantizationRefusal = false;
	FString QuantizationRefusalText;
	bool bMetricDotRefusal = false;
	FString MetricDotRefusalText;

	// dim 3a (coverage audit G-3a): set when the compute was cancelled via
	// DebugCancelAfterChunks before completion (mirrors
	// FSuperFAISSInspectorCorrespondenceCancelTest's own status string, "cancelled").
	// Every field below is unpopulated (default) when this is set -- no partial result.
	bool bCancelled = false;

	// Whole-row headline (§8.1, §8.3.1).
	float Movement = 0.0f;
	float SpreadCurrent = 0.0f;
	float SpreadBaseline = 0.0f;
	float HeadlineRatio = 0.0f;
	bool bHeadlineZeroDenominatorRefusal = false;
	bool bHeadlineZeroNormQueryRefusal = false;

	// Worst-case / typical divergence (§8.1, §8.3.2).
	float MaxNN = 0.0f;
	float WorstCaseRatio = 0.0f;
	bool bWorstCaseZeroDenominatorRefusal = false;
	float MeanNN = 0.0f;
	float TypicalRatio = 0.0f;
	bool bTypicalZeroDenominatorRefusal = false;

	// Per-channel table (§8.1/P-2a, §8.3.2).
	TArray<FSuperFAISSDriftPerChannelResultForTest> Channels;

	// The shared identity block (§7.2, dim-7 G-31).
	FSuperFAISSDriftIdentityRecordForTest Identity;
};

// §9.7's diversity seam return -- the selected index order (positions within the pool
// SelectDiverseMMR was called with), the relevance/redundancy display arrays, the
// mid-selection refusal state (§9.5a), and the whole-panel refusal state (§6.2).
struct FSuperFAISSMMRSelectionForTest
{
	TArray<int32> SelectedIndices;
	TArray<float> Relevance;
	TArray<float> Redundancy;
	bool bMidSelectionRefusal = false; // §9.5a
	FString MidSelectionRefusalNoteText; // §9.5a's one shared note
	// §6.2's zero-scale guard (D-SLM7830): a WHOLE-PANEL refusal, decided before any
	// candidate pool is over-fetched -- SelectDiverseMMR is never called, the plain ranking
	// renders (SelectedIndices the pool's own prefix, Relevance each entry's pool score,
	// Redundancy 0), and the panel's one note slot shows the refusal line. Distinct from
	// bMidSelectionRefusal, which is set only when the kernel ran and refused mid-selection.
	// Populated from the same displayed state the note slot renders, never recomputed:
	// bWholePanelRefusal mirrors the panel's own whole-panel refusal flag, and
	// WholePanelRefusalText is the note slot's text when (and only when) it is set.
	bool bWholePanelRefusal = false;
	FString WholePanelRefusalText;
};

// A precise, named gap in §9.7's own seam list, filed here rather than silently assumed
// (the test author's Phase 5 discipline): §11.2's own G-1 pin requires the diversity oracle to drive
// "the real query path (RunQuery, exercising §9.1's mechanic...)", but §9.7 names no
// query-TRIGGER seam -- only SetDiversityLambdaForTest/SetQueryKForTest/
// GetLastMMRSelectionForTest, none of which executes a query. `RunQuery(const FString&)`
// is private (`SSuperFAISSBankInspector.h:508`) with no existing pass-through. Mirroring
// P-14's own `...ForTest` pattern (a thin pass-through with the production signature, e.g.
// `BuildAnalysisSampleForTest`), Gate 6b's build is required to add:
//   void RunQueryForTest(const FString& QueryText) { RunQuery(QueryText); }
// This is a mechanical application of an already-established codebase pattern, not a new
// design decision the test author is making on the planner's behalf -- routed to the planner
// nonetheless as an explicit §9.7 completion, since a seam list a test author must silently
// extend to make the commission executable is exactly the vagueness Phase 5 exists to
// surface. See the test-design record's "Routed findings" section.

// A second, precise §9.7 gap, named and specified here (D-SLM1832 item 2, 2026-08-08
// completion pass, the same routing shape as RunQueryForTest above): §9.5a names TWO
// mid-selection refusal triggers (the Cosine weighted-zero-norm ZeroNormQuery, and the
// segment-list InvalidArgument) but §9.7 gives no seam reaching the second one -- "a
// correctly operating plugin never constructs [a malformed segment list] itself" (§9.5a's
// own text), so no existing test seam (SetChannelWeightForTest included) can drive it; only
// a direct override of the resolved segment list SelectDiverseMMR's redundancy call
// receives can. Gate 6b's build is required to add:
//   void SetDiversitySegmentOverrideForTest(const TArray<superfaiss::QuerySegment>& Segments);
// Semantics: an empty array (the default) means "no override -- use the widget's own real,
// always-well-formed channel-weight resolution," unchanged from today. A non-empty array
// REPLACES the resolved list for the NEXT RunQueryForTest() call only, then resets to empty
// (consumed, one-shot-per-query) -- mirroring DebugCancelAfterChunks's own one-shot-per-pass
// convention (P-14's precedent for a test-only override that must not silently leak into a
// later, unrelated query in the same test). This is a mechanical application of the
// identical `...ForTest` override pattern `DebugCancelAfterChunks` already establishes, not
// a new design decision on the planner's behalf.
struct FSuperFAISSDiversitySegmentOverrideContract; // documentation marker only, no fields
