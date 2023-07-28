using UnrealBuildTool;
using System.IO;
using System;

public class Archipelago : ModuleRules
{
    public Archipelago(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // FactoryGame transitive dependencies
        // Not all of these are required, but including the extra ones saves you from having to add them later.
        PublicDependencyModuleNames.AddRange(new[] {
            "Core", "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "PhysicsCore",
            "InputCore",
            "OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemUtils",
            "SignificanceManager",
            "GeometryCollectionEngine",
            "ChaosVehiclesCore", "ChaosVehicles", "ChaosSolverEngine",
            "AnimGraphRuntime",
            "AkAudio",
            "AssetRegistry",
            "NavigationSystem",
            "ReplicationGraph",
            "AIModule",
            "GameplayTasks",
            "SlateCore", "Slate", "UMG",
            "InstancedSplines",
            "RenderCore",
            "CinematicCamera",
            "Foliage",
            "Niagara",
            "EnhancedInput",
            "GameplayCameras",
            "TemplateSequence",
            "NetCore",
            "GameplayTags",
		});

        // FactoryGame plugins
        PublicDependencyModuleNames.AddRange(new[] {
            "AbstractInstance",
            "InstancedSplinesComponent",
            "SignificanceISPC"
        });

        // Header stubs
        PublicDependencyModuleNames.AddRange(new[] {
            "DummyHeaders",
        });

        //PrivateDependencyModuleNames.AddRange(new string[] { "ContentLib" });

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