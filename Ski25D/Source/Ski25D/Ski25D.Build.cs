// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Ski25D : ModuleRules
{
	public Ski25D(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ProceduralMeshComponent", "Slate", "SlateCore", "AudioMixer" });
	}
}
