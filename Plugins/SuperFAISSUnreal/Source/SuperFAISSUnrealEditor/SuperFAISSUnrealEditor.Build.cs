using System.IO;
using UnrealBuildTool;

public class SuperFAISSUnrealEditor : ModuleRules
{
	public SuperFAISSUnrealEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		// V3.4 plan §11 Scope / §8.9: the drift/diversity oracle test files under
		// Private/Tests/DriftDiversityOracle/*.cpp include the shared assertion helpers
		// ("SuperFAISSDriftDiversityOracleAsserts.h") by bare filename, one directory level
		// up from their own folder (Private/Tests/, not Private/Tests/DriftDiversityOracle/).
		// UBT's default include search covers a module's own Public/Private roots plus each
		// including file's own directory (which is why the sibling "Fixtures/..." includes
		// and the module-root "SSuperFAISSBankInspector.h" include both resolve with no
		// entry here) but not an ARBITRARY ancestor directory two levels up from a deeply
		// nested source file -- this entry adds exactly that one directory, so the governed
		// test files' own #include lines (never edited to work around a missing search
		// path) resolve as authored.
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private", "Tests"));

		// V3.4 drift/diversity plan Gate 6b (the diversity-engine wiring into
		// SSuperFAISSBankInspector, with its RunQueryForTest/SetDiversityLambdaForTest/
		// SetQueryKForTest/GetLastMMRSelectionForTest/SetDiversitySegmentOverrideForTest seams)
		// is built. The diversity tests under Private/Tests/DriftDiversityOracle/ that call those
		// seams are guarded by this define; it is 1, so every one of them compiles
		// and runs.
		PublicDefinitions.Add("SUPERFAISS_GATE6B_BUILT=1");

		// This module compiles its own copy of the vendored SuperFAISS core (see
		// Private/Vendored), so it carries the same FP contract as the runtime module:
		// no implicit FP contraction. See SuperFAISSUnreal.Build.cs for the full note.
		FPSemantics = FPSemanticsMode.Precise;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"SuperFAISSUnreal",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"UnrealEd",
			// Inspector/visualizer tab (plan section 18.2).
			"Slate",
			"SlateCore",
			"InputCore",
			"WorkspaceMenuStructure",
			// USuperFAISSInspectorSettings, UDeveloperSettings (V3.2 plan section 25.3).
			"DeveloperSettings",
			// IPluginManager, for the slot-3 grep-target regression test's module-dir lookup.
			"Projects",
			// UE::Trace::ToggleChannel (V3.2 slot 5, plugin plan section 5.1/25.6): the
			// B8-extension non-perturbation test toggles the SuperFAISS channel; needs
			// TraceLog named explicitly to resolve at link time (see the runtime
			// module's Build.cs for the identical note).
			"TraceLog",
			// SF34-002: the "Open Archive..." affordance's native file-picker
			// (IDesktopPlatform::OpenFileDialog) and its parent-window resolution
			// (IMainFrameModule::GetParentWindow) for the modal dialog.
			"DesktopPlatform",
			"MainFrame",
		});
	}
}
