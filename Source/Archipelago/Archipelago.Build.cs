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
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "PhysicsCore",
            "InputCore",
            //"OnlineSubsystem", "OnlineSubsystemUtils", "OnlineSubsystemNull",
            //"SignificanceManager",
            "GeometryCollectionEngine",
            //"ChaosVehiclesCore", "ChaosVehicles", "ChaosSolverEngine",
            "AnimGraphRuntime",
            //"AkAudio",
            "AssetRegistry",
            "NavigationSystem",
            //"ReplicationGraph",
            "AIModule",
            "GameplayTasks",
            "SlateCore", "Slate", "UMG",
            //"InstancedSplines",
            "RenderCore",
            "CinematicCamera",
            "Foliage",
            //"Niagara",
            "EnhancedInput",
            //"GameplayCameras",
            //"TemplateSequence",
            "NetCore",
            "GameplayTags",
		});

        // FactoryGame plugins
        PublicDependencyModuleNames.AddRange(new string[] {
            //"AbstractInstance",
            //"InstancedSplinesComponent",
            //"SignificanceISPC"
        });

        // Header stubs
        PublicDependencyModuleNames.AddRange(new string[] {
            "DummyHeaders",
        });

        PrivateDependencyModuleNames.AddRange(new string[] { "ContentLib" });

        if (Target.Type == TargetRules.TargetType.Editor) {
            PublicDependencyModuleNames.AddRange(new string[] {"OnlineBlueprintSupport", "AnimGraph"});
        }
        PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});

        var thirdPartyDir = Path.Combine(ModuleDirectory, "ThirdParty");
        PublicIncludePaths.Add(Path.Combine(thirdPartyDir, "include"));
        PublicAdditionalLibraries.Add(Path.Combine(thirdPartyDir, "lib", "APCpp-static.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(thirdPartyDir, "lib", "ixwebsocket.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(thirdPartyDir, "lib", "jsoncpp.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(thirdPartyDir, "lib", "mbedcrypto.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(thirdPartyDir, "lib", "mbedtls.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(thirdPartyDir, "lib", "mbedx509.lib"));
        PublicAdditionalLibraries.Add(Path.Combine(thirdPartyDir, "lib", "crypt32.lib"));
        //RuntimeDependencies.Add("$(BinaryOutputDir)/APCpp.dll", Path.Combine(thirdPartyDir, "dll", "APCpp.dll"));
    }
}
