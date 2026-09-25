// SuperFAISS For Unreal 3.4 -- dim 3b, diversity's concurrency absence claim
// (the 3.4 drift-and-diversity plan §12 dim 3, coverage-model
// audit G-29; re-scoped by D-SLM1871). `SelectDiverseMMR`/`ScoreXdPairSegmented` are
// synchronous, single-threaded, allocation-free-on-the-caller's-buffers functions with no
// thread/task/async handle in their specification (§6.2) -- proven here by scanning the two
// kernels' own translation units and complete call graph for the banned-symbol set the
// existing `FSuperFAISSInspectorConcurrencyGrepTargetTest` uses
// (`SuperFAISSInspectorPanelTests.cpp`):
//   Source/ThirdParty/SuperFAISS/include/superfaiss/diversity.h
//   Source/ThirdParty/SuperFAISS/src/diversity.cpp
//   Source/ThirdParty/SuperFAISS/include/superfaiss/analytics.h
//   Source/ThirdParty/SuperFAISS/src/analytics.cpp
// Both kernels call nothing outside these four files, and neither has any call-graph
// presence under `Source/SuperFAISSUnreal` (D-SLM1871), so the runtime-module scan root the
// earlier construction carried was never load-bearing for this claim and is removed.
//
// COMPILE STATUS: compiles and runs TODAY -- a pure file read, no panel or core symbol
// dependency at all.
//
// StandardsDocument.md §4's own coverage requirement: a new structure is validated against
// an independently-found population before it is trusted. Two properties make this scan
// able to fail: every one of the four files must actually be read (an unreadable or
// mistyped path fails the cell instead of contributing zero hits), and
// FSuperFAISSDiversityConcurrencyScanFindsInjectedSymbolTest below injects a banned symbol
// into a scratch copy of EACH of the four files and confirms the scan counts it.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "SuperFAISSDriftDiversityOracleAsserts.h"
#include "HAL/FileManager.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
	const TCHAR* kBannedSymbols[] = {TEXT("QueryAsync"), TEXT("FSuperFAISSTicket"), TEXT("AsyncTask(")};

	// The four files D-SLM1871 names, relative to Source/ThirdParty/SuperFAISS.
	const TCHAR* kKernelFiles[] = {
		TEXT("include/superfaiss/diversity.h"),
		TEXT("src/diversity.cpp"),
		TEXT("include/superfaiss/analytics.h"),
		TEXT("src/analytics.cpp"),
	};

	FString ThirdPartyRoot(const IPlugin& Plugin)
	{
		return FPaths::Combine(Plugin.GetBaseDir(), TEXT("Source/ThirdParty/SuperFAISS"));
	}

	// Counts banned-symbol occurrences (one per symbol per file) across Files. A file that
	// cannot be read is counted in OutUnreadable rather than silently skipped.
	int32 CountBannedSymbolHits(const TArray<FString>& Files, int32& OutUnreadable)
	{
		int32 Hits = 0;
		OutUnreadable = 0;
		for (const FString& File : Files)
		{
			FString Contents;
			if (!FFileHelper::LoadFileToString(Contents, *File))
			{
				++OutUnreadable;
				continue;
			}
			for (const TCHAR* Symbol : kBannedSymbols)
			{
				if (Contents.Contains(Symbol)) { ++Hits; }
			}
		}
		return Hits;
	}
}

// ===========================================================================
// dim 3b: the four-file scan. Zero hits expected, and all four files must be read.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityConcurrencyExtendedScanTest,
	"SuperFAISS.D.DiversityOracle.ConcurrencyExtendedScan",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityConcurrencyExtendedScanTest::RunTest(const FString& Parameters)
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SuperFAISSUnreal"));
	AssertExactValue(*this, TEXT("(setup) SuperFAISSUnreal plugin found"), Plugin.IsValid(), true);
	if (!Plugin.IsValid()) { return true; }

	TArray<FString> Files;
	for (const TCHAR* Rel : kKernelFiles)
	{
		Files.Add(FPaths::Combine(ThirdPartyRoot(*Plugin), Rel));
	}

	int32 Unreadable = 0;
	const int32 Hits = CountBannedSymbolHits(Files, Unreadable);
	AssertExactValue(*this, TEXT("all four kernel files read"), Unreadable, 0);
	AssertExactValue(*this, TEXT("diversity.h/.cpp + analytics.h/.cpp: zero banned-symbol hits"), Hits, 0);
	return true;
}

// ===========================================================================
// StandardsDocument.md §4: a guard that goes green on files it has never scanned proves only
// that it ran, not that it covers. For each of the four kernel files, this test writes a
// scratch copy with a banned symbol appended, confirms the scan counts exactly one hit for
// that copy, then deletes the copy.
// ===========================================================================
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FSuperFAISSDiversityConcurrencyScanFindsInjectedSymbolTest,
	"SuperFAISS.D.DiversityOracle.ConcurrencyScanGuardVitality",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FSuperFAISSDiversityConcurrencyScanFindsInjectedSymbolTest::RunTest(const FString& Parameters)
{
	const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("SuperFAISSUnreal"));
	AssertExactValue(*this, TEXT("(setup) SuperFAISSUnreal plugin found"), Plugin.IsValid(), true);
	if (!Plugin.IsValid()) { return true; }

	const FString ScratchDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SuperFAISSConcurrencyScanProbe"));
	for (const TCHAR* Rel : kKernelFiles)
	{
		const FString Source = FPaths::Combine(ThirdPartyRoot(*Plugin), Rel);
		FString Contents;
		const bool bRead = FFileHelper::LoadFileToString(Contents, *Source);
		AssertExactValue(*this, FString::Printf(TEXT("(setup) %s read"), Rel), bRead, true);
		if (!bRead) { continue; }

		// The injection is observable only if the original carries no banned symbol already.
		int32 OriginalUnreadable = 0;
		AssertExactValue(*this, FString::Printf(TEXT("(setup) original %s has zero hits"), Rel),
			CountBannedSymbolHits({Source}, OriginalUnreadable), 0);

		const FString ScratchPath = FPaths::Combine(ScratchDir, FPaths::GetCleanFilename(Rel));
		Contents += TEXT("\n// scratch probe, deleted by the test that wrote it\nvoid Probe() { AsyncTask(nullptr); }\n");
		const bool bWrote = FFileHelper::SaveStringToFile(Contents, *ScratchPath);
		AssertExactValue(*this, FString::Printf(TEXT("(setup) scratch copy of %s written"), Rel), bWrote, true);

		int32 Unreadable = 0;
		const int32 Hits = CountBannedSymbolHits({ScratchPath}, Unreadable);
		IFileManager::Get().Delete(*ScratchPath);

		AssertExactValue(*this, FString::Printf(TEXT("injected AsyncTask( found in the copy of %s"), Rel), Hits, 1);
		AssertExactValue(*this, FString::Printf(TEXT("copy of %s read"), Rel), Unreadable, 0);
	}
	IFileManager::Get().DeleteDirectory(*ScratchDir, false, true);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
