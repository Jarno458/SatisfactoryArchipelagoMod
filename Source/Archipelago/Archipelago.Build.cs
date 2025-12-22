using UnrealBuildTool;
using System.IO;
using System;

public class Archipelago : ModuleRules
{
    public Archipelago(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        CppStandard = CppStandardVersion.Cpp20;
        bUseUnity = false;

        // FactoryGame transitive dependencies
        // Not all of these are required, but including the extra ones saves you from having to add them later.
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core", "CoreUObject",
            "Engine",
            "DeveloperSettings",
            "PhysicsCore",
            "InputCore",
            "GeometryCollectionEngine",
            "AnimGraphRuntime",
            "AssetRegistry",
            "NavigationSystem",
            "AIModule",
            "GameplayTasks",
            "SlateCore", "Slate", "UMG",
            "RenderCore",
            "CinematicCamera",
            "Foliage",
            "EnhancedInput",
            "NetCore",
            "GameplayTags",
            "Json", "JsonUtilities",
            "AssetRegistry"
        });

        // Header stubs
        PublicDependencyModuleNames.AddRange(new string[] {
            "DummyHeaders",
        });

        // Mod code depdencies
        PrivateDependencyModuleNames.AddRange(new string[] { 
            "ContentLib",
            "FreeSamples",
            "APCpp",
        });

        if (Target.Type == TargetRules.TargetType.Editor) {
            PublicDependencyModuleNames.AddRange(new string[] { "AnimGraph", "UnrealEd" });
        }

        PublicDependencyModuleNames.AddRange(new string[] {"FactoryGame", "SML"});

        if (Target.Type == TargetRules.TargetType.Server)
        {
            PublicDependencyModuleNames.Add("FactoryDedicatedServer");
        }
        else
        {
            //not using client specific code atm
            //PublicDependencyModuleNames.Add("FactoryDedicatedClient");
        }

        CppStandard = CppStandardVersion.Cpp20;
        //UseUnityBuild = false;
        OptimizeCode = CodeOptimization.Never;
    }
}
