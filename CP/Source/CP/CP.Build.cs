// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CP : ModuleRules
{
	public CP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
			"NavigationSystem",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"CP",
			"CP/Variant_Platforming",
			"CP/Variant_Platforming/Animation",
			"CP/Variant_Combat",
			"CP/Variant_Combat/AI",
			"CP/Variant_Combat/Animation",
			"CP/Variant_Combat/Gameplay",
			"CP/Variant_Combat/Interfaces",
			"CP/Variant_Combat/UI",
			"CP/Variant_SideScrolling",
			"CP/Variant_SideScrolling/AI",
			"CP/Variant_SideScrolling/Gameplay",
			"CP/Variant_SideScrolling/Interfaces",
			"CP/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
