// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BYGMultiplayer : ModuleRules
{
	public BYGMultiplayer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
			}
			);
			
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
				"OnlineSubsystem",
				"OnlineSubsystemUtils",
				"OnlineSubsystemSteam",
				"Steamworks",
				"ImGui"
			}
			);
	}
}
