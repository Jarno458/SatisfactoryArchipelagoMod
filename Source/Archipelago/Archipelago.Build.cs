using UnrealBuildTool;
using System.IO;
using System;

public class Archipelago : ModuleRules
{
    public Archipelago(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject",
            "Engine",
            "InputCore",
            "OnlineSubsystem", "OnlineSubsystemUtils", "OnlineSubsystemNULL",
            "SignificanceManager",
            "PhysX", "APEX", "PhysXVehicles", "ApexDestruction",
            "AkAudio",
            "ReplicationGraph",
            "UMG",
            "AIModule",
            "NavigationSystem",
            "AssetRegistry",
            "GameplayTasks",
            "AnimGraphRuntime",
            "Slate", "SlateCore",
            "Json"
            });


        if (Target.Type == TargetRules.TargetType.Editor) {
			PublicDependencyModuleNames.AddRange(new string[] {"OnlineBlueprintSupport", "AnimGraph"});
		}
        PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});

        var modelThirdPartyDir = Path.Combine(ModuleDirectory, "ThirdParty");
        PublicAdditionalLibraries.Add(Path.Combine(modelThirdPartyDir, "lib", "jsoncpp.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(modelThirdPartyDir, "lib", "ixwebsocket.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(modelThirdPartyDir, "lib", "APCpp-static.lib"));
        PublicIncludePaths.Add(Path.Combine(modelThirdPartyDir, "include"));
    }
}